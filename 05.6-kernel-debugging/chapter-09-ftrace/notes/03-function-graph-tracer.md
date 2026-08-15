# 函数图追踪 (function_graph tracer)

> 🔴 精读

## 概念详解

### function_graph tracer 是什么

function_graph tracer 记录函数的调用链（进入 + 退出），用缩进显示嵌套关系，并测量每个函数的执行时间。它是 HFT 延迟分析的**核心工具**。

### 与 function tracer 的区别

| 特性 | function tracer | function_graph tracer |
|------|----------------|----------------------|
| 记录内容 | 函数名 + 调用者 | 函数进入/退出 + 耗时 |
| 显示方式 | 平铺列表 | 缩进树形 |
| 嵌套关系 | 不直观 | 一目了然 |
| 执行时间 | 不显示 | 显示 |
| 开销 | ~100-300ns/调用 | ~200-500ns/调用 |

### 基本用法

```bash
cd /sys/kernel/tracing

echo function_graph > current_tracer
echo 1 > tracing_on
sleep 1
echo 0 > tracing_on
cat trace | head -40

# 输出示例:
# 1)   |  my_app() {
# 0.500 us |    __kmalloc();
# 3.200 us |    memcpy();
# 1)   |    vfs_write() {
# 0.300 us |      rw_verify_area();
# 2.100 us |      my_driver_write();
# 5.600 us |    }
# 12.500 us |  }
```

### 输出解读

```
 1)   |  my_app() {                    ← CPU1, 函数入口
 0.500 us |    __kmalloc();            ← 耗时 0.5μs
 3.200 us |    memcpy();               ← 耗时 3.2μs
 1)   |    vfs_write() {               ← 嵌套函数入口
 0.300 us |      rw_verify_area();
 2.100 us |      my_driver_write();
 5.600 us |    }                        ← vfs_write 总耗时 5.6μs
 12.500 us |  }                          ← my_app 总耗时 12.5μs
```

### 控制选项

```bash
echo 1 > options/funcgraph-proc      # 显示进程名
echo 1 > options/funcgraph-cpu       # 显示 CPU 号
echo 1 > options/funcgraph-duration  # 显示耗时
echo 1 > options/funcgraph-abstime   # 显示绝对时间戳
echo 0 > options/funcgraph-irqs      # 不显示中断（减少噪音）
```

### 过滤

```bash
# 只追踪特定函数及其子调用
echo 'my_driver_write' > set_graph_function

# 追踪多个函数
echo 'vfs_write' > set_graph_function
echo 'my_driver_write' >> set_graph_function

# 排除函数（减少噪音）
echo 'rcu_*' > set_graph_notrace
echo '__*lock*' >> set_graph_notrace

# 限制追踪深度
echo 5 > max_graph_depth

# 按进程过滤
echo 1234 > set_ftrace_pid
```

### set_graph_function vs set_ftrace_filter

| 特性 | set_graph_function | set_ftrace_filter |
|------|-------------------|-------------------|
| 用于 | function_graph tracer | function tracer |
| 效果 | 追踪函数及其所有子调用 | 只追踪指定函数 |
| 典型用途 | 分析函数内部行为 | 统计函数调用频率 |

### 识别延迟热点

```bash
# 找出耗时最长的函数
cat trace | grep 'us |' | sort -t' ' -k1 -rn | head -20
# 123.456 us |    schedule()      ← 最大延迟
# 45.678 us  |    tcp_sendmsg()
# 12.345 us  |    mutex_lock()
```

### HFT 关联应用

function_graph 是 HFT 延迟分析的**核心工具**——一眼看出哪个函数耗时最长，以及完整的调用链。

```bash
# HFT: 分析交易路径延迟
echo function_graph > current_tracer
echo mono > trace_clock
echo 'my_trade_handler' > set_graph_function
echo 1 > tracing_on
# ... 触发交易 ...
echo 0 > tracing_on

# 分析输出: 找出 > 10us 的调用
cat trace | awk '/us \|/ {gsub(/us/,"",$1); if ($1+0 > 10) print}'
# 15.234 us |    tcp_sendmsg()     ← 网络延迟
# 45.678 us |    schedule()         ← 调度延迟
# 12.345 us |    mutex_lock()       ← 锁等待
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** `set_graph_function` 和 `set_ftrace_filter` 的区别？

> `set_ftrace_filter` 限制 function tracer 只追踪指定函数（不显示子调用）。`set_graph_function` 限制 function_graph tracer 只追踪指定函数及其**所有子调用**（显示完整调用树和耗时）。

**Q2:** function_graph tracer 如何测量函数执行时间？

> 在每个函数入口和出口分别记录时间戳。出口时间戳 - 入口时间戳 = 执行时间。用 `+` 后跟微秒数表示。

**Q3:** 如何减少 function_graph 输出中的噪音？

> (1) 用 `set_graph_function` 限制只追踪目标函数；(2) 用 `set_graph_notrace` 排除噪音函数（如 `rcu_*`）；(3) 用 `max_graph_depth` 限制嵌套深度；(4) 用 `options/funcgraph-irqs=0` 隐藏中断。

**Q4:** function_graph 输出中 CPU 号后面的 `|` 和缩进代表什么？

> `|` 表示函数调用层级。每一层缩进代表一层函数嵌套。`{` 表示函数入口，`}` 表示函数出口。通过缩进可以直观看出调用树结构。

**Q5:** HFT 延迟分析中如何用 function_graph 找到瓶颈？

> (1) 设置 `set_graph_function` 为交易入口函数；(2) 追踪后分析输出；(3) 找耗时最长的叶子函数（非嵌套函数）；(4) 找耗时最长的父函数（总时间）。关注 `schedule()`（调度延迟）和 `mutex_lock()`（锁等待）。

</details>

## 交叉引用

- [05.6 ch09 Ftrace 架构与 tracefs](../../chapter-09-ftrace/notes/01-ftrace-architecture-tracefs.md)
- [05.6 ch09 函数追踪 function tracer](../../chapter-09-ftrace/notes/02-function-tracer.md)
- [05.6 ch09 trace-cmd 命令行前端](../../chapter-09-ftrace/notes/05-trace-cmd.md)
