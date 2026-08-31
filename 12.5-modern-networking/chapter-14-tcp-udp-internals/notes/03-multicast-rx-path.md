# 03 — UDP 组播收包路径与行情接入

> **对应 Rosen:** 组播路由有覆盖；**收包路径上的组播分支未展开**（本篇补这部分）
> **内核源码路径:** `net/ipv4/ip_input.c`、`net/ipv4/udp.c`、`net/ipv4/igmp.c`

## 文档概述

行情链路几乎全部走 **UDP 组播**（ITCH/OUCH over MoldUDP64），而不是 TCP。
本笔记补全现代内核中组播收包的完整路径，以及**旁路之后 IGMP 失效**这个最容易踩的坑。

> 📌 本篇原在 chapter-02-napi-rx-path 下，已移至本章 —— 组播属**协议内部机制**
> （L2 有损映射、IGMP、组播复制、gap 检测），不是 NAPI 收包路径的内容。

→ 前置：[chapter-02/01-rx-path-bootlin](../../chapter-02-napi-rx-path/notes/01-rx-path-bootlin.md)（通用收包路径）
→ 相关：[chapter-02/06-queue-steering-rss](../../chapter-02-napi-rx-path/notes/06-queue-steering-rss.md)（把组播流钉到指定队列）
→ 下游：[13-dpdk 组播行情接入](../../../13-dpdk/01-Intro-Book/notes/chapter-05-组播行情接入.md)

---

## 核心内容

### 为什么行情一定是组播

| 维度 | 单播 TCP | 组播 UDP |
|------|---------|---------|
| 交易所带宽 | 随客户数线性增长 | 恒定，一份流 |
| 复制位置 | 交易所服务器 | 交换机 / 网卡 |
| 可靠性 | 内核保证（重传、排序） | **应用层自己保证** |
| 延迟 | 有握手、ACK、拥塞控制 | 无，纯单向 |
| 一个包的去留 | 只影响自己 | 丢包影响所有订阅者 |

行情的本质是"同一份数据、同时给所有人"，组播把复制代价从交易所转嫁给网络设备，
这是唯一能支撑上万客户端同时收同一份 orderbook 的方式。

**代价转嫁给了你：** UDP 不重传、不排序、不保到达。接收端必须自己做
**序列号 gap 检测 + 独立的 TCP 重传通道**（交易所一般提供 "retransmission / snapshot" 服务）。

---

### IPv4 组播 → MAC 地址映射（第一个坑）

网卡硬件过滤工作在 L2，但组播成员管理在 L3。二者靠一个**有损映射**连接：

```
IPv4 组播地址      224.x.y.z        (224.0.0.0/4, 高 4 位 = 1110)
                        │
                        │  只取低 23 位（丢掉高 5 位）
                        ▼
MAC 地址           01:00:5E:0<23 位>
```

映射规则：`MAC 低 23 位 = IPv4 组播地址低 23 位`。

**后果：2^5 = 32 个不同的 IPv4 组播地址，映射到同一个 MAC 地址。**

| 现象 | 影响 |
|------|------|
| 网卡按 MAC 过滤 | 会收到"你不想要的"另外 31 个组播地址的流量 |
| 多余流量进入驱动 | 消耗 XDP/sk_buff 处理，直到 IP 层才被丢弃 |
| HFT 应对 | 向交易所确认组播地址分配，**避免选到同 MAC 的另一个组** |

这也是为什么 `XDP` 早过滤在组播场景收益特别大——L2 硬件过滤不过滤干净的部分，
在 XDP 里一次判断就能丢掉，省掉后面的 sk_buff 分配和协议栈遍历。

---

### 组播收包路径（对比单播多出来的步骤）

```
NIC 硬件组播过滤（ imperfect filter，见上）
   ↓
[交换机侧] IGMP snooping 决定是否往本端口转发
   ↓
驱动收帧 → XDP hook                    ← 无关组播在此 DROP，收益最大
   ↓
sk_buff 分配
   ↓
ip_rcv() → ip_route_input_mc()         ← 组播专用路由判定（非普通路由表）
   ↓
ip_mc_sf_allow()  源过滤（IGMPv3 SSM）
   ↓
udp_rcv() → __udp4_lib_mcast_deliver() ← 组播 socket 匹配
   ↓
★ skb clone/copy → 每一个匹配的 socket 各一份
   ↓
各 socket receive queue → 唤醒用户进程
```

**单播没有的两处开销：**

