# 9.2 函数追踪 (function tracer)

> 🔴 精读

## 本节要点

### function tracer

```bash
# 启用函数追踪
echo function > current_tracer
echo 1 > tracing_on
sleep 2
echo 0 > tracing_on
cat trace | head -30

# 输出示例:
#                tracer: function
#  entries-in-buffer/entries-written: 12345/12345   P:4631
#                                _-----=> irqs-off
#                               / _----=> need-resched
#                              | / _---=> hardirq/softirq
#                              || / _--=> preempt-depth
#                              ||| /     delay
#             TASK-PID   CPU#  ||||   TIMESTAMP  FUNCTION
#                | |       |   ||||      |          |
#             my_app-1234  [002] .... 12345.678901: schedule <-sys_sched_yield
#             my_app-1234  [002] .... 12345.678902: rcu_all_qs <-schedule
```

### 过滤

```bash
# 只追踪特定函数
echo 'schedule' > set_ftrace_filter
echo 'schedule' > set_ftrace_filter  # 只追踪 schedule
echo 'sched*' > set_ftrace_filter     # 通配符
echo '!schedule' >> set_ftrace_filter # 排除某个函数

# 按模块过滤
echo ':mod:my_module' > set_ftrace_filter

# 按进程过滤
echo 1234 > set_ftrace_pid  # 只追踪 PID 1234
```

### HFT 关联

function tracer 可追踪交易线程调用的所有内核函数，定位延迟来源。但开销较大（~100-300ns/函数调用），不适合生产环境。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** function tracer 的性能开销大约是多少？

> 每次函数调用额外约 100-300ns（保存寄存器 + 写环形缓冲区）。对于高频函数调用（如 schedule、kmalloc），总开销显著。建议先用 `set_ftrace_filter` 过滤到少量函数，或用 function_graph tracer 看调用链后再精确过滤。

</details>
