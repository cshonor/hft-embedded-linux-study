# Ch 5 应用程序 · Applications

> **Systems Performance 2nd** · Brendan Gregg · **精读**

> 本章定位：**性能调优的主战场在应用层** — 底层系统调优往往只有百分比级收益，而应用层（算法、数据结构、并发、I/O 模式）可以带来**数量级**提升。Ch 2 给了方法论与延迟分解；Ch 4 给了工具地图；本章讲**应用该长什么样、怎么写快、怎么剖**。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 5.1 应用程序基础 | [notes/section-5.1-应用程序基础.md](./notes/section-5.1-应用程序基础.md) |
| 5.2 应用程序性能提升技术 | [notes/section-5.2-应用程序性能提升技术.md](./notes/section-5.2-应用程序性能提升技术.md) |
| 5.3 编程语言与垃圾回收 | [notes/section-5.3-编程语言与垃圾回收.md](./notes/section-5.3-编程语言与垃圾回收.md) |
| 5.4 性能分析方法论 | [notes/section-5.4-性能分析方法论.md](./notes/section-5.4-性能分析方法论.md) |
| 5.5 观测工具 | [notes/section-5.5-观测工具.md](./notes/section-5.5-观测工具.md) |
| 5.6 常见陷阱（Gotchas） | [notes/section-5.6-常见陷阱Gotchas.md](./notes/section-5.6-常见陷阱Gotchas.md) |

---

## 大白话 · 本章就五件事

> **离实际干活最近的地方，最值得动刀。**

**① 别盲调 — 先有目标，再优化常见路径。**

- 明确指标：延迟 P99、吞吐、资源利用率、成本；可用 **Apdex** 把「满意 / 可容忍 / 受挫」量化。
- 生产负载里**最常走**的代码路径（CPU 上或等 I/O）优先优化；冷门分支再漂亮也救不了整体。

**② 应用层技术栈：I/O、缓存、并发、绑核。**

- 摊销 syscall：合适 I/O 块大小；**缓存**减读、**缓冲**（环形队列等）合并写。
- 别忙轮询占满 CPU → **epoll / kqueue** 事件驱动；并发用多线程 + 细粒度锁；警惕 **伪共享**。
- 非阻塞 I/O + **CPU 亲和性** = HFT 标配组合。

**③ 语言与 GC 是性能的一部分。**

- C/C++ 编译优化 vs Java JIT vs 解释型 — 选型和编译参数都影响延迟。
- **GC** 可能带来内存膨胀、CPU 开销、**Stop-the-world** 长尾 — 少分配、调 GC 是 Java 量化系统的必修课。

**④ 剖应用：Gregg 首选「线程状态分析」。**

- 把线程时间拆成 **9 种状态**（User / Kernel / Runnable / Swapping / Disk I/O / Net I/O / Sleeping / Lock / Idle）。
- **CPU 火焰图** 找算力热点；**Off-CPU 火焰图** 找阻塞（I/O、锁、调度）；两者合起来才是全貌。

**⑤ 工具与陷阱：perf、BPF、strace — 但符号和栈可能丢。**

- `profile`、`offcputime`、`syscount` 等 BPF 工具是应用剖析利器。
- **Missing Symbols / Missing Stacks** 会让火焰图一片 `[unknown]` — 编译时留符号、留帧指针，Java 用 `perf-map-agent`。

下面按原书 5.1–5.6 展开。

---

## 本章 Checklist

- [ ] 能说清 **应用层 vs 系统层** 优化的数量级差异
- [ ] 对 hot path 做过 **常见路径** 识别（profile 或分段 timestamp）
- [ ] 会用 **9 种线程状态** 框定下一步用 CPU 还是 Off-CPU 剖
- [ ] 跑通过 **CPU 火焰图** + 知道 **Off-CPU** 的必要性
- [ ] 编译参数保证 **符号 + 帧指针** 可用于 perf
- [ ] 并发代码查过 **伪共享** 与锁持有时间

---

## HFT 精读捷径（Ch 5 在路线中的位置）

```
Ch 2  方法论（延迟分解、P99）
Ch 3  OS（syscall、调度、线程模型）
Ch 4  观测工具（perf/BPF 选型）
Ch 5  应用程序（本章：优化主战场 + 剖应用方法论）
  → Ch 6 CPU / Ch 7 内存 / Ch 10 网络（资源层验证假设）
  → Ch 13 perf 实操
  → Ch 15 BPF + 04-BPF 专书
  → 12-HFT Practice 工程落地
```

**本章最小行动集：**

1. 对 **strategy 进程** 做线程状态粗分：top / pidstat / 一次 offcputime。
2. **perf record -g** → CPU 火焰图，找最宽函数；对照 Big O 与数据结构。
3. 检查 Release 编译是否 **-g -fno-omit-frame-pointer**，避免危机时 `[unknown]`。
4. 画一条 **tick 内 span**（recv → decode → book → signal → send），对齐 P99 尖刺窗口。

**Gregg 本章金句（HFT 版）：**

> 内核调优省 5%，换算法省 50%，**去掉不必要的工作省 500%**。  
> 剖应用时 **CPU 火焰图 + Off-CPU 火焰图** 缺一不可。

---

## 相关章节

- 上一章：[../chapter-04-observability-tools/](../chapter-04-observability-tools/)
- 下一章：[../chapter-06-cpus/](../chapter-06-cpus/)
- 方法论：[../chapter-02-methodologies/](../chapter-02-methodologies/)
- perf：[../chapter-13-perf/](../chapter-13-perf/)
- BPF：[../chapter-15-bpf/](../chapter-15-bpf/)
- BPF 专书：[04-BPF-Performance-Tools](../../04-BPF-Performance-Tools/)
- CSAPP 算法/机器级：[01-CSAPP-3rd](../../01-CSAPP-3rd/)
- HFT 工程：[15-HFT-Low-Latency-Practice](../../15-HFT-Low-Latency-Practice/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
