# 04 — GRO / GSO：聚合与分段的代价

> **对应 Rosen:** Ch11（Layer 4，sk_buff 处理）
> **内核版本:** GRO 2.6.29+；GSO 2.6.26+；UDP GRO **5.0+**；USO **5.x+**
> **内核源码路径:** `net/core/gro.c`、`net/core/skbuff.c`、`include/linux/skbuff.h`、`net/ipv4/udp_offload.c`

## 文档概述

原笔记正确给了"HFT 应关 GRO"的结论，但没讲**为什么 GRO 会引入延迟**。
这个"为什么"很重要 —— 因为有个反直觉的点：

> **GRO 是把延迟换吞吐。** 你关掉它是把吞吐换回去。
> 如果你的瓶颈根本不是延迟而是"处理不过来"，关 GRO 会让情况**更糟**。

本篇讲清这个权衡，以及 GSO/TSO/USO 在**发送**侧的对应问题。

---

## 一、GRO 到底在等什么

### 合并发生在哪

```
驱动 poll
   │
   ├─ napi_gro_receive(napi, skb)
   │     └─ dev_gro_receive()
   │           ├─ gro_find_receive_by_type() 按 flow hash 找 napi->gro_hash 里的同流 skb
   │           ├─ 找到且能合并 → skb_gro_receive() 把新包并进已有 skb
   │           │                    （只加页引用，不拷贝数据）
   │           └─ 找不到 → 新建一个"待合并"条目挂进 gro_hash，先不上送
   │
   └─ napi_gro_flush(napi)            ← poll 结束时冲刷
         └─ 把 gro_hash 里还没凑满的条目全部上送协议栈
```

**延迟就是从"挂进 gro_hash"到"被 flush"之间的这段时间。**

### 合并条件

| 层 | 要求 |
|----|------|
| L2 | 同一入端口、同一 MAC 头 |
| L3 | 同 src/dst IP、同协议、同 TOS |
| L4（TCP） | 同 src/dst port、**TCP 序号连续**、标志位兼容 |
| L4（UDP） | 同 src/dst port（UDP 无序号，按到达顺序合并） |

**注意 UDP GRO 的语义差异**：TCP 有序号，乱序就不合并；
UDP 没有序号，**先到什么就合并什么**。这意味着 UDP GRO 合并出来的
"大包"里，各原始数据报的边界是**靠 `gso_size` 反推**的，
内核会把它们重新拆回 N 个等长的 UDP 数据报再交付。

### 什么时候会被 flush

1. **攒够数量** —— 受 `net.core.gro_normal_batch`（默认 8）与分片上限约束
2. **超时** —— `gro_flush_timeout`（`/sys/class/net/<dev>/gro_flush_timeout`，**单位纳秒**）
3. **NAPI poll 结束** —— `napi_gro_flush()` 强制冲刷
4. **同流出现不兼容的包** —— 例如 TCP 标志位变了（SYN/FIN/RST）立刻 flush

```bash
sysctl net.core.gro_normal_batch           # 8
cat /sys/class/net/eth0/gro_flush_timeout  # 纳秒
```

---

## 二、收包侧：GRO 的四种形态

| 形态 | 开关 | 说明 |
|------|------|------|
| 软件 GRO | `ethtool -K eth0 gro on` | 内核做，最通用 |
| 硬件 GRO / LRO | `ethtool -K eth0 rx-gro-hw on` | 网卡做，**HFT 别开**（连内核都看不见原始包边界） |
| UDP GRO | `ethtool -K eth0 rx-udp-gro-forwarding on` | 5.0+，为转发场景设计 |
| 关闭 | `ethtool -K eth0 gro off` | 每包独立上送，延迟最低 |

```bash
# 看当前所有 offload 状态
ethtool -k eth0 | grep -E 'gro|gso|tso|lro|udp'
```

> ⚠️ `rx-gro-hw`（硬件聚合）比软件 GRO 更糟：包在进内核之前就被合并了，
> 连 `tcpdump` 看到的都是合并后的大包，**丢包定位和延迟归因全部失真**。

---

## 三、发包侧：GSO / TSO / USO

GRO 的镜像操作。核心思想是**推迟分段**：

```
应用 send() 大块数据
   → 协议栈构造一个大的 skb（可达 64KB）
   → 不立刻分段
   → 到驱动出口才分：
        ├─ TSO：网卡硬件分（TCP）
        ├─ GSO：内核软件兜底分（硬件不支持时）
        └─ USO：UDP 分段卸载（5.x+）
```

| 机制 | 分层 | 硬件/软件 | 说明 |
|------|------|----------|------|
| TSO | TCP | 硬件 | 最成熟，10G+ 网卡基本都支持 |
| GSO | 通用 | 软件 | 兜底，也可在不支持 TSO 时提供批量收益 |
| GSO partial | TCP | 混合 | 内核分一部分、硬件分一部分 |
| UFO | UDP | 硬件 | **已在 5.x 移除** |
| USO | UDP | 硬件 | 5.x+ 替代 UFO，`tx-udp-segmentation` |

**关键字段**（`struct sk_buff`）：

```c
skb->gso_size   /* 每个分段的 payload 长度 */
skb->gso_segs   /* 会分成几段 */
skb->gso_type   /* SKB_GSO_TCPV4 / SKB_GSO_UDP_L4 / ... */

/* 判断是不是 GSO 包 */
if (skb_is_gso(skb)) { ... }
```

### 发送侧对 HFT 的影响

TSO/GSO 的收益是**省 CPU**（一次性构造大 skb，而不是 N 个小 skb）。
但代价是**发送时机被批量化**：