| 步骤 | 说明 |
|------|------|
| `ip_route_input_mc()` | 组播不走普通路由表，走组播路由判定（LOCAL / FORWARD / 丢弃） |
| `__udp4_lib_mcast_deliver()` | 一个 skb 要复制给**所有**匹配 socket |

**关键陷阱：同一台机器上开 N 个进程收同一个组播组，内核要复制 N 份 skb。**
每份都要走一遍协议栈后半段。所以 HFT 机器上"多开一个监控进程看同一份行情"
不是免费的——它直接放大主进程的延迟抖动。正确做法是**单进程收 + 用户态分发**（共享内存 / SPSC ring）。

---

### IGMP：旁路之后最大的坑

内核协议栈会自动发 IGMP Membership Report（v2/v3），告诉交换机"我要这个组"。
交换机靠 IGMP snooping 只往订阅过的端口转发。**不发 report = 收不到任何包。**

```
内核栈路径：  socket(AF_INET, SOCK_DGRAM)
              setsockopt(IP_ADD_MEMBERSHIP)   ← 内核自动发 IGMP report ✓
                    ↓ 交换机学到 → 开始转发

DPDK 旁路：   网卡被 UIO/VFIO 接管，内核不再管这张网卡
              rte_eth_allmulticast_enable()   ← 只是让网卡"硬件不过滤"
              ✗ 没有任何 IGMP report 发出
              ✗ 交换机从未学习 → 不往这个端口转发
              ✗ 程序跑了半天，一个包都收不到，还以为 rx_burst 写错了
```

**三种解法：**

| 方案 | 做法 | 场景 |
|------|------|------|
| 交换机静态组播 | 配 static mrouter port / static group 到该端口 | 托管机房、自家交换机，最省事 |
| 应用自己发 IGMP | DPDK 里构造 IGMPv2/v3 Membership Report 并从该口发出 | 无权改交换机配置 |
| 端口 flood | 交换机配成泛洪 | 不推荐，流量放大几十倍 |

→ 实操见 [13-dpdk/code/mcast-minimal](../../../13-dpdk/01-Intro-Book/code/mcast-minimal/README.md)

---

### 丢包定位（组播没有重传，必须自己发现）

组播 UDP 丢包是静默的——协议栈不会告诉你。必须主动监控：

```bash
# 网卡层丢包（驱动收不进来）
ethtool -S eth0 | grep -Ei 'miss|no_buf|drop|fifo'

# 内核 UDP 层丢包（socket 队列满）
cat /proc/net/udp        # 看 drops 列
ss -ulpn                 # Recv-Q 堆积

# AF_XDP 专用
xdp-stat queue 0         # rx_dropped = FILL ring 空
```

| 层 | 计数字段 | 典型原因 |
|----|---------|---------|
| NIC | `rx_missed`, `rx_no_buffer` | 网卡 buffer 满，PCIe/RSS 跟不上 |
| 驱动 | `rx_nombuf`（DPDK） | mbuf 池耗尽，用户态消费太慢 |
| AF_XDP | `rx_dropped` | FILL ring 空，没及时回填 buffer |
| socket | `/proc/net/udp` drops | 接收队列满，用户态消费太慢 |

**HFT 硬要求：** 应用层必须做**序列号 gap 检测**。交易所组播头里带递增序号，
发现跳号立刻走 TCP 重传通道补单，而不是等下一个快照。

---

## HFT 要点

- **行情 = UDP 组播，不是 TCP。** 指望内核帮你重传、排序是错的，这些都在应用层
- **L2 映射有损：** 32 个组播 IP 共用一个 MAC，多余的 31 个要靠 XDP 或 IP 层过滤
- **同机组播多收一份 = 多一次 skb 复制**，监控进程要收用户态分发，不要多开进程订阅同一组
- **旁路后 IGMP 失效**是最高频的踩坑点：先确认交换机侧是否配了静态组播
- **gap 检测是刚需**，没有它你不知道自己看到的是不是完整的 orderbook

## 与 Rosen 3.x 的差异

| 维度 | Rosen（3.x） | 现代（5.x/6.x） |
|------|-------------|----------------|
| 组播过滤 | 主要靠 IP 层 + 协议栈 | XDP 可在 sk_buff 之前 DROP，省掉分配 |
| Rx buffer | 动态 alloc_page | page_pool 复用，AF_XDP 直接映射 |
| 旁路后 IGMP | 未涉及（当时 XDP 不存在） | 旁路必须自己处理 IGMP，否则收不到流 |
| 组播复制 | 已有 `__udp4_lib_mcast_deliver` | 机制不变，但多队列 + RSS 下队列选择影响延迟 |
