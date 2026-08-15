# 3.5 ftrace_printk (trace_marker 前身)

> 🔴 精读 · Part 2: Instrumentation & Memory Debugging

## 本节要点

`trace_printk()` 是 HFT 热路径调试的**首选工具**——写入 per-CPU 环形缓冲区，不阻塞、不改变时序。

## trace_printk vs printk

| 特性 | printk | trace_printk |
|------|--------|-------------|
| 输出目标 | 全局环形缓冲区 + 控制台 | ftrace per-CPU 环形缓冲区 |
| 中断上下文 | ✅ | ✅ |
| NMI 上下文 | ⚠️ 需 printk_deferred | ✅ |
| 控制台阻塞 | 可能（串口 ~17ms） | ❌ 不输出到控制台 |
| 时序影响 | 高（序列化输出 + I/O） | 极低（写内存缓冲区） |
| 开销 | ~1-10μs（不含控制台） | ~100-200ns |
| 锁 | 全局自旋锁 | per-CPU（无锁） |
| 读取方式 | dmesg | cat trace / trace_pipe |
| 时间戳精度 | 毫秒 | 纳秒 |
| 上下文信息 | CPU号 | CPU号 + 上下文（中断/进程） |

## 使用方法

### 基本用法

```c
#include <linux/ftrace.h>

// 基本用法
trace_printk("entered my_function, arg=%d\n", arg);

// 在热路径中使用
static int hft_process_packet(struct hft_dev *dev, struct sk_buff *skb)
{
    u64 ts = ktime_get_ns();

    trace_printk("rx: ts=%llu seq=%u len=%u\n",
                 ts, hdr->seq, skb->len);

    // 处理...

    trace_printk("done: ts=%llu elapsed=%lluns\n",
                 ktime_get_ns(), ktime_get_ns() - ts);

    return 0;
}
```

### 在 ftrace 中查看

```bash
# 方法 1: 查看所有 trace（包含 trace_printk 输出）
echo 1 > /sys/kernel/tracing/tracing_on
# ... 触发 trace_printk ...
cat /sys/kernel/tracing/trace

# 方法 2: 流式查看（不停止追踪）
cat /sys/kernel/tracing/trace_pipe

# 方法 3: 配合 function_graph tracer（推荐）
echo function_graph > /sys/kernel/tracing/current_tracer
echo 1 > /sys/kernel/tracing/options/funcgraph-proc
# trace_printk 输出会嵌入在函数调用图中
cat /sys/kernel/tracing/trace_pipe

# 输出示例:
#  CPU  DURATION                  FUNCTION            CALLS
#  0)   0.500 us    |    hft_process_packet() {
#  0)               |    /* rx: ts=1234567890 seq=42 len=64 */
#  0)   0.200 us    |      parse_header();
#  0)   0.300 us    |      validate_checksum();
#  0)               |    /* done: ts=1234567891 elapsed=1000ns */
#  0)   1.500 us    |    }
```

### trace_printk 的优势

```c
// ✅ 不会阻塞，不会改变时序
// 写入 per-CPU 环形缓冲区，无锁
trace_printk("hot path: ts=%llu price=%d\n", ts, price);
// 开销: ~100-200ns

// ❌ printk 可能阻塞数十毫秒（串口输出）
printk("hot path: ts=%llu price=%d\n", ts, price);
// 开销: ~1-10μs (无控制台) 或 ~17ms (串口控制台)
// 破坏热路径延迟！

// ✅ NMI 上下文安全
// printk 在 NMI 中可能死锁（logbuf lock）
// trace_printk 使用 per-CPU buffer，无锁，NMI 安全
```

## trace_printk vs trace_buf_size

```bash
# trace buffer 大小
cat /sys/kernel/tracing/buffer_size_kb
# 1408  (默认 ~1.4MB per CPU)

# 增大 buffer（保留更多历史）
echo 10240 > /sys/kernel/tracing/buffer_size_kb  # 10MB per CPU

# 查看 buffer 使用情况
cat /sys/kernel/tracing/per_cpu/cpu0/stats
# entries: 12345
# overrun: 0    # 0 = 没有丢失数据
# oldest event ts: 1234567.890
# now ts: 1234568.123
```

## 用户空间等价: trace_marker

```c
// Android 的 trace_marker (atrace) 是 trace_printk 的用户空间等价
// 写入 /sys/kernel/tracing/trace_marker

// 用户空间标记开始/结束
echo "B|1234|my_section" > /sys/kernel/tracing/trace_marker  // begin
echo "E" > /sys/kernel/tracing/trace_marker                  // end

// Linux 6.x 也支持
echo "HFT: order submitted" > /sys/kernel/tracing/trace_marker

// 在 trace 中查看
cat /sys/kernel/tracing/trace
#  <...>-1234  |   0.000 us | |   /* HFT: order submitted */
```

## 配合 ftrace tracer 使用

### trace_printk + function_graph

```bash
# 最推荐的调试组合
echo function_graph > /sys/kernel/tracing/current_tracer
echo 1 > /sys/kernel/tracing/options/funcgraph-proc
echo 1 > /sys/kernel/tracing/options/funcgraph-duration
echo 1 > /sys/kernel/tracing/tracing_on

# trace_printk 输出嵌入在调用图中
# 可以看到每个函数的耗时 + 自定义标记
cat /sys/kernel/tracing/trace_pipe
```