```
你的交易报文很小（几十~几百字节）
开 TSO：内核可能攒着跟后续数据合并后再发
      → 你的报文被推迟了
```

所以：

```bash
# 交易/报单口的典型配置
ethtool -K eth0 tso off gso off gro off
```

---

## 四、性能权衡（含"什么时候不该关"）

| 场景 | 建议 | 理由 |
|------|------|------|
| **行情接收（HFT）** | `gro off` | 每个 tick 都要尽快单独交付，等合并窗口 = 白等 |
| **报单发送（HFT）** | `tso off gso off` | 避免小报文被攒批 |
| 高 pps 但 CPU 打满 | `gro on` | 减少协议栈遍历次数，先活下来再谈延迟 |
| 转发/路由 | `rx-udp-gro-forwarding on` | 5.0+ 专为转发设计，合并后直接转出去 |
| 抓包排障期 | `gro off` **且** `rx-gro-hw off` | 否则抓到的包和真实包不一致 |

**判断你属于哪种**：

```bash
# 如果这两个在涨，说明你"处理不过来"，不该关 GRO
cat /proc/net/softnet_stat    # 第 2 列 time_squeeze
ethtool -S eth0 | grep -E 'missed|no_buf|drop'
```

---

## 五、观测命令

```bash
# 当前 offload 全貌
ethtool -k eth0 | grep -E 'gro|gso|tso|lro|udp'

# 关闭 HFT 不需要的（收包口）
ethtool -K eth0 gro off lro off rx-gro-hw off
# 关闭 HFT 不需要的（发包口）
ethtool -K eth0 tso off gso off

# GRO 批量上送的数量
sysctl net.core.gro_normal_batch

# 验证 GRO 真的关了：tcpdump 应看到未合并的小包
tcpdump -i eth0 -nn -c 20 'udp port 12345'
# 若看到 length 明显大于单个行情包（如 >1500），说明还有某层在聚合
```

---

## HFT 要点

- **GRO 引入延迟的机制是"等待合并窗口"**，不是"合并这个动作慢"。
  合并本身只花几十纳秒，等窗口能等到几百微秒 —— 量级完全不同
- **关 GRO 是有代价的**：协议栈遍历次数回到 O(包数)，CPU 占用上升
- **`rx-gro-hw` 是排障的敌人**：它让 tcpdump 看到假包
- **`gro_flush_timeout` 单位是纳秒**，写成 200 = 0.2μs（常见错误）
  → 该参数在 NAPI defer 场景下更重要，见 [05](./05-busy-poll-mechanism.md)
- **发送侧别忘 TSO**：只关 GRO 不关 TSO，报单口还是会被攒批

## 与 Rosen 3.x 的差异

| 维度 | Rosen（3.x） | 现代（5.x/6.x） |
|------|-------------|----------------|
| UDP GRO | 无 | 5.0+ 支持（`rx-udp-gro-forwarding`） |
| UDP 分段 | UFO | **UFO 已移除**，改为 USO（5.x+） |
| 硬件聚合 | LRO（粗糙，问题多） | `rx-gro-hw` + LRO 并存，仍不推荐 HFT 使用 |
| 上送方式 | 逐 skb | `netif_receive_skb_list()` 批量 |
| flush 控制 | 固定逻辑 | `gro_flush_timeout`（ns）**且被 NAPI defer 复用** |

## 代码自测

<details>
<summary>Q1：我已经 `ethtool -K eth0 gro off`，但 tcpdump 抓到的包还是比实际大，为什么？</summary>

大概率还有一层在聚合，按这个顺序查：

1. `ethtool -k eth0 | grep rx-gro-hw` —— 硬件 GRO 开着的话，
   包在**进内核之前**就被合并了，`gro off` 管不到它
2. `ethtool -k eth0 | grep lro` —— LRO 同理，且比 GRO 更激进
3. 抓包点在聚合之后 —— `tcpdump` 走 AF_PACKET，位置在 GRO **之后**。
   想看原始包要在驱动层或用 XDP 抓

正确关法：`ethtool -K eth0 gro off lro off rx-gro-hw off`
</details>

<details>
<summary>Q2：UDP GRO 没有序号，合并后怎么还原成一个个数据报？</summary>

靠 `gso_size` 反推。合并时内核记录每个原始数据报的长度到
`skb->gso_size`，交付时 UDP 层调用 `udp_rcv()` 前先经过
`udp_gro_receive()/udp_gro_complete()`，把大 skb 按 `gso_size`
切成 N 个等长数据报再逐个上送。

**推论：如果一条流里的 UDP 包长度不一致，UDP GRO 的合并条件就不满足**
（它要求同长度才能用同一个 `gso_size` 反推）。
行情数据里如果有多种消息类型、长度不一，UDP GRO 实际合并率会很低 ——
这也意味着开了基本没收益，不如直接关掉。
</details>

<details>
<summary>Q3：关掉 GRO 后 CPU 占用涨了很多，但延迟确实降了，这个取舍怎么判断？</summary>

看你的**瓶颈在哪一侧**：

- 延迟已经达标、CPU 还有富余 → 保持 `gro off`，延迟优先
- CPU 接近打满、`softnet_stat` 的 `time_squeeze` 在涨 → 说明"处理不过来"
  已经成了主要矛盾，此时 GRO 换来的吞吐比那点延迟更重要
- 两者都紧张 → 说明该考虑旁路了（AF_XDP / DPDK），
  它们从根上绕开了"要不要合并"这个问题
  → [../../chapter-07-xdp-redirect-dpdk/](../../chapter-07-xdp-redirect-dpdk/)

判断依据永远是**实测数据**，不是"听说 HFT 要关 GRO"。
</details>
