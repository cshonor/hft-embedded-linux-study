# 3.5 ftrace_printk (trace_marker 前身)

> 🔴 精读

## 本节要点

### trace_printk vs printk

| 特性 | printk | trace_printk |
|------|--------|-------------|
| 输出目标 | 环形缓冲区 + 控制台 | ftrace 环形缓冲区 |
| 中断上下文 | ✅ | ✅ |
| 控制台阻塞 | 可能（串口） | ❌ 不输出到控制台 |
| 时序影响 | 高（序列化输出） | 极低（写内存缓冲区） |
| 读取方式 | dmesg | trace cat / trace_pipe |

### 使用方法

```c
#include <linux/ftrace.h>

// 基本用法
trace_printk("entered my_function, arg=%d\n", arg);

// 在 ftrace 中查看
// 方法 1: trace_printk tracer
echo 1 > /sys/kernel/debug/tracing/options/trace_printk
cat /sys/kernel/debug/tracing/trace

// 方法 2: 配合 function_graph tracer
echo function_graph > /sys/kernel/debug/tracing/current_tracer
echo 1 > /sys/kernel/debug/tracing/options/funcgraph-proc
# trace_printk 输出会嵌入在函数调用图中
cat /sys/kernel/debug/tracing/trace_pipe
```

### trace_printk 的优势

```c
// 不会阻塞，不会改变时序
// 写入 per-CPU 环形缓冲区，无锁
trace_printk("hot path: ts=%llu price=%d\n", ts, price);

// 对比 printk 的风险
printk("hot path: ts=%llu price=%d\n", ts, price);
// ↑ 可能阻塞数十毫秒（串口输出），破坏热路径延迟
```

### 用户空间等价: trace_marker

```c
// Android 的 trace_marker (atrace) 是 trace_printk 的用户空间等价
// 写入 /sys/kernel/debug/tracing/trace_marker
echo "B|1234|my_section" > /sys/kernel/debug/tracing/trace_marker  // begin
echo "E" > /sys/kernel/debug/tracing/trace_marker                  // end

// 6.x 也支持 perf trace-event
```

## HFT 关联

trace_printk 是 HFT 热路径调试的**首选**——不阻塞、不改变时序、可配合 function_graph 看完整调用链。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** trace_printk 为什么比 printk 更适合热路径调试？

> trace_printk 写入 per-CPU 环形缓冲区（无锁、无 I/O），开销约 100-200ns。printk 输出到控制台时可能阻塞数十毫秒（串口），且持有全局自旋锁影响所有 CPU。对于 HFT 纳秒级热路径，trace_printk 的开销可接受，printk 不可接受。

**Q2:** trace_printk 的输出在哪里查看？和 dmesg 有什么区别？

> trace_printk 输出在 `/sys/kernel/debug/tracing/trace`（或 `trace_pipe` 用于实时流式读取）。dmesg 只显示 printk 的输出。trace_printk 的输出包含时间戳、CPU 号、上下文（中断/进程），且可配合 function_graph tracer 嵌入函数调用链中，上下文更丰富。


**Q:** trace_printk 相比 printk 的性能优势来自哪里？

> printk 写入全局 logbuf 需要锁 + 控制台输出（可能 I/O）。trace_printk 写入 per-CPU trace buffer，无锁（per-CPU），不做 I/O。快路径仅需 ~100ns（写环形缓冲区 + 时间戳）。

**Q:** trace_printk 的输出在哪里查看？

> 在 trace buffer 中查看：`cat /sys/kernel/tracing/trace`。需要先启用 tracing（`echo 1 > tracing_on`）。trace_printk 消息出现在 trace 输出的 FUNCTION 列中，格式 "trace_printk: message"。

</details>

## 交叉引用

- [05.6 ch09 ftrace](chapter-09-ftrace/notes/section-9-1.md)
