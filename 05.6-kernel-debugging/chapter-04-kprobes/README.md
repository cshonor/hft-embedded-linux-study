# Ch4 Debug via Instrumentation - Kprobes

> Part 2: Instrumentation & Memory Debugging · 🔴 精读

Kprobes 框架：kprobe (入口探针) / kretprobe (返回探针) / jprobe (已弃用)；静态注册 vs 动态注册；perf probe 和 bpftrace 的底层。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 4.1 Kprobes 原理与架构 | `notes/section-4-1.md` |
| 4.2 kprobe：函数入口探针 | `notes/section-4-2.md` |
| 4.3 kretprobe：函数返回探针 | `notes/section-4-3.md` |
| 4.4 动态注册 Kprobes (通过 /sys) | `notes/section-4-4.md` |
| 4.5 perf probe 与 Kprobes 的关系 | `notes/section-4-5.md` |
| 4.6 Kprobes 与 eBPF 的关系 | `notes/section-4-6.md` |

---

## HFT 关联

精读。Kprobes 是 HFT 延迟溯源的核心工具之一，可在生产环境动态插入探针测量内核函数耗时。同时也是 eBPF tracing 的底层机制。