### trace_printk + 事件追踪

```bash
# 同时启用事件和 trace_printk
echo function > /sys/kernel/tracing/current_tracer
echo 1 > /sys/kernel/tracing/events/irq/enable
echo 1 > /sys/kernel/tracing/events/sched/enable
echo 1 > /sys/kernel/tracing/tracing_on

# trace 中同时显示: 函数调用 + 事件 + trace_printk
cat /sys/kernel/tracing/trace
```

### trace_printk 过滤

```bash
# 只看 trace_printk 输出
grep "trace_printk" /sys/kernel/tracing/trace

# 过滤特定关键字
grep "HFT" /sys/kernel/tracing/trace_pipe
```

## trace_printk 性能对比

```
开销对比（单次调用）:
┌─────────────────────┬──────────────┬──────────────────┐
│ 方法                │ 开销         │ 是否改变时序      │
├─────────────────────┼──────────────┼──────────────────┤
│ trace_printk        │ ~100-200ns   │ ❌ 不改变         │
│ printk (无控制台)   │ ~1-10μs      │ ⚠️ 轻微          │
│ printk (串口控制台) │ ~17ms        │ ✅ 严重改变       │
│ pr_debug (关闭)     │ ~1-2ns       │ ❌ 不改变         │
│ pr_debug (开启)     │ ~1-10μs      │ ⚠️ 轻微          │
│ ftrace function     │ ~10-50ns     │ ❌ 不改变         │
│ kprobe              │ ~100-500ns   │ ⚠️ 轻微          │
│ KGDB 断点           │ 暂停 CPU     │ ✅ 严重改变       │
└─────────────────────┴──────────────┴──────────────────┘
```

## HFT 关联

trace_printk 是 HFT 热路径调试的**首选**：

1. **不阻塞**：写入 per-CPU 内存缓冲区，无 I/O
2. **不改变时序**：~100ns 开销对微秒级热路径影响可忽略
3. **纳秒时间戳**：精确测量每步耗时
4. **配合 function_graph**：看到完整调用链 + 自定义标记
5. **NMI 安全**：在 NMI handler 中也可安全使用

```c
// HFT 热路径调试模板
static int hft_rx_packet(struct hft_dev *dev)
{
    u64 t0, t1, t2, t3;

    t0 = ktime_get_ns();
    trace_printk("rx enter: ts=%llu\n", t0);

    t1 = ktime_get_ns();
    struct sk_buff *skb = hft_dma_recv(dev);
    trace_printk("dma recv: %lluns\n", t1 - t0);

    t2 = ktime_get_ns();
    hft_parse_header(skb);
    trace_printk("parse: %lluns\n", t2 - t1);

    t3 = ktime_get_ns();
    hft_dispatch(skb);
    trace_printk("dispatch: %lluns total=%lluns\n",
                 t3 - t2, t3 - t0);

    return 0;
}
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** trace_printk 为什么比 printk 更适合热路径调试？

> trace_printk 写入 per-CPU 环形缓冲区（无锁、无 I/O），开销约 100-200ns。printk 输出到控制台时可能阻塞数十毫秒（串口），且持有全局自旋锁影响所有 CPU。对于 HFT 纳秒级热路径，trace_printk 的开销可接受，printk 不可接受。

**Q2:** trace_printk 的输出在哪里查看？和 dmesg 有什么区别？

> trace_printk 输出在 `/sys/kernel/tracing/trace`（或 `trace_pipe` 用于实时流式读取）。dmesg 只显示 printk 的输出。trace_printk 的输出包含时间戳、CPU 号、上下文（中断/进程），且可配合 function_graph tracer 嵌入函数调用链中，上下文更丰富。

**Q3:** trace_printk 相比 printk 的性能优势来自哪里？

> printk 写入全局 logbuf 需要锁 + 控制台输出（可能 I/O）。trace_printk 写入 per-CPU trace buffer，无锁（per-CPU），不做 I/O。快路径仅需 ~100ns（写环形缓冲区 + 时间戳）。

**Q4:** trace_printk 在 NMI 上下文中安全吗？

> 安全。trace_printk 使用 per-CPU buffer，不需要全局锁，在 NMI 中不会死锁。printk 在 NMI 中可能死锁（如果 NMI 打断了正在持有 logbuf lock 的 CPU）。6.x 引入了 printk_safe 机制部分缓解，但 NMI 中仍推荐用 trace_printk。

**Q5:** 如何配合 function_graph tracer 使用 trace_printk？

> 先设置 `echo function_graph > current_tracer`，启用 `echo 1 > options/funcgraph-proc`，然后开启追踪 `echo 1 > tracing_on`。trace_printk 的输出会以 `/* message */` 格式嵌入在函数调用图中，可以看到每个函数耗时 + 自定义标记的完整时间线。

</details>

## 交叉引用

- [05.6 ch03 printk 基础](../../chapter-03-printk/notes/01-printk-basics-loglevel.md)
- [05.6 ch09 ftrace 架构](../../chapter-09-ftrace/notes/01-ftrace-architecture-tracefs.md)
- [05.6 ch09 function_graph](../../chapter-09-ftrace/notes/03-function-graph-tracer.md)
