# Ch 7 内存 · Memory

> **BPF Performance Tools** · Brendan Gregg · **精读 🔴**

> 本章定位：**内存压力与分配路径** — CPU 扩展快于 DRAM 的时代，**内存 I/O** 常是隐性瓶颈。7.1 虚拟/物理内存、缺页与回收机制；7.2 传统工具；7.3 详解 **11 个 BPF 工具**（oomkill/memleak/mmapsnoop/brkstack/shmsnoop/faults/ffaults/vmscan/drsnoop/swapin/hfaults）；7.4 单行、7.5 练习、7.6 小结。  
> **HFT：** 热路径应 **预分配 + 池化 + mlockall + 巨页**，正常交易时缺页/回收计数应趋零；本章工具主要用于 **共置机内存争抢、泄漏、OOM、直接回收卡顿** 等 incident。与 [Ch 6](../chapter-06-cpus/) `llcstat` / cache 衔接。  
> **上一章：** [chapter-06-CPU.md](../chapter-06-cpus/) · **下一章：** [chapter-08-文件系统.md](../chapter-08-file-systems/)

---

## 小节笔记

| 节 | 主题 | 笔记 |
|----|------|------|
| 7.1 | 背景知识（内存基础 / BPF 分析能力 / 分析策略） | [notes/section-1-背景知识.md](./notes/section-1-背景知识.md) |
| 7.2 | 传统工具（内核日志 / 内核统计 / 硬件统计和采样） | [notes/section-2-传统工具.md](./notes/section-2-传统工具.md) |
| 7.3.1 | BPF 工具：oomkill | [notes/section-3-BPF工具-oomkill.md](./notes/section-3-BPF工具-oomkill.md) |
| 7.3.2 | BPF 工具：memleak | [notes/section-4-BPF工具-memleak.md](./notes/section-4-BPF工具-memleak.md) |
| 7.3.3–5 | BPF 工具：mmapsnoop / brkstack / shmsnoop | [notes/section-5-BPF工具-内存映射与堆.md](./notes/section-5-BPF工具-内存映射与堆.md) |
| 7.3.6–7,11 | BPF 工具：faults / ffaults / hfaults | [notes/section-6-BPF工具-缺页错误.md](./notes/section-6-BPF工具-缺页错误.md) |
| 7.3.8–10 | BPF 工具：vmscan / drsnoop / swapin | [notes/section-7-BPF工具-内存回收.md](./notes/section-7-BPF工具-内存回收.md) |
| 7.3.12 | BPF 工具：其他工具（llcstat/profile 复用） | [notes/section-8-BPF工具-其他工具.md](./notes/section-8-BPF工具-其他工具.md) |
| 7.4 | BPF 单行程序（BCC / bpftrace） | [notes/section-9-BPF单行程序.md](./notes/section-9-BPF单行程序.md) |
| 7.5 | 可选练习（10 题） | [notes/section-10-可选练习.md](./notes/section-10-可选练习.md) |
| 7.6 | 小结 | [notes/section-11-小结.md](./notes/section-11-小结.md) |

---

## 大白话

内存分析两条主线：

1. **用量为什么涨**：`faults`（缺页栈，RSS 增长直接原因）→ `brkstack`（堆扩展）→ `memleak`（未释放分配，测试环境）→ `ffaults`（缺页来自哪些文件）
2. **压力为什么卡**：`vmscan`（回收耗时，盯 **D-RECLAIM 直接回收**）→ `drsnoop`（卡了谁多久）→ `swapin`（换入受害者）→ `oomkill`（最后防线，常驻）

**开销纪律**：缺页/换页/brk/vmscan/OOM 都是低频事件，跟踪≈0 开销可常驻；malloc 每秒百万次，memleak 是调试工具（可掉到 1/10 速度），分配画像用 profile 采样替代。

**关键认知**：RSS 只在缺页时增长（faults 比 malloc 跟踪更贴近 RSS）；换入才直接伤应用（换出/扫描是间接信号）；直接回收 = 前台阻塞分配（无 swap 交易机的头号内存风险）。

---

## 本章 Checklist

- [ ] **热路径设计**应让 Ch 7 工具在常态下 **几乎无事可做** — 池化、预分配、禁 swap、mlockall、巨页（hfaults 验证）。
- [ ] **`oomkill` 常驻**：找 **触发者**（Triggered by）而非只看被杀者；关键进程 oom_score_adj = -1000。
- [ ] **`vmscan` 盯 D-RECLAIM**（>0 即告警）→ `drsnoop -T` 定位受害者与延迟 — Direct reclaim 是「内存够但抖」的常见根因。
- [ ] **`memleak` 是 incident/测试工具**— 加 `-S` 采样、限 PID，勿与低延迟核长期共存。
- [ ] **缺页火焰图 (`faults`)**用于冷启动、新库上线 — 稳态交易进程缺页计数应趋零（`software:page-fault:1` 单行常驻监控）。
- [ ] **free 看 available 不看 free 列**；vmstat 第一行是开机均值。

---

## 相关章节

- 上一章：[chapter-06-CPU.md](../chapter-06-cpus/)
- 下一章：[chapter-08-文件系统.md](../chapter-08-file-systems/)
- 磁盘 I/O：[chapter-09-磁盘IO.md](../chapter-09-disk-io/)
- 内核内存工具：[chapter-14-kernel](../chapter-14-kernel/)
- 方法论：[chapter-03-性能分析.md](../chapter-03-performance-analysis/)
- SysPerf 内存：[chapter-07-memory](../../../06.6-systems-performance/chapter-07-memory/)
- CSAPP 虚拟内存：[chapter-09-virtual-memory](../../../02-computer-systems/chapter-09-virtual-memory/)
- MM 理论：[06-linux-mm](../../../06-linux-mm/)
