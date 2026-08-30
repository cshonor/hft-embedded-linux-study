# Ch 5 组播行情接入 · Multicast Market Data

> **01-Intro-Book** · 官方 Programmer's Guide · **精读**

> **内核对照：** [12.5/chapter-02/notes/05-multicast-rx-path](../../../12.5-modern-networking/chapter-02-napi-rx-path/notes/05-multicast-rx-path.md)
> —— 那篇讲**内核栈**里组播怎么走，本篇讲**旁路之后**要自己补哪些事。两篇对着读。

> **实验：** [code/mcast-minimal/](../code/mcast-minimal/)（DPDK 版 + 内核栈对照版，同口径分位统计）

---

## 为什么行情是组播

一份 orderbook 同时给上万人，交易所不可能单发 —— 组播把复制代价转嫁给交换机，
这是唯一能支撑"同一份数据、数万订阅者"的方式。

代价是 **UDP 不重传、不排序、不保到达**，接收端必须自己做
**序列号 gap 检测 + 独立的 TCP 补单通道**。
（详见内核栈那篇，此处不重复）

---

## 旁路之后：内核不再帮你做的四件事

这是本笔记的核心。走 `rte_eth_rx_burst()` 拿到的是**裸以太网帧**，
以下这些原先由内核协议栈默默做的事，全部消失：

| 内核原本做的 | 旁路后 | 必须自己处理 |
|---|---|---|
| **发 IGMP Membership Report** | 没人发，交换机 IGMP snooping 学不到 | 交换机配静态组播，**或**应用自己构造 IGMP 报文 |
| ARP 请求 / 应答 | 网卡被 DPDK 独占，不响应 ARP | 管理口走另一张网卡；或自己实现 ARP 应答 |
| IP / UDP 校验和验证 | **不验证**，坏包照样递给你 | 行情场景通常跳过（省 CPU），但要知情 |
| IP 分片重组 | 不重组，分片各自独立到达 | 行情包应 < MTU，交易所一般会保证 |
| 组播复制（多进程各一份） | 无，用户态只有一份 | 用户态自己分发（SPSC ring / 共享内存） |
| 序列号 / 重传 | 本来就没有 | gap 检测 + TCP 补单通道 |

**最大的坑是第一行。** 典型症状：程序跑起来 `rx_burst` 一直返回 0，
查半天代码，实际是交换机根本没往这个端口转发。
排查顺序：`ethtool -S` 看网卡计数 → 计数不涨是交换机/IGMP 问题；
计数涨但应用收不到，才是用户态代码问题。

---

## DPDK 侧的组播接收配置

| 方式 | 含义 | 取舍 |
|------|------|------|
| `rte_eth_allmulticast_enable()` | 收**所有**组播 | 最常用；会收到同 MAC 的无关组，靠软件过滤 |
| `rte_eth_dev_set_mc_addr_list()` | 只收指定组播地址 | 更精确，依赖驱动支持 |
| `rte_eth_promiscuous_enable()` | 混杂模式，什么都收 | 调试用；生产环境杂包最多 |
| `rte_flow` / Flow Director | 硬件把行情流钉到指定队列 | 与 [12.5/07 队列定向](../../../12.5-modern-networking/chapter-02-napi-rx-path/notes/07-queue-steering-rss.md) 配合 |

注意 **IPv4 组播 → MAC 是有损映射**：32 个组播 IP 共用同一个 MAC
（`01:00:5E:0<低23位>`）。所以即便开了精确过滤，
硬件层面仍可能递给你"不想要的组播"，最终仍要在用户态按 dst IP 过滤一遍。

---

## 收包循环要点

```c
uint16_t nb_rx = rte_eth_rx_burst(port, queue, bufs, BURST_SIZE);
for (i = 0; i < nb_rx; i++) {
    parse_packet(bufs[i]);      /* 自己解析以太网帧，边界检查必须自己做 */
    rte_pktmbuf_free(bufs[i]);  /* ★ 必须归还，否则 mbuf 池耗尽 → rx_nombuf */
}
```

| 要点 | 说明 |
|------|------|
| 返回值 | 实际包数，可能 < BURST_SIZE，也可能是 0（正常轮询） |
| mbuf 归还 | 忘记 free 是新手第一大坑，表现为跑一会儿后 `rx_nombuf` 暴涨 |
| 批处理大小 | `BURST_SIZE` 大 → 吞吐高，但批次内后包队头等待长，**尾延迟变差** |
| 丢包监控 | `imissed` = 网卡收不进来；`rx_nombuf` = mbuf 池耗尽 |
| 时间戳 | 用 `rte_rdtsc()`，**不要**用 `gettimeofday()`（系统调用，~μs 级） |

批处理 vs 尾延迟的取舍可以在 `mcast-minimal` 里实测：
改 `BURST_SIZE`（对照组的 `-v`），观察 `hist_burst` 的 p999 怎么变。

---

## 与内核栈逐项对照

| 维度 | 内核栈（socket） | DPDK 旁路 |
|------|-----------------|-----------|
| IGMP | 内核自动发 | **自己处理** |
| 组播复制 | 多 socket 各一份（每份一次 skb 复制） | 无，用户态自己分发 |
| 数据拷贝 | 内核 → 用户态 1 次 | **0 次**（DMA 直写 mbuf） |
| 系统调用 | 每批 1 次 `recvmmsg` | **0 次** |
| 协议解析 | 内核做 IP/UDP/校验和 | **自己解析** |
| 边界检查 | 内核保证 | **自己保证**（裸帧可能比头还短） |
| 网卡占用 | 与内核共存 | **独占**，SSH 要留另一张卡 |
| 典型延迟量级 | 数 μs | 亚 μs |

---

## 常见坑清单

1. **一个包都收不到** → 先查 IGMP / 交换机静态组播，别急着改代码
2. **`rx_nombuf` 涨** → mbuf 没 free，或用户态消费太慢
3. **解析时崩 / 读到垃圾** → 没做 `m->data_len` 边界检查，裸帧可能比协议头短
4. **流量跑到别的队列去了** → RSS hash 把组播流分到非预期队列，用 ntuple 钉死
5. **SSH 断了** → DPDK 把网卡接管了，管理流量必须走另一张网卡
6. **延迟数据不可信** → 用了 `gettimeofday()`，或没扣 `rdtsc` 基线，
   或只看了均值没看 p999 → [延迟测量方法](../../../12.5-modern-networking/chapter-15-debugging-perf-tuning/notes/03-latency-measurement.md)

---

## 相关章节

- 上一章：[chapter-04-零拷贝与用户态旁路.md](./chapter-04-零拷贝与用户态旁路.md)
- 下一梯度：[02-Advanced note-openonload-rdma对比](../../02-Advanced-Book/notes/note-openonload-rdma对比.md)
- 内核栈组播路径：[12.5/chapter-02/notes/05-multicast-rx-path](../../../12.5-modern-networking/chapter-02-napi-rx-path/notes/05-multicast-rx-path.md)
- 队列定向：[12.5/chapter-02/notes/07-queue-steering-rss](../../../12.5-modern-networking/chapter-02-napi-rx-path/notes/07-queue-steering-rss.md)
- 延迟测量：[12.5/chapter-15/notes/03-latency-measurement](../../../12.5-modern-networking/chapter-15-debugging-perf-tuning/notes/03-latency-measurement.md)
- 协议：[11-tcpip-protocols](../../../11-tcpip-protocols/) · 内核组播：[12-kernel-networking/note-组播IGMP](../../../12-kernel-networking/note-组播IGMP.md)
- 实验：[code/mcast-minimal/](../code/mcast-minimal/)
