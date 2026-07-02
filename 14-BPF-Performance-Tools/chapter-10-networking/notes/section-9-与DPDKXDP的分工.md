# 9. 与 DPDK / XDP 的分工

| 路径 | 观测手段 |
|------|----------|
| **内核栈 TCP/UDP** | 本章 BCC 工具 |
| **XDP 早丢弃/转发** | [note-XDP与tc-BPF](../../note-XDP与tc-BPF.md) |
| **DPDK 用户态** | PMD stats、`testpmd`、应用计数 — [12-DPDK](../12-DPDK-Low-Latency-Network/) |

**勿混读：** DPDK 口上 **`tcpretrans` 可能无事件** — 工具针对内核 TCP 栈。

---
