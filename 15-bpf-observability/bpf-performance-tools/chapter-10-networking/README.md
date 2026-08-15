# Ch 10 网络 · Networking

> **BPF Performance Tools** · Brendan Gregg · **精读 🔴** · 印刷 p411–530

> 本章定位：**全书 Part II 最厚的一章** — Linux 网络栈全路径 + **31 个 BPF 工具**（表 10-3）。eBPF 源于包过滤；相对 `tcpdump`，BPF 能把 **包/连接事件 ↔ PID ↔ 调用栈** 绑在一起。
> **HFT：** 共置机 **内核网络栈** 仍是行情/风控/日志的主战场之一（未全量 DPDK 时）；**`tcpretrans`、`tcpconnect`、`tcplife`、`gethostlatency`** 是 Ch 3 runbook 核心。旁路路径见 [note-XDP与tc-BPF](../note-XDP与tc-BPF.md) · [15-DPDK](../../../13-dpdk/)。
> **上一章：** [chapter-09-磁盘IO.md](../chapter-09-disk-io/) · **下一章：** [chapter-11-安全.md](../chapter-11-security/)

---

## 小节笔记（按原书真实小节）

| 原书小节 | 笔记 | 内容 |
|----|------|------|
| 10.1 背景 | [section-1-背景知识](./notes/section-1-背景知识.md) | 图 10-1 网络栈 · DPDK vs XDP · sk_buff/sock/proto · RSS/RPS/RFS/XPS/SO_REUSEPORT · SYN 双队列 · RTO/SACK · GSO/TSO/GRO · 拥塞算法 · Nagle/BQL/TSQ/EDT · 延迟七指标 · 十步策略 · 三大跟踪陷阱 |
| 10.2 传统工具 | [section-2-传统工具](./notes/section-2-传统工具.md) | ss -tiepm · ip -s link · nstat 重置陷阱 · netstat · sar -n 组合 · nicstat %util · ethtool -S/-i/-k/-K · tcpdump 四盲区 · /proc |
| 10.3.1–8 套接字层 | [section-3-BPF工具-套接字层](./notes/section-3-BPF工具-套接字层.md) | sockstat · sofamily · soprotocol · soconnect · soaccept · socketio · socksize · sormem |
| 10.3.9–13 连接与生命周期 | [section-4-BPF工具-TCP连接与生命周期](./notes/section-4-BPF工具-TCP连接与生命周期.md) | soconnlat · solstbyte · tcpconnect(-tp) · tcpaccept(-tp) · tcplife |
| 10.3.14–19 流量与重传 | [section-5-BPF工具-TCP流量与重传](./notes/section-5-BPF工具-TCP流量与重传.md) | tcptop · tcpsnoop · tcpretrans · tcpsynbl · tcpwin · tcpnagle |
| 10.3.20–23, 31 UDP/DNS/特殊 | [section-6-BPF工具-UDP-DNS与特殊协议](./notes/section-6-BPF工具-UDP-DNS与特殊协议.md) | udpconnect · gethostlatency · ipecn · superping · solisten/tcpstates/tcpdrop/sofdsnoop 等 |
| 10.3.24–25 qdisc | [section-7-BPF工具-qdisc队列](./notes/section-7-BPF工具-qdisc队列.md) | qdisc-fq · qdisc-cbq/cbs/codel/fq_codel/red/tbf（Qdisc_ops 模板法） |
| 10.3.26–30 设备层与 skb | [section-8-BPF工具-设备层与skb](./notes/section-8-BPF工具-设备层与skb.md) | netsize · nettxlat · skbdrop · skblife · ieee80211scan |
| 10.4 单行 | [section-9-BPF单行程序](./notes/section-9-BPF单行程序.md) | connect 失败/ustack · 字节直方图 · net_dev_xmit 全路径 kstack · 驱动跟踪点 |
| 10.5 练习 | [section-10-可选练习](./notes/section-10-可选练习.md) | 13 题（9–13 进阶未解决） |
| 10.6 小结 | [section-11-小结](./notes/section-11-小结.md) | 事件式跟踪原则 · 三板斧 · 章间衔接 |

---

## 本章 Checklist

- [ ] **Ch 10 是共置机网络 incident 主章**— 与 Ch 6 CPU 并列精读。
- [ ] **网络三板斧：tcpretrans / tcpconnect -t / tcplife**— 低开销常驻候选（仍须限流）。
- [ ] **延迟七指标对号入座**— DNS(gethostlatency) / 建连(soconnlat) / TTFB(solstbyte) / RTT(tcpwin) / 时长(tcplife) / 丢包(tcpdrop+skbdrop)。
- [ ] **`tcpdump` 不能替代 BPF**— 无 PID/栈/内核状态；高 pps 下抓包本身可能改变行为。
- [ ] **nstat 默认重置**— 前后对比必须 `-S`。
- [ ] **Nagle/offload 核对**— `tcpnagle` 先测再关；`ethtool -k` 看 TSO/GSO/GRO。
- [ ] **热路径已 DPDK 化的部分本章工具失效**— 数据面用 14-DPDK + 应用指标；XDP 仍可观测。

---

## 相关章节

- 上一章：[chapter-09-磁盘IO.md](../chapter-09-disk-io/)
- 下一章：[chapter-11-安全.md](../chapter-11-security/)
- XDP 延伸：[note-XDP与tc-BPF.md](../note-XDP与tc-BPF.md)
- 检查清单：[chapter-03-性能分析.md](../chapter-03-performance-analysis/)
- SysPerf 网络：[chapter-10-network](../../../14-systems-performance/chapter-10-network/)
- DPDK：[13-dpdk](../../../13-dpdk/)
- CSAPP 网络：[chapter-11-network-programming](../../../02-computer-systems/chapter-11-network-programming/)
