# 事件追踪 (trace events)

> 🔴 精读

## 概念详解

### tracepoints (静态追踪点)

tracepoints 是开发者在内核代码中预埋的追踪点，性能开销极低（未启用时几乎为零）。与 kprobes 的动态探针不同，tracepoints 数量有限但稳定可靠。

### 查看可用事件

```bash
ls /sys/kernel/tracing/events/
# block/  ext4/  irq/  kmem/  net/  sched/  syscalls/  timer/  ...

# 查看某类事件
ls /sys/kernel/tracing/events/sched/
# sched_switch  sched_wakeup  sched_process_fork  ...

# 查看事件格式
cat /sys/kernel/tracing/events/sched/sched_switch/format
# 显示事件 ID 和字段布局
```

### 启用事件

```bash
# 启用单个事件
echo 1 > events/sched/sched_switch/enable

# 启用整类事件
echo 1 > events/sched/enable

# 启用所有事件
echo 1 > events/enable

# 关闭
echo 0 > events/sched/sched_switch/enable
```

### 查看事件输出

```bash
echo 1 > events/sched/sched_switch/enable
echo 1 > tracing_on
cat trace_pipe

# 输出:
# my_app-1234 [002] d..2 12345.678: sched_switch: prev_comm=my_app prev_pid=1234 prev_prio=120 prev_state=S ==> next_comm=swapper/2 next_pid=0 next_prio=120
```

### 过滤事件

```bash
# 按字段过滤
echo 'prev_pid == 1234' > events/sched/sched_switch/filter
echo 1 > events/sched/sched_switch/enable

# 只看特定进程的调度切换
echo 'prev_comm == "trade_app"' > events/sched/sched_switch/filter
```

### 查看事件字段

```bash
cat /sys/kernel/tracing/events/sched/sched_switch/format
# field:char prev_comm[16]; offset:8; size:16; signed:0;
# field:pid_t prev_pid; offset:24; size:4; signed:1;
# field:int prev_prio; offset:28; size:4; signed:1;
# field:int prev_state; offset:32; size:4; signed:1;
# field:char next_comm[16]; offset:36; size:16; signed:0;
# field:pid_t next_pid; offset:52; size:4; signed:1;
# field:int next_prio; offset:56; size:4; signed:1;
```

### 常用 HFT 相关事件

| 事件 | 子系统 | 用途 |
|------|--------|------|
| `sched_switch` | sched | 追踪调度切换 |
| `sched_wakeup` | sched | 追踪唤醒 |
| `irq_handler_entry/exit` | irq | 追踪中断处理 |
| `kmalloc/kfree` | kmem | 追踪内存分配 |
| `netif_receive_skb` | net | 追踪网络收包 |
| `timer_expire` | timer | 追踪定时器 |
| `sys_enter/exit` | syscalls | 追踪系统调用 |

### tracepoints vs kprobes

| 特性 | tracepoints | kprobes |
|------|------------|---------|
| 类型 | 静态（代码中预埋） | 动态（运行时插入） |
| 性能开销 | 极低（未启用时~0） | 较高（~100-200ns） |
| 灵活性 | 固定位置 | 任意函数 |
| 数量 | 有限（内核预定义） | 无限（任意地址） |
| 稳定性 | 稳定（API 保证） | 可能因内核版本变化 |

### trace event vs function tracer

| 特性 | trace event | function tracer |
|------|------------|----------------|
| 粒度 | 特定事件 | 所有函数调用 |
| 数据 | 结构化参数 | 仅函数名+调用者 |
| 开销 | 可控（只启用需要的事件） | 较高（所有函数） |
| 精确度 | 高（预定义语义） | 低（只有函数名） |

### HFT 关联应用

```bash
# HFT: 追踪交易线程的调度切换
echo 'prev_comm == "trade_app" || next_comm == "trade_app"' > events/sched/sched_switch/filter
echo 1 > events/sched/sched_switch/enable
echo 1 > tracing_on

# 追踪网络收包延迟
echo 1 > events/net/netif_receive_skb/enable

# 追踪中断处理时间
echo 1 > events/irq/irq_handler_entry/enable
echo 1 > events/irq/irq_handler_exit/enable

# 分析: 交易线程被调度出去的原因
cat trace | grep "prev_comm=trade_app"
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** tracepoints 和 kprobes 有什么区别？

> tracepoints 是**静态**的——开发者在代码中预埋的追踪点，性能开销极低。kprobes 是**动态**的——可在任意函数入口插入探针。tracepoints 数量有限但稳定，kprobes 灵活性更高但开销更大。

**Q2:** trace event 和 function tracer 的区别？

> function tracer 追踪所有函数调用（粗粒度）。trace event 追踪特定事件（如 sched_switch），携带结构化参数。trace event 更精确且开销可控。

**Q3:** 如何查看某个 trace event 携带哪些字段？

> `cat /sys/kernel/tracing/events/sched/sched_switch/format`。输出显示事件 ID 和字段布局。这些字段可以在 filter 中使用。

**Q4:** 如何只追踪特定进程的调度切换？

> `echo 'prev_pid == 1234' > events/sched/sched_switch/filter`，然后启用事件。filter 支持字段比较（==, !=, >, <）和逻辑组合（&&, ||）。

**Q5:** HFT 中追踪中断处理时间需要启用哪些事件？

> 启用 `irq_handler_entry` 和 `irq_handler_exit` 两个事件。entry 记录中断开始时间，exit 记录结束时间。两者时间差就是中断处理时间。可用 `trace-cmd report` 或脚本计算。

</details>

## 交叉引用

- [05.6 ch09 Ftrace 架构与 tracefs](../../chapter-09-ftrace/notes/01-ftrace-architecture-tracefs.md)
- [05.6 ch09 trace-cmd 命令行前端](../../chapter-09-ftrace/notes/05-trace-cmd.md)
- [05.6 ch04 kprobes 架构](../../chapter-04-kprobes/notes/01-kprobes-architecture.md)
