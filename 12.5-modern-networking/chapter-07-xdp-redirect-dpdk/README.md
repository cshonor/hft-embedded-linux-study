# Chapter 07: XDP_REDIRECT 与 XDP vs DPDK

> 来源：LWN（XDP redirect + XDP vs DPDK 对比）+ kernel-docs
> 对标：Rosen（**无 XDP / 无 DPDK**，3.x 时代两者都不存在）
> 内核版本：以 **v6.6** 为准，机制与常量均取自源码
> （`net/core/filter.c`、`kernel/bpf/devmap.c`、`kernel/bpf/cpumap.c`、`include/net/xdp.h`）
>
> ⚠️ **路径提示**：v6.6 起 devmap 已从 `net/core/devmap.c` 移到 **`kernel/bpf/devmap.c`**，
> cpumap 在 **`kernel/bpf/cpumap.c`**。网上大量资料仍写旧路径。

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [xdp-redirect](notes/01-xdp-redirect.md) | **机制**：`XDP_REDIRECT` 的三步批量语义、四个目的地（devmap / cpumap / xskmap / ifindex）、批量队列大小、devmap 不重排队、cpumap 的 kthread 与 skb 分配、tracepoint 观测表 |
| 2 | [xdp-vs-dpdk](notes/02-xdp-vs-dpdk.md) | **选型**：以"谁拥有网卡"为轴的所有权对比、DPDK 的结构性优势在哪两跳、XDP 的工程优势、混合架构、10 条误区澄清、三段式延迟测量方法 |

## 本篇的五个核心结论

1. **`XDP_REDIRECT` 不是立即动作。** 程序返回后包还在 per-CPU 批量队列里，
   真正送走要等 `xdp_do_flush()`——它由驱动在 **NAPI poll 结束前**调用
   （`net/core/filter.c:4202`，且必须早于 `napi_complete_done()`，
   整个 redirect 靠"同处一个 NAPI poll"获得 RCU 保护）。
2. **批量大小：devmap 16、cpumap 8、AF_XDP 跟随 NAPI weight（默认 64）。**
   所以单包延迟取决于它在批次里的位置——谈 XDP 延迟要看分布（P50/P99），不是平均值。
3. **devmap 没有重排队。** `ndo_xdp_xmit()` 没发完的帧被
   `xdp_return_frame_rx_napi()` 直接释放（丢弃），**不产生 errno**，
   只在 `trace_xdp_devmap_xmit` 的 `drops` 参数里可见。
4. **cpumap 不省 `sk_buff`**（最常见的误解）。它在目标 CPU 上
   `kmem_cache_alloc_bulk()` 批量分配 skb，终点是 `netif_receive_skb_list()`。
   cpumap 优化的是**分配位置与分发策略**，而且 v6.6 用的是 **kthread**（有线程调度），
   所以它**不是低延迟机制**。
5. **XDP vs DPDK 的分野是"谁拥有网卡"，不是"谁更快"。**
   要跟内核共存 → XDP；要独占网卡换确定性延迟 → DPDK。

## HFT 关联

- **devmap 做内核态 L2 转发**：绕过 qdisc、绕过 tc egress、不构建 skb。
  但⚠️**没有重排队**，且**静默丢弃超过出口 MTU 的帧**
- **cpumap 按业务规则分核**：比 RPS 灵活（BPF 自定义策略 + 二级过滤），
  但有一次线程调度——**低延迟分核请用硬件 RSS / flow steering**
- **XSKMAP 是唯一跳过 `xdp_convert_buff_to_frame()` 的目的地**——
  包已经在 UMEM 里，不需要搬。这是零拷贝路径最短的结构性原因
- **混合架构是常态**：行情接收旁路、下单走内核、管理走内核，
  用不同网卡/不同队列分开，别追求"全线旁路"
- **⚠️ 留一个队列给内核栈**：全旁路后内核收不到 IGMP，
  组播成员关系会超时被交换机剪掉，行情断流
- **性能数字必须自己测**：包长、驱动、IOMMU、CPU 型号任一不同结论都可能反转。
  延迟只有硬件时间戳测出来的才可信，且要有"内核 socket + busy poll"对照基线

## 交叉引用

- `12.5-modern-networking/chapter-05-xdp-architecture/`：XDP 基础架构与五个动作
- `12.5-modern-networking/chapter-06-af-xdp/`：XSKMAP 目的地，AF_XDP 完整机制
- `12.5-modern-networking/chapter-03-tx-path-skbbuff/notes/04-sk-buff-xdp-buff.md`：
  `xdp_convert_buff_to_frame()` 为什么必须要有 headroom
- `12.5-modern-networking/chapter-04-page-pool/`：驱动侧缓冲区池
  （⚠️ AF_XDP 零拷贝不用它）
- `12.5-modern-networking/chapter-15-debugging-perf-tuning/notes/03-latency-measurement.md`：
  延迟测量方法
- `13-dpdk/`：DPDK 详细内容
