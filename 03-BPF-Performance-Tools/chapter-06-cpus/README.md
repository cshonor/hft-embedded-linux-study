# Ch 6 CPU · CPUs

> **BPF Performance Tools** · Brendan Gregg · **精读 🔴**

> 本章定位：**Part II 开篇** — CPU 执行所有代码，通常是性能分析的 **第一个切入点**。回顾 CPU 模式/调度/缓存基础与传统工具后，重点介绍 **CPU 与调度器相关的 BCC/bpftrace 工具**。  
> **HFT：** 共置交易机上 **绑核 + 专用核** 场景下，`runqlat` 应接近 0；若 P99 抖动却 `top` 不忙，用 **`offcputime`** 找阻塞栈、**`profile`** 找在核热点 — 与 [Ch 3 清单](../chapter-03-performance-analysis/) 直接衔接。  
> **上一章：** [chapter-05-bpftrace.md](../chapter-05-bpftrace/) · **下一章：** [chapter-07-内存.md](../chapter-07-memory/)

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1 本章要回答的两个问题 | [notes/section-1-本章要回答的两个问题.md](./notes/section-1-本章要回答的两个问题.md) |
| 2 CPU 基础知识 (Background) | [notes/section-2-CPU基础知识.md](./notes/section-2-CPU基础知识.md) |
| 3 传统 CPU 分析工具 | [notes/section-3-传统CPU分析工具.md](./notes/section-3-传统CPU分析工具.md) |
| 4 BPF 相对传统工具的优势 | [notes/section-4-BPF相对传统工具的优势.md](./notes/section-4-BPF相对传统工具的优势.md) |
| 5 进程与线程生命周期 | [notes/section-5-进程与线程生命周期.md](./notes/section-5-进程与线程生命周期.md) |
| 6 调度器与饱和度 | [notes/section-6-调度器与饱和度.md](./notes/section-6-调度器与饱和度.md) |
| 7 CPU 使用时间与剖析 (On-CPU) | [notes/section-7-CPU使用时间与剖析.md](./notes/section-7-CPU使用时间与剖析.md) |
| 8 Off-CPU 时间 — `offcputime` 🔴 | [notes/section-8-Off-CPU时间offcputime.md](./notes/section-8-Off-CPU时间offcputime.md) |
| 9 中断与其他 | [notes/section-9-中断与其他.md](./notes/section-9-中断与其他.md) |
| 10 BPF 单行命令 (One-Liners) | [notes/section-10-BPF单行命令.md](./notes/section-10-BPF单行命令.md) |
| 11 工具选型速查 | [notes/section-11-工具选型速查.md](./notes/section-11-工具选型速查.md) |

---

## 大白话

> Part II 开篇

下面按原书小节展开；细节见 **小节笔记** 表。

---

## 本章 Checklist

- [ ] **两个核心问题：**在核忙什么（`profile`）vs 为什么拿不到核（`runqlat`）vs 不跑时在等什么（`offcputime`）。
- [ ] **`runqlat` 是绑核健康度体温计**— dedicated 策略核右尾应极短；与 [SysPerf Ch6](../../02-Systems-Performance-2nd/chapter-06-cpus/) 一致。
- [ ] **火焰图频率 99Hz**，避免与 tick 对齐；热路径 profile 加 `-p` 降噪。
- [ ] **Off-CPU 与 On-CPU 成对使用**— 只 profile 会漏掉「等锁/I/O」型延迟。
- [ ] **`cpufreq` / governor**— 生产前 checklist，别在压测机以外的地方抄配置。

---

## 相关章节

- 上一章：[chapter-05-bpftrace.md](../chapter-05-bpftrace/)
- 下一章：[chapter-07-内存.md](../chapter-07-memory/)
- 检查清单：[chapter-03-性能分析.md](../chapter-03-performance-analysis/)
- BCC 工具箱：[chapter-04-BCC.md](../chapter-04-bcc/)
- SysPerf CPU：[chapter-06-cpus](../../02-Systems-Performance-2nd/chapter-06-cpus/)
- SysPerf BPF 总览：[chapter-15-bpf](../../02-Systems-Performance-2nd/chapter-15-bpf/)
- 体系结构/cache：[04-Hennessy](../04-Computer-Architecture-6th/) · [01-CSAPP Ch6](../01-CSAPP-3rd/chapter-06-memory-hierarchy/)
