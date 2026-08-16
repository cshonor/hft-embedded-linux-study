# Ch 6 CPU · CPUs

> **BPF Performance Tools** · Brendan Gregg · **精读 🔴**

> 本章定位：**Part II 开篇** — CPU 执行所有代码，通常是性能分析的 **第一个切入点**。6.1–6.2 回顾 CPU 模式/调度/缓存基础与传统工具，6.3 逐一详解 **17 个 BCC/bpftrace CPU 工具**，6.4 单行程序、6.5 练习、6.6 小结。  
> **HFT：** 共置交易机上 **绑核 + 专用核** 场景下，`runqlat` 应接近 0；若 P99 抖动却 `top` 不忙，用 **`offcputime`** 找阻塞栈、**`profile`** 找在核热点 — 与 [Ch 3 清单](../chapter-03-performance-analysis/) 直接衔接。  
> **上一章：** [chapter-05-bpftrace.md](../chapter-05-bpftrace/) · **下一章：** [chapter-07-内存.md](../chapter-07-memory/)

---

## 小节笔记

| 节 | 主题 | 笔记 |
|----|------|------|
| 6.1 | 背景知识（CPU 基础 / BPF 分析能力 / 分析策略） | [notes/section-1-背景知识.md](./notes/section-1-背景知识.md) |
| 6.2 | 传统工具（内核统计 / 硬件统计 / 硬件采样 / 定时采样 / 事件统计与跟踪） | [notes/section-2-传统工具.md](./notes/section-2-传统工具.md) |
| 6.3.1–2 | BPF 工具：execsnoop / exitsnoop | [notes/section-3-BPF工具-进程生命周期.md](./notes/section-3-BPF工具-进程生命周期.md) |
| 6.3.3–5 | BPF 工具：runqlat / runqlen / runqslower | [notes/section-4-BPF工具-运行队列延迟.md](./notes/section-4-BPF工具-运行队列延迟.md) |
| 6.3.6–7 | BPF 工具：cpudist / cpufreq | [notes/section-5-BPF工具-CPU执行时长.md](./notes/section-5-BPF工具-CPU执行时长.md) |
| 6.3.8–9 | BPF 工具：profile / offcputime | [notes/section-6-BPF工具-剖析profile与offcputime.md](./notes/section-6-BPF工具-剖析profile与offcputime.md) |
| 6.3.10–11 | BPF 工具：syscount / argdist+trace | [notes/section-7-BPF工具-系统调用.md](./notes/section-7-BPF工具-系统调用.md) |
| 6.3.12–15 | BPF 工具：funccount / softirqs / hardirqs / smpcalls | [notes/section-8-BPF工具-函数与中断.md](./notes/section-8-BPF工具-函数与中断.md) |
| 6.3.16–17 | BPF 工具：llcstat / 其他工具 | [notes/section-9-BPF工具-缓存与其他.md](./notes/section-9-BPF工具-缓存与其他.md) |
| 6.4 | BPF 单行程序（BCC 版 / bpftrace 版） | [notes/section-10-BPF单行程序.md](./notes/section-10-BPF单行程序.md) |
| 6.5 | 可选练习（13 题） | [notes/section-11-可选练习.md](./notes/section-11-可选练习.md) |
| 6.6 | 小结 | [notes/section-12-小结.md](./notes/section-12-小结.md) |

---

## 大白话

CPU 分析三板斧（对应三个核心问题）：

1. **在核上忙什么** → `profile` 采样 + 火焰图（开销≈0，可常驻）
2. **为什么拿不到核** → `runqlat` 直方图量化排队（事件跟踪，短期用；常驻监控用 `runqlen -O`）
3. **离核在等什么** → `offcputime` + 蓝底 off-CPU 火焰图（开销可能 >10%，窗口化使用）

再配外围：`execsnoop` 抓短命进程、`syscount`/`argdist` 下钻系统调用、`funccount` 判断"函数慢还是调用勤"、`softirqs`/`hardirqs`/`smpcalls` 量化中断与核间打断、`llcstat` 看缓存效率。

**开销纪律**：定时采样类（profile/runqlen/cpufreq/llcstat）≈ 零开销；事件跟踪类（runqlat/cpudist/offcputime/funccount）与事件频率成正比，繁忙生产系统短期运行 + `-p` 限定进程。

---

## 本章 Checklist

- [ ] **三个核心问题：**在核忙什么（`profile`）、为什么拿不到核（`runqlat`）、离核等什么（`offcputime`）— 三工具成对互补。
- [ ] **`runqlat` 是绑核健康度体温计**— dedicated 策略核右尾应极短；常驻监控用 `runqlen -O`（99Hz 采样零开销）。
- [ ] **火焰图频率 99Hz**（防与 100Hz 活动锁定步进）；热路径 profile 加 `-p` 降噪。
- [ ] **Off-CPU 与 On-CPU 成对使用**— 只 profile 会漏掉「等锁/I/O」型延迟；off-CPU 图蓝底区分。
- [ ] **`cpufreq` / governor**— 交易机必须 performance；cpufreq **无输出** = 频率恒定 = 正确状态。
- [ ] **中断落点审计**— `hardirqs` 验证网卡中断不落策略核；`smpcalls` 查 TLB shootdown / 监控读 /proc 引发的 IPI。
- [ ] **事件跟踪类工具绝不 7×24 常驻**（runqlat/offcputime/funccount 开销随频率线性涨）。
- [ ] **strace 是禁忌**（ptrace 掉 99% 性能）；系统调用分析一律 syscount/argdist/trace。

---

## 相关章节

- 上一章：[chapter-05-bpftrace.md](../chapter-05-bpftrace/)
- 下一章：[chapter-07-内存.md](../chapter-07-memory/)
- 检查清单：[chapter-03-性能分析.md](../chapter-03-performance-analysis/)
- BCC 工具箱：[chapter-04-BCC.md](../chapter-04-bcc/)
- SysPerf CPU：[chapter-06-cpus](../../../06.6-systems-performance/chapter-06-cpus/)
- SysPerf BPF 总览：[chapter-15-bpf](../../../06.6-systems-performance/chapter-15-bpf/)
- 体系结构/cache：[19-Hennessy](../../../17-computer-architecture/) · [02-CSAPP Ch6](../../../02-computer-systems/chapter-06-memory-hierarchy/)
