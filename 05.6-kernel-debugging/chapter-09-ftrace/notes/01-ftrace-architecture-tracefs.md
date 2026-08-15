# Ftrace 架构与 tracefs 接口

> 🔴 精读 · Part 3: Diagnostics & Advanced Tools

## 概念详解

### Ftrace 是什么

Ftrace (Function Tracer) 是内核内建的追踪框架，不依赖任何外部工具。它提供多种 tracer（函数追踪、函数图、事件追踪等），通过 tracefs 虚拟文件系统控制。

### Ftrace 架构

```
用户接口 (/sys/kernel/tracing/ 或 /sys/kernel/debug/tracing/)
    ↓
tracefs (虚拟文件系统)
    ↓
Ftrace 核心 (tracer 引擎)
    ↓
┌─────────────┬──────────────┬──────────────┐
│ function    │ function_graph│ trace events │
│ tracer      │ tracer       │ (tracepoints)│
├─────────────┴──────────────┴──────────────┤
│        ftrace_ops (mcount / fentry)        │
├────────────────────────────────────────────┤
│        per-CPU ring buffer                 │
└────────────────────────────────────────────┘
```

### tracefs 接口

```bash
# 主目录 (5.x+ 使用 tracefs)
ls /sys/kernel/tracing/
# available_tracers  current_tracer  trace  trace_pipe
# set_ftrace_filter  set_event  options/  instances/
# events/  kprobe_events  trace_clock  tracing_on

# 旧路径 (4.x，软链接到 tracefs)
ls /sys/kernel/debug/tracing/

# 查看可用 tracer
cat available_tracers
# hwlat blk function_graph wakeup_dl wakeup_rt wakeup function nop
```

### tracer 说明

| tracer | 功能 | 适用场景 |
|--------|------|---------|
| `function` | 函数调用追踪 | 统计函数调用频率 |
| `function_graph` | 函数调用图（带耗时） | 分析调用链和延迟 |
| `wakeup` | 唤醒延迟追踪 | 调度延迟分析 |
| `wakeup_rt` | 实时任务唤醒延迟 | RT 调度分析 |
| `hwlat` | 硬件延迟检测 | 检测硬件延迟毛刺 |
| `blk` | 块设备追踪 | I/O 分析 |
| `nop` | 关闭 tracer（事件仍可用） | 仅用 trace events |

### 基本 操作流程

```bash
# 1. 选择 tracer
echo function > current_tracer

# 2. 开关追踪
echo 1 > tracing_on   # 开始
echo 0 > tracing_on   # 停止

# 3. 查看数据
cat trace             # 快照（不清空）
cat trace_pipe        # 流式（读后清空）

# 4. 清空缓冲区
echo > trace
```

### trace vs trace_pipe

| 特性 | trace | trace_pipe |
|------|-------|------------|
| 读取行为 | 快照（不清空） | 消费式（读后清空） |
| 适用场景 | 事后查看 | 实时监控 |
| 阻塞 | 非阻塞 | 有数据则读 |

### tracefs 关键文件

| 文件 | 用途 |
|------|------|
| `available_tracers` | 可用 tracer 列表 |
| `current_tracer` | 当前 tracer |
| `tracing_on` | 追踪开关 |
| `trace` / `trace_pipe` | 追踪数据 |
| `set_ftrace_filter` | 函数过滤 |
| `events/` | 事件目录 |
| `instances/` | 多实例 |
| `trace_clock` | 时间戳时钟 |
| `buffer_size_kb` | 缓冲区大小 |

### 多实例 (instances)

```bash
# 创建独立的追踪实例
mkdir /sys/kernel/tracing/instances/my_trace
# 每个实例有独立的 current_tracer/trace/events/

# 删除实例
rmdir /sys/kernel/tracing/instances/my_trace
```

### 时间戳时钟

```bash
cat trace_clock
# local global counter uptime mono mono_raw boot

echo mono > trace_clock  # 推荐：单调时钟，适合延迟分析
```

### ring buffer

```
per-CPU ring buffer:
  CPU0: [data] → [data] → [data] → ... (环形)
  CPU1: [data] → [data] → [data] → ... (环形)

优势:
  - 每个 CPU 写自己的 buffer，无需锁
  - 读取时按时间戳合并各 CPU buffer
  - HFT 场景中不会引入跨核锁竞争
```

### HFT 关联应用

```bash
# HFT 延迟分析: 用 function_graph 追踪交易路径
cd /sys/kernel/tracing
echo mono > trace_clock
echo function_graph > current_tracer
echo my_trade_handler > set_graph_function
echo 1 > tracing_on
# ... 触发交易 ...
echo 0 > tracing_on
cat trace > /tmp/trade_trace.log
```

### Ftrace 的性能特点

| 特性 | 说明 |
|------|------|
| 零依赖 | 内核内建，无需安装工具 |
| per-CPU buffer | 无锁写入，不引入跨核竞争 |
| 动态插桩 | 关闭时零开销 |
| 开销 | function tracer ~100-300ns/调用 |
| 适用 | 开发/staging，不适合 HFT 生产 |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** `trace` 和 `trace_pipe` 的区别？

> `trace` 读取快照，不清空，适合事后查看。`trace_pipe` 是流式读取，读后清空，适合实时监控。

**Q2:** tracefs 和 debugfs 中的 ftrace 接口有什么关系？

> 4.x 之前在 /sys/kernel/debug/tracing/。5.x+ 迁移到 /sys/kernel/tracing/（tracefs），debugfs 路径保留为软链接。推荐使用 tracefs 路径。

**Q3:** ftrace 的 ring buffer 是 per-CPU 的，为什么？

> per-CPU ring buffer 避免了写入时的锁竞争。每个 CPU 写自己的 buffer 无需同步。HFT 场景中不会引入跨核锁竞争。

**Q4:** instances（多实例）有什么用？

> 允许创建独立的追踪实例，每个实例有自己的 current_tracer/events/trace。可以同时运行多个不同配置的追踪。

**Q5:** HFT 延迟分析时为什么建议用 `mono` 时钟？

> `mono` (CLOCK_MONOTONIC) 是单调时钟，不受 NTP 调整影响，适合测量时间间隔。默认的 `local` 是 per-CPU 时钟，跨 CPU 分析时不准确。

</details>

## 交叉引用

- [05.6 ch09 函数追踪 function tracer](../../chapter-09-ftrace/notes/02-function-tracer.md)
- [05.6 ch09 函数图追踪 function_graph](../../chapter-09-ftrace/notes/03-function-graph-tracer.md)
- [05.6 ch09 事件追踪 trace events](../../chapter-09-ftrace/notes/04-trace-events.md)
- [05.6 ch09 Ftrace 与 eBPF 的关系](../../chapter-09-ftrace/notes/08-ftrace-ebpf-relation.md)
