# 9.3 函数图追踪 (function_graph tracer)

> 🔴 精读

## 本节要点

### function_graph tracer

```bash
echo function_graph > current_tracer
echo 1 > tracing_on
sleep 1
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

### 控制选项

```bash
# 显示函数名+CPU号+进程名
echo 1 > options/funcgraph-proc
echo 1 > options/funcgraph-cpu
echo 1 > options/funcgraph-duration  # 显示耗时

# 过滤
echo 'my_driver_write' > set_graph_function  # 只追踪此函数的子调用
echo 1 > options/funcgraph-irqs  # 显示中断处理
echo 0 > options/funcgraph-irqs  # 不显示中断
```

### HFT 关联

function_graph 是 HFT 延迟分析的**核心工具**——一眼看出哪个函数耗时最长，以及完整的调用链。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** `set_graph_function` 和 `set_ftrace_filter` 的区别？

> `set_ftrace_filter` 限制 function tracer 只追踪指定函数（只显示函数名，不显示子调用）。`set_graph_function` 限制 function_graph tracer 只追踪指定函数及其**所有子调用**（显示完整调用树和耗时）。后者更适合分析单个函数的内部行为。


**Q:** function_graph tracer 如何测量函数执行时间？

> 在每个函数入口和出口分别记录时间戳。出口时间戳 - 入口时间戳 = 执行时间。function_graph 输出中用 `+` 后跟微秒数表示。例如 `schedule() {` ... `} 45.234 us` 表示 schedule 执行了 45μs。这对定位延迟瓶颈很有用。

</details>

## 交叉引用

- [05.6 ch09 function tracer](chapter-09-ftrace/notes/section-9-2.md)
