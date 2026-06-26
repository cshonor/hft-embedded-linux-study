# Ch 6 中央处理器 · CPUs

> **Systems Performance 2nd** · Brendan Gregg · **精读**

> 本章定位：**CPU 驱动一切软件** — 性能分析往往从 CPU 开刀。Ch 2 的 USE/饱和度在这里落地；Ch 5 的应用热点最终要落到「哪颗核、多少 IPC、run queue 多长」。本章从硬件模型、调度器、PMC 周期分析到 perf/BPF 工具与火焰图，构成 **CPU 资源层的完整地图**。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 6.1–6.3 CPU 模型与核心概念 | [notes/section-6.1-6.3-CPU-模型与核心概念.md](./notes/section-6.1-6.3-CPU-模型与核心概念.md) |
| 6.4 硬件与软件架构 | [notes/section-6.4-硬件与软件架构.md](./notes/section-6.4-硬件与软件架构.md) |
| 6.5 性能分析方法论 | [notes/section-6.5-性能分析方法论.md](./notes/section-6.5-性能分析方法论.md) |
| 6.6–6.7 观测工具与可视化 | [notes/section-6.6-6.7-观测工具与可视化.md](./notes/section-6.6-6.7-观测工具与可视化.md) |
| 6.9 CPU 调优 | [notes/section-6.9-CPU-调优.md](./notes/section-6.9-CPU-调优.md) |

---

## 大白话 · 本章就五件事

> **CPU 快不快，不只看频率，还要看 IPC、队列和 cache。**

**① 先搞清硬件层级：Processor → Core → 硬件线程 → Cache → Run Queue。**

- 一颗 CPU 多核心；**超线程（SMT）** 是共享执行资源的两个逻辑核 — HFT 热路径常 **禁用或独占物理核**。
- **L1/L2/L3** 决定访存延迟；**Run Queue** 决定线程等 CPU 等多久。

**② 两个比「GHz」更重要的数：IPC 和 Stall。**

- **IPC**（每周期指令数）高 = 算力用满；**低 IPC** 常意味着在等内存（**stall cycles**）。
- **使用率** 看忙不忙；**饱和度** 看 run queue / 调度延迟 — 利用率不高也可能在排队。

**③ 硬件机制 + 调度器 = 你看到的 top/mpstat。**

- **P-State** 调频率、**C-State** 省电休眠 — 低延迟机器用 `performance` governor，慎用深 C-State。
- Linux **CFS / RT 调度**、抢占、负载均衡 vs **CPU affinity / NUMA 本地性** — 量化要绑核、隔离 housekeeping。

**④ 分析三板斧：USE、Profile、Cycle Analysis（PMC）。**

- 每颗 CPU：**Utilization / Saturation / Errors**。
- **perf record** + **CPU 火焰图** 找热点函数。
- **perf stat** 测 IPC、cache miss — 判断是算力问题还是内存问题。

**⑤ 调优：先删活，再绑核、调频、cgroups。**

- `taskset` / **isolcpus** / cpusets；**PSI** 比 load average 更准；**FlameScope** 抓微秒级抖动。

下面按原书 6.1–6.7、6.9 展开（6.8 为案例/延伸，与工具章重叠处见 Ch 13/15）。

---

## 本章 Checklist

- [ ] 能画 **Socket → Core → Thread → Cache → Run Queue** 层级
- [ ] 会用 **`perf stat`** 看 IPC，并知道低 IPC 常指向内存/stall
- [ ] 对每 CPU 做过 **USE**：mpstat + run queue/PSI
- [ ] 跑过 **`perf record -g`** 并读过 CPU 火焰图最宽栈
- [ ] 知道 **load average ≠ CPU 利用率**，会看 **mpstat -P ALL**
- [ ] 裸机确认 **governor / isolcpus / IRQ affinity** 配置文档化

---

## HFT 精读捷径（Ch 6 在路线中的位置）

```
Ch 2  USE / 饱和度 / 火焰图概念
Ch 3  调度、上下文切换、syscall
Ch 5  应用热点、Off-CPU
Ch 6  CPU（本章：硬件 + 调度 + PMC + 工具）
  → Ch 7 内存（IPC 低时常跳这里）
  → Ch 10 网络（kernel % / softirq 高时）
  → Ch 13 perf 深入
  → Ch 15 BPF runqlat/profile
  → 02-Hennessy cache/MESI
  → 12-HFT ch05 内核调优落地
```

**本章最小行动集：**

1. **`mpstat -P ALL 1`** 跑 60 秒，记录是否单核热点、sys% 是否异常。
2. **`perf stat -e cycles,instructions,cache-misses`** 对 strategy 进程压测一轮，记下 IPC。
3. **`sudo runqlat-bpfcc 10`** 看 dedicated 核 run queue 延迟是否接近 0。
4. **CPU 火焰图** 一张 + 对照 Ch 5 线程状态，确认热点在 User 还是 Kernel。

**Gregg 本章金句（HFT 版）：**

> CPU 通常是第一个要查的资源 — 但 **高利用率不等于高效**，要看 **IPC、run queue 和火焰图**。  
> **绑核** 是为了 cache 和 NUMA 本地性，不是为了把 load average 做好看。

---

## 相关章节

- 上一章：[../chapter-05-applications/](../chapter-05-applications/)
- 下一章：[../chapter-07-memory/](../chapter-07-memory/)
- 方法论 / USE：[../chapter-02-methodologies/](../chapter-02-methodologies/) · [appendix-A-USE方法Linux.md](../appendix-A-USE方法Linux.md)
- 观测工具：[../chapter-04-observability-tools/](../chapter-04-observability-tools/)
- perf：[../chapter-13-perf/](../chapter-13-perf/)
- BPF：[../chapter-15-bpf/](../chapter-15-bpf/)
- OS 调度：[../chapter-03-operating-systems/](../chapter-03-operating-systems/)
- 架构：[02-Computer-Architecture-6th](../../02-Computer-Architecture-6th/)
- HFT 绑核调优：[12-HFT ch05](../../15-HFT-Low-Latency-Practice/chapter-05-操作系统内核极致调优/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
