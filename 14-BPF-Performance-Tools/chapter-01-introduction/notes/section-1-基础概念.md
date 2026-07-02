# 1. 基础概念

| 术语 | 含义 |
|------|------|
| **BPF** | 经典 Berkeley Packet Filter — 最初用于 tcpdump 包过滤的 **内核字节码 VM** |
| **eBPF** | 扩展 BPF（2014+）— **通用、图灵完备、可验证** 的内核沙箱程序；本书「BPF」多指此 |
| **Tracing** | **事件追踪** — 每次事件发生记录一条（exec、open、syscall enter/exit） |
| **Snooping** | **嗅探** — 非修改地观察活动（opensnoop、execsnoop 一类） |
| **Sampling** | **采样** — 周期性快照（如 `profile` 按频率采栈）；低开销、可能漏短事件 |
| **Profiling** | **剖析** — 汇总「时间/次数花在哪」；常与采样栈或聚合 map 结合 |
| **Observability** | **可观测性** — 从外部输出推断内部状态；BPF 让 **内核 + 用户态** 同屏可见 |

> **HFT 直觉：** 延迟尖刺往往是 **短事件**（一次 block I/O、一次 run-queue 排队、一次 TCP 重传）— **Tracing/BCC 直方图** 补 **perf 采样** 的盲区；采样适合 CPU 热点，追踪适合「谁、何时、持续了多久」。

---
