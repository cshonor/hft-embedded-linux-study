# Ch 17 其他 BPF 性能工具 · Other BPF Performance Tools

> **BPF Performance Tools** · Brendan Gregg · **选读 🟡**

> 本章定位：**BPF 之上的可观测性生态** — Ch 4–16 以 **BCC/bpftrace CLI** 为主；本章介绍 **Vector/PCP、Grafana、Prometheus、kubectl-trace** 及 Cilium/Sysdig 等，把 BPF 指标 **GUI 化、持久化、集群化**。  
> **HFT：** **NOC/运维大屏** 可用 **热力图** 看 `runqlat`/`biolatency` 长尾；**交易机本身** 仍 **短窗口 CLI** 为主，长期 exporter 需 **限指标、限频率**。K8s 辅助服务用 **kubectl-trace**。  
> **上一章：** [chapter-16-虚拟机管理程序.md](../chapter-16-hypervisors/) · **下一章：** [chapter-18-技巧与常见问题.md](../chapter-18-tips-and-tricks/)

---

## 原书真实小节 → 笔记映射

| 原书小节 | 笔记 |
|----|------|
| 章首：从 CLI 到 GUI（云规模必须 GUI） | [notes/section-1-从CLI到平台.md](./notes/section-1-从CLI到平台.md) |
| 17.1 Vector 和 Performance Co-Pilot（PCP）（17.1.1–17.1.10） | [notes/section-2-Vector与PerformanceCo-Pilot.md](./notes/section-2-Vector与PerformanceCo-Pilot.md) |
| 17.2 Grafana 和 Performance Co-Pilot（17.2.1–17.2.4） | [notes/section-3-Grafana与PCP.md](./notes/section-3-Grafana与PCP.md) |
| 17.3 Cloudflare eBPF Prometheus Exporter（17.3.1–17.3.4） | [notes/section-4-CloudflareeBPFPrometheus导出器.md](./notes/section-4-CloudflareeBPFPrometheus导出器.md) |
| 17.4 kubectl-trace（17.4.1–17.4.3） | [notes/section-5-kubectl-trace.md](./notes/section-5-kubectl-trace.md) |
| 17.5 其他工具（Cilium/Sysdig/Android eBPF/osquery/ply） | [notes/section-6-其他工具.md](./notes/section-6-其他工具.md) |
| 17.6 小结 | [notes/section-7-小结.md](./notes/section-7-小结.md) |

---

## 大白话

> BPF 之上的可观测性生态

下面按原书小节展开；细节见 **小节笔记** 表。

---

## 本章 Checklist

- [ ] **CLI 仍是 incident 第一选择**— 平台层 **不能替代** `runqlat`/`tcpretrans` 手跑。
- [ ] **热力图价值**— `runqlat`/`biolatency` **长尾随时间** — 适合 **开盘/roll** 窗口回顾，非 tick 核常驻。
- [ ] **Prometheus/Grafana**— 存 **少量 SLI**（重传率、runq P99、steal%）；避免 **全量 biosnoop** 式 exporter。
- [ ] **ebpf_exporter**— 与现有 **K8s 监控栈** 统一；Cloudflare 实践可借鉴 metric 设计。
- [ ] **kubectl-trace**— 仅 **容器化辅助服务**；裸金属 SSH + bcc 更简单。
- [ ] **Cilium/Sysdig**— 了解即可；**14-DPDK** 热路径不在此栈。

---

## 相关章节

- 上一章：[chapter-16-虚拟机管理程序.md](../chapter-16-hypervisors/)
- 下一章：[chapter-18-技巧与常见问题.md](../chapter-18-tips-and-tricks/)
- BCC 工具源：[chapter-04-BCC.md](../chapter-04-bcc/)
- bpftrace：[chapter-05-bpftrace.md](../chapter-05-bpftrace/)
- 容器/K8s：[chapter-15-容器.md](../chapter-15-containers/)
- SysPerf 监控方法论：[chapter-02-methodologies](../../../06.6-systems-performance/chapter-02-methodologies/)
- HFT 工程监控：[chapter-09-latency-measurement-benchmarking](../../../14-hft-engineering/chapter-09-latency-measurement-benchmarking/README.md)
