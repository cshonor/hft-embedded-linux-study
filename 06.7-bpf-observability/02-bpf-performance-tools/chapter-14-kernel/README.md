# Ch 14 内核 · Kernel

> **BPF Performance Tools** · Brendan Gregg · **选读 🟡**（内核开发者 🔴）

> 本章定位：**内核本身作为分析目标** — Ch 6–13 借内核观测 **应用**；本章深入 **调度唤醒链、内核锁、Slab/页分配、工作队列**。对 **内核开发者** 极有用；HFT 共置机 **incident 深潜** 时用于「系统卡顿但应用说不清」类问题。
> **HFT：** 常态 **选读**；`offwaketime` 解阻塞链、`kmem`/`slabratetop` 查内核内存、`mlock` 查内核 mutex、`criticalstat` 抓禁 IRQ 临界区。优先 **tracepoint** 而非脆弱 kprobe。与 [05-linux-kernel](../../../05-linux-kernel/) · [06-linux-mm](../../../06-linux-mm/) 对照。
> **上一章：** [chapter-13-applications](../chapter-13-applications/) · **下一章：** [chapter-15-containers](../chapter-15-containers/)

---

## 小节笔记（按原书真实小节）

| 原书小节 | 笔记 | 覆盖工具 |
|----------|------|----------|
| 14.1 背景知识（内核基础 / BPF 能力） | [notes/section-1-背景知识.md](./notes/section-1-背景知识.md) | 唤醒链、slab/页分配器、互斥锁三路径、RCU、tasklet/workqueue、表14-1 事件源 |
| 14.2 分析策略 | [notes/section-2-分析策略.md](./notes/section-2-分析策略.md) | 九步策略 |
| 14.3 传统工具 | [notes/section-3-传统工具.md](./notes/section-3-传统工具.md) | Ftrace（funccount/kprobe/hist triggers/funcgraph）、perf sched、slabtop |
| 14.4.1–14.4.4 唤醒分析 | [notes/section-4-BPF工具-唤醒分析.md](./notes/section-4-BPF工具-唤醒分析.md) | loads、offcputime 深入（--state 2）、wakeuptime、offwaketime |
| 14.4.5–14.4.6 内核锁 | [notes/section-5-BPF工具-内核锁.md](./notes/section-5-BPF工具-内核锁.md) | mlock、mheld、自旋锁 |
| 14.4.7–14.4.11 内核内存 | [notes/section-6-BPF工具-内核内存.md](./notes/section-6-BPF工具-内核内存.md) | kmem、kpages、memleak、slabratetop、numamove |
| 14.4.12–14.4.14 workq 与 tasklet | [notes/section-7-BPF工具-workq与tasklet.md](./notes/section-7-BPF工具-workq与tasklet.md) | workq、小任务、inject、criticalstat |
| 14.5–14.7 单行程序与挑战 | [notes/section-8-BPF单行程序与挑战.md](./notes/section-8-BPF单行程序与挑战.md) | BCC/bpftrace 单行、syscall_table 反查、hrtimer、内联/黑名单/kprobe 三挑战 |
| 14.8 小结 | [notes/section-9-小结.md](./notes/section-9-小结.md) | 工具全景表 |

---

## 大白话

资源章看"内核替应用干了什么"，本章看"内核自己好不好"：唤醒链断在哪、哪个内核锁在排队、slab 谁吃得最多、NUMA 均衡是不是在帮倒忙。

## 本章 Checklist

- [ ] **`offwaketime`** — 比单独 `offcputime` 多 **唤醒者** 半条链；共置机 **I/O 完成路径** 排查利器。
- [ ] **内核内存** — `slabratetop`（流量）+ `slabtop`（存量）+ `kmem`（栈）；与用户态 `memleak`（Ch 7）分工。
- [ ] **`mlock` vs `pmlock`** — 内核 mutex vs **pthread**（Ch 13）；`syscount` 见 futex 时先 Ch 13。
- [ ] **自旋锁测不了时长** — kretprobe 被禁，改用 CPU 剖析/火焰图（自旋以耗 CPU 函数出现）。
- [ ] **`criticalstat`** — 禁 IRQ >100us 的内核临界区 = 抖动源。
- [ ] **优先跟踪点** — kprobe 怕内联、黑名单、内核版本变动。
