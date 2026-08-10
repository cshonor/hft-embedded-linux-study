# 9.1 Ftrace 架构与 tracefs 接口

> 🔴 精读 · Part 3: Diagnostics & Advanced Tools

## 本节要点

### Ftrace 架构

```
用户接口 (/sys/kernel/debug/tracing/)
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
# 主目录
ls /sys/kernel/debug/tracing/
# available_tracers    current_tracer   trace          trace_pipe
# set_ftrace_filter    set_event        options/       instances/
# events/              kprobe_events    trace_clock    tracing_on

# 查看可用 tracer
cat available_tracers
# hwlat blk function_graph wakeup_dl wakeup_rt wakeup function nop

# 设置 tracer
echo function > current_tracer
echo function_graph > current_tracer
echo nop > current_tracer  # 关闭

# 开关追踪
echo 1 > tracing_on   # 开始
echo 0 > tracing_on   # 停止
cat trace             # 查看快照 (停止后查看)
cat trace_pipe        # 流式查看 (不停止)
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** `trace` 和 `trace_pipe` 的区别？

> `trace` 读取环形缓冲区的快照，读取后不清空，适合事后查看。`trace_pipe` 是流式读取，读取后数据被消费，适合实时监控（如 `cat trace_pipe` 持续输出）。调试时用 `trace`（保留数据），监控时用 `trace_pipe`。


**Q:** tracefs 和 debugfs 中的 ftrace 接口有什么关系？

> 4.x 之前 ftrace 在 /sys/kernel/debug/tracing/。5.x+ 迁移到 /sys/kernel/tracing/（tracefs），debugfs 路径保留为软链接。推荐使用 tracefs 路径。两者底层完全相同。

**Q:** ftrace 的 ring buffer 是 per-CPU 的，为什么？

> per-CPU ring buffer 避免了写入时的锁竞争——每个 CPU 写自己的 buffer 无需同步。读取时 ftrace 按时间戳合并各 CPU buffer。这种设计在高核数系统上扩展性好。HFT 场景中 per-CPU buffer 意味着 ftrace 不会引入跨核锁竞争。

</details>

## 交叉引用

- [05.6 ch09 function tracer](chapter-09-ftrace/notes/section-9-2.md)
