# 9. 与 DPDK / XDP 的分工

| 路径 | 观测手段 |
|------|----------|
| **内核栈 TCP/UDP** | 本章 BCC 工具 |
| **XDP 早丢弃/转发** | [note-XDP与tc-BPF](../../note-XDP与tc-BPF.md) |
| **DPDK 用户态** | PMD stats、`testpmd`、应用计数 — [15-DPDK](../15-dpdk/) |

**勿混读：** DPDK 口上 **`tcpretrans` 可能无事件** — 工具针对内核 TCP 栈。


### 常见陷阱

1. **在 DPDK 路径上用内核 BPF 工具** — DPDK 绕过内核网络栈，内核 BPF 工具（tcpretrans/tcpconnlat 等）看不到 DPDK 流量；需用 DPDK 自身统计
2. **混淆 XDP 和 DPDK 的定位** — XDP 在内核网卡驱动层做早期包处理（仍经过内核），DPDK 完全绕过内核（用户态驱动）；两者观测方式不同
3. **以为 XDP 能完全替代 DPDK** — XDP 适合包过滤/转发/丢弃，不适合完整的协议栈处理；HFT 如果需要用户态 TCP 栈仍需 DPDK

<details>
<summary>📝 自测题（点击展开）</summary>

1. **内核网络栈、XDP、DPDK 三条路径的观测手段分别是什么？**

   <details>
   <summary>参考答案</summary>

   (1) 内核网络栈 TCP/UDP：本章 BCC 工具（tcpretrans/tcpconnlat 等）+ ethtool；(2) XDP 早丢弃/转发：XDP 程序自带统计 + `bpftool prog show` + xdpdump；(3) DPDK 用户态：PMD 统计（testpmd show port stats）、应用层计数器、rte_eth_stats。关键：DPDK 口上 tcpretrans 可能无事件——工具针对内核 TCP 栈。

   </details>

2. **为什么不能在 DPDK 路径上用内核 BPF 工具？**

   <details>
   <summary>参考答案</summary>

   DPDK 使用用户态网卡驱动（PMD），完全绕过内核网络栈——包从网卡 DMA 到用户态内存，不经 tcp_sendmsg/dev_queue_xmit 等内核函数。因此 tcpretrans、tcpconnlat、socketsnoop 等基于内核 probe 的工具看不到 DPDK 流量。需用 DPDK 自身统计接口（rte_eth_stats_get）或应用层计数器。

   </details>

3. **XDP 和 DPDK 的定位有什么区别？HFT 如何选择？**

   <details>
   <summary>参考答案</summary>

   XDP：在内核网卡驱动层做早期包处理（仍经内核），适合包过滤/转发/丢弃，开销低于 DPDK 但灵活性受限。DPDK：完全绕过内核（用户态驱动），适合完整协议栈处理和极低延迟，但占用专用 CPU 核。HFT 选择：(1) 内核栈 + XDP 做早过滤——延迟可接受（微秒级）时优先；(2) DPDK——需要纳秒级延迟或自定义协议栈时使用；(3) 混合——XDP 做粗过滤，DPDK 做精细处理。

   </details>

</details>

---
