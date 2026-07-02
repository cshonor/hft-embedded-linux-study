# Ch 10 网络 · Networking

> **BPF Performance Tools** · Brendan Gregg · **精读 🔴**

> 本章定位：**全书 Part II 最厚的一章** — Linux 网络栈全路径 + **海量 BPF 工具**。eBPF 源于包过滤；相对 `tcpdump`，BPF 能把 **包/连接事件 ↔ PID ↔ 调用栈** 绑在一起。  
> **HFT：** 共置机 **内核网络栈** 仍是行情/风控/日志的主战场之一（未全量 DPDK 时）；**`tcpretrans`、`tcpconnect`、`tcplife`、`gethostlatency`** 是 Ch 3 runbook 核心。旁路路径见 [note-XDP与tc-BPF](./note-XDP与tc-BPF.md) · [12-DPDK](../12-DPDK-Low-Latency-Network/)。  
> **上一章：** [chapter-09-磁盘IO.md](../chapter-09-disk-io/) · **下一章：** [chapter-11-安全.md](../chapter-11-security/)

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1 本章要回答的问题 | [notes/section-1-本章要回答的问题.md](./notes/section-1-本章要回答的问题.md) |
| 2 网络基础知识 (Background) | [notes/section-2-网络基础知识.md](./notes/section-2-网络基础知识.md) |
| 3 传统网络分析工具 | [notes/section-3-传统网络分析工具.md](./notes/section-3-传统网络分析工具.md) |
| 4 套接字层 (Socket API) 工具 | [notes/section-4-套接字层工具.md](./notes/section-4-套接字层工具.md) |
| 5 TCP 协议层工具 | [notes/section-5-TCP协议层工具.md](./notes/section-5-TCP协议层工具.md) |
| 6 UDP、DNS 与其他 | [notes/section-6-UDPDNS与其他.md](./notes/section-6-UDPDNS与其他.md) |
| 7 底层：qdisc / skb / 驱动 | [notes/section-7-底层qdiscskb驱动.md](./notes/section-7-底层qdiscskb驱动.md) |
| 8 工具选型速查（HFT 优先） | [notes/section-8-工具选型速查HFT优先.md](./notes/section-8-工具选型速查HFT优先.md) |
| 9 与 DPDK / XDP 的分工 | [notes/section-9-与DPDKXDP的分工.md](./notes/section-9-与DPDKXDP的分工.md) |
| 10 BPF / bpftrace One-Liners（示意） | [notes/section-10-BPFbpftraceOne-Liners示意.md](./notes/section-10-BPFbpftraceOne-Liners示意.md) |

---

## 大白话

> 全书 Part II 最厚的一章

下面按原书小节展开；细节见 **小节笔记** 表。

---

## 本章 Checklist

- [ ] **Ch 10 是共置机网络 incident 主章**— 与 Ch 6 CPU 并列精读。
- [ ] **`tcpretrans` + `ss -ti`**— 重传是否发生、cwnd/RTT 是否异常；先 30s 短采。
- [ ] **`tcpdump` 不能替代 BPF**— 无 PID/栈；高 pps 下抓包本身可能 **改变** 行为。
- [ ] **`tcplife`**低开销看清 **谁和谁谈了多久、传了多少** — 适合长跑监控（仍须限流）。
- [ ] **`gethostlatency`**— 「偶发慢」先查 DNS，再 blame 网络。
- [ ] **热路径已 DPDK 化**— 本章工具看 **管理面/辅助 TCP**；数据面用 14-DPDK + 应用指标。
- [ ] **Nagle/offload**— `tcpnagle`、`ethtool -k` 与 socket 选项一并核对。

---

## 相关章节

- 上一章：[chapter-09-磁盘IO.md](../chapter-09-disk-io/)
- 下一章：[chapter-11-安全.md](../chapter-11-security/)
- XDP 延伸：[note-XDP与tc-BPF.md](./note-XDP与tc-BPF.md)
- 检查清单：[chapter-03-性能分析.md](../chapter-03-performance-analysis/)
- SysPerf 网络：[chapter-10-network](../../13-Systems-Performance-2nd/chapter-10-network/)
- DPDK：[12-DPDK-Low-Latency-Network](../12-DPDK-Low-Latency-Network/)
- CSAPP 网络：[chapter-11-network-programming](../01-CSAPP-3rd/chapter-11-network-programming/)
