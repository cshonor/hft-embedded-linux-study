# 3. 传统内核分析工具

### Ftrace

Linux 内置追踪器 — SysPerf 有专章：[chapter-14-ftrace](../../../03-Systems-Performance-2nd/chapter-14-ftrace/)

| 能力 | 示例 |
|------|------|
| **kprobe + stack** | 函数命中栈 |
| **function graph** | 子调用 **耗时图** |
| 统计 | `echo function > current_tracer` |

```bash
# 示意 — 详见 SysPerf ftrace 笔记
cat /sys/kernel/debug/tracing/available_tracers
```

**BPF vs Ftrace：** BPF 易 **聚合/直方图/过滤**；Ftrace **funcgraph** 对读内核代码流仍极强 — 互补。

### perf sched

```bash
perf sched record -- sleep 10
perf sched latency
```

调度 **延迟、等待、运行时间** — 与 `runqlat`（Ch 6）同族。

### slabtop

```bash
slabtop -o
```

当前 **Slab 缓存占用** — 内核内存压力传统首选。

| | `slabtop` | **`slabratetop`** (BPF) |
|---|-----------|-------------------------|
| 看什么 | **当前总量** | **分配速率** |

---
