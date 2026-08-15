# 函数追踪 (function tracer)

> 🔴 精读

## 概念详解

### function tracer 是什么

function tracer 追踪内核中所有函数调用（或过滤后的子集），记录函数名、调用者、时间戳和上下文信息。

### 工作原理

```
编译时: -pg 选项在每个函数入口插入 mcount/fentry 调用
运行时:
  1. tracer 关闭时: mcount 直接 ret (1条指令，零开销)
  2. tracer 开启时: mcount 调用 ftrace_ops 回调
  3. 回调将函数名/时间戳写入 per-CPU ring buffer
```

### 基本用法

```bash
cd /sys/kernel/tracing

echo function > current_tracer
echo 1 > tracing_on
sleep 2
echo 0 > tracing_on
cat trace | head -30

# 输出示例:
#             TASK-PID   CPU#  ||||   TIMESTAMP  FUNCTION
#                | |       |   ||||      |          |
#             my_app-1234  [002] .... 12345.678901: schedule <-sys_sched_yield
#             my_app-1234  [002] .... 12345.678902: rcu_all_qs <-schedule
```

### 输出字段解读

| 字段 | 含义 | 示例 |
|------|------|------|
| TASK-PID | 进程名/PID | `my_app-1234` |
| CPU# | CPU 编号 | `[002]` |
| irqs-off | 中断是否禁用 | `d` = disabled |
| need-resched | 是否需要调度 | `N` = need resched |
| hardirq/softirq | 中断上下文 | `H` = hardirq, `s` = softirq |
| preempt-depth | 抢占深度 | `.` = 0 |
| TIMESTAMP | 时间戳 | `12345.678901` |
| FUNCTION | 函数名 <- 调用者 | `schedule <-sys_sched_yield` |

### 过滤

```bash
# 只追踪特定函数
echo 'schedule' > set_ftrace_filter
echo 'sched*' > set_ftrace_filter          # 通配符
echo '*lock*' > set_ftrace_filter           # 包含 lock
echo '!schedule' >> set_ftrace_filter       # 排除某个函数

# 按模块过滤
echo ':mod:my_module' > set_ftrace_filter

# 按进程过滤
echo 1234 > set_ftrace_pid

# 清空过滤
echo > set_ftrace_filter
```

### set_ftrace_filter 通配符

| 模式 | 含义 | 示例 |
|------|------|------|
| `*` | 匹配任意字符 | `sched*` |
| `?` | 匹配单个字符 | `sched_switch?` |
| `!func` | 排除函数 | `!schedule` |
| `:mod:NAME` | 按模块 | `:mod:my_module` |

### 查看可用过滤函数

```bash
cat available_filter_functions | wc -l
# 40000+ (取决于内核配置)

cat available_filter_functions | grep my_driver
# my_driver_write
# my_driver_read
```

### function tracer 的中断标志

```
my_app-1234 [002] d..2 12345.678: schedule <-sys_sched_yield
                   ^^^
                   |||
                   ||+-- preempt-depth (2 = 持有2个锁)
                   |+--- softirq (s = 在软中断中)
                   +---- hardirq (d = 中断禁用)
```

### HFT 关联应用

```bash
# HFT: 追踪交易线程调用的所有内核函数
echo function > current_tracer
echo 'tcp_*' > set_ftrace_filter
echo 'schedule' >> set_ftrace_filter
echo $(pidof trade_app) > set_ftrace_pid
echo 1 > tracing_on
# ... 运行交易 ...
echo 0 > tracing_on

# 分析: 哪些内核函数被调用最多
cat trace | awk '{print $NF}' | sort | uniq -c | sort -rn | head -20
```

```c
// HFT: 使用 trace_printk() 标记关键点
void on_trade_signal(struct signal *sig) {
    trace_printk("trade signal received: %d\n", sig->type);
    // 在 function tracer 输出中会出现这条标记
}
```

### 性能开销

| 配置 | 开销 | 说明 |
|------|------|------|
| tracer 关闭 | ~0 | mcount 直接 ret |
| tracer 开启(无过滤) | ~100-300ns/调用 | 追踪所有函数 |
| tracer 开启(有过滤) | ~50-100ns/调用 | 只追踪过滤函数 |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** function tracer 的性能开销大约是多少？

> 每次函数调用额外约 100-300ns。建议先用 `set_ftrace_filter` 过滤到少量函数，配合 `set_ftrace_pid` 限制进程。

**Q2:** set_ftrace_filter 如何提高性能？

> 不过滤时追踪所有函数（数十万次/秒），buffer 溢出快且开销大。set_ftrace_filter 限制只追踪特定函数，减少 buffer 写入和开销。

**Q3:** function tracer 和 function_graph tracer 的区别？

> function tracer 记录每个函数调用事件（时间戳+函数名+调用者）。function_graph tracer 记录调用链（进入+退出+耗时），用缩进显示嵌套。function_graph 更直观但开销更大。

**Q4:** 输出中的 `d..2` 标志代表什么？

> `d` = 中断禁用，`.` = 不需要调度，`.` = 不在软中断中，`2` = preempt-depth 为 2（持有 2 个锁）。帮助判断函数调用的上下文。

**Q5:** `trace_printk()` 有什么用？

> 将自定义消息写入 ftrace ring buffer，与函数调用记录混合显示。用于标记关键事件，开销比 printk 低得多（~200ns vs ~10μs）。

</details>

## 交叉引用

- [05.6 ch09 Ftrace 架构与 tracefs](../../chapter-09-ftrace/notes/01-ftrace-architecture-tracefs.md)
- [05.6 ch09 函数图追踪 function_graph](../../chapter-09-ftrace/notes/03-function-graph-tracer.md)
- [05.6 ch09 事件追踪 trace events](../../chapter-09-ftrace/notes/04-trace-events.md)
- [05.6 ch03 ftrace_printk](../../chapter-03-printk/notes/05-ftrace-printk.md)
