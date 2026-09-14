# Ch6 性能类：热点采样

> 选读 · 程序「太慢」怎么办（深入交给 06.6）

**这一章解决什么症状**：CPU 高、吞吐低、延迟毛刺——「性能」类问题。注意：性能调优是**另一个大领域**，本模块只讲「用 perf 快速定位热点函数」这一入门技能，深入的系统级性能分析交给 [06.6 Systems Performance](../../06.6-systems-performance/)。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 6.1 perf 基础采样（record / report 定位热点函数） | `notes/01-perf-basics.md` |
| 6.2 火焰图（调用链可视化 / cache miss 初探） | `notes/02-flamegraph.md` |

---

## 可跑的 demo（`code/`）

| 文件 | 演示什么 | 一句话看点 |
|------|----------|------------|
| `code/c6_1_cache_miss.c` | 同一份计算，只改**访存顺序** | 加法次数一模一样，按列遍历慢 **11.2 倍**；16 MiB 时只慢 1.4 倍——**尺寸就是结论** |
| `code/c6_2_self_sampler.c` | 不装 perf，自己在进程内实现一个采样 profiler | 采出与 `perf report` 同构的热点表：`slow_sqrt` 占 **87.8%** |

> **本章的诚实边界**：perf 本体测不了（容器里没有 PMU 权限、也没装 perf），
> 所以 `perf stat` / `perf report` 的**输出形状是格式示意**。
> 但「采样 → 聚合 → 热点表」这条原理链是**完整实测**的：`c6_2` 把 perf 的
> 三步（定时中断 → 记录 PC+栈 → 按函数聚合）用 `setitimer(ITIMER_PROF)`
> + `SIGPROF` + `backtrace()` 自己走了一遍，而且它输出的 **folded 折叠栈格式**
> 就是 `stackcollapse-perf.pl` 的输出——贴到自己的 Linux 上跑 `flamegraph.pl`
> 就得到一张真火焰图。

详见 [`code/README.md`](code/README.md)（含实测输出与四个踩坑记录）。

---

## HFT 关联

- **延迟毛刺先采样别猜**：某笔单延迟 250ms，perf 采样 + 火焰图直接定位热点函数，比读代码猜高效得多；
- **cache miss 是低延迟杀手**：perf 的 cache 事件采样能定位「哪行代码在频繁刷缓存」，是 HFT 优化的入口；
- **边界**：这里只做「定位热点」，系统级瓶颈分析（CPU/内存/IO/网络）交给 06.6。
