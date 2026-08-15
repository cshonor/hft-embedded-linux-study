# Ftrace 与 eBPF 的关系

> 🔴 精读

## 概念详解

### Ftrace vs eBPF 对比

| 特性 | Ftrace | eBPF |
|------|--------|------|
| 内核版本 | 2.6+ (广泛) | 4.x+ (5.x+ 完整) |
| 编程 | 预定义 tracer + 事件 | 自定义程序 |
| 数据处理 | 简单过滤 | map 聚合 + 直方图 |
| 性能开销 | ~100-300ns/事件 | ~50-100ns/事件 |
| 安全性 | 需 root | 验证器保证 |
| 输出 | trace 文件 | map + pipe + 事件 |
| 依赖 | 内核内建 | 需要工具链 (clang/LLVM) |

### eBPF 替代的 Ftrace 功能

| Ftrace 功能 | eBPF 替代 | 优势 |
|------------|----------|------|
| function tracer | bpftrace kprobe | 自定义过滤/聚合 |
| function_graph | bpftrace kretprobe | 自定义耗时统计 |
| trace events | bpftrace tracepoint | 自定义字段提取 |
| hist trigger | bpftrace map hist | 更灵活的直方图 |

### bpftrace 示例

```bash
# 替代 funclatency: 测量 schedule() 耗时
bpftrace -e 'kretprobe:schedule { @ns = hist(nsecs - @start[tid]); } kprobe:schedule { @start[tid] = nsecs; }'

# 替代 funccount: 统计函数调用
bpftrace -e 'kprobe:vfs_* { @[func] = count(); }'

# 替代 trace events: 追踪调度切换
bpftrace -e 'tracepoint:sched:sched_switch { @[args->next_comm] = count(); }'

# 测量函数耗时并按进程分组
bpftrace -e 'kprobe:vfs_write { @start[tid] = nsecs; } kretprobe:vfs_write /@start[tid]/ { @us[comm] = hist((nsecs - @start[tid]) / 1000); delete(@start[tid]); }'
```

### 何时用 Ftrace vs eBPF

| 场景 | 推荐工具 | 原因 |
|------|---------|------|
| 快速排查 | Ftrace | tracefs 即用，无需安装 |
| 复杂数据处理 | eBPF | map 聚合、直方图、条件逻辑 |
| 低开销生产环境 | eBPF | 开销更低，验证器保证安全 |
| 内核版本 < 4.x | Ftrace | eBPF 功能不全 |
| 需要可视化时间线 | Ftrace + KernelShark | 时间线视图 |
| 自定义统计 | eBPF | 可编程聚合 |

### eBPF 的优势

1. **可编程**：自定义过滤条件、聚合逻辑、输出格式
2. **低开销**：JIT 编译，~50-100ns/事件
3. **安全**：验证器保证程序不会崩溃内核
4. **灵活输出**：map（聚合）、pipe（流式）、事件（结构化）

### Ftrace 仍然不可替代的场景

1. **function_graph tracer**：eBPF 没有等价的调用图可视化
2. **零依赖**：不需要安装任何工具
3. **KernelShark 可视化**：eBPF 没有等价的 GUI
4. **旧内核**：4.x 以下只有 ftrace

### Ftrace 是 eBPF 的基础

```
eBPF 的 kprobe/tracepoint 机制复用了 Ftrace 的基础设施:

  eBPF program
      ↓
  bpf_attach(kprobe/tracepoint)
      ↓
  Ftrace 的 kprobe_events / trace_events 机制
      ↓
  内核插桩 (mcount/fentry/kprobe breakpoint)
```

### HFT 延迟分析工具选择

```
HFT 延迟分析工作流:

1. 初步定位: ftrace function_graph
   → 快速看调用链和时间分布
   → 找到耗时最长的函数

2. 深入分析: eBPF (bpftrace)
   → 统计某函数的延迟直方图
   → 按进程/CPU分组统计
   → 自定义条件过滤

3. 持续监控: eBPF (bcc tools)
   → 低开销生产环境监控
   → 延迟告警
```

### HFT 关联应用

```bash
# HFT: ftrace 初步定位
echo function_graph > /sys/kernel/tracing/current_tracer
echo 'my_trade_handler' > set_graph_function
echo 1 > tracing_on
# ... 触发交易 ...
echo 0 > tracing_on
cat trace | grep 'us |' | sort -rn | head -10
# 发现: schedule() 耗时 45μs ← 延迟来源

# HFT: eBPF 深入分析
bpftrace -e '
  kprobe:schedule { @start[tid] = nsecs; }
  kretprobe:schedule /@start[tid]/ {
    @us = hist((nsecs - @start[tid]) / 1000);
    delete(@start[tid]);
  }'
# 输出 schedule() 耗时直方图，按区间统计
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么 eBPF 正在逐步替代 Ftrace 的部分功能？

> eBPF 提供更灵活的数据处理（map 聚合、直方图、条件逻辑）、更低的开销（JIT 编译）、更好的安全性（验证器）。Ftrace 的预定义 tracer 功能固定，无法自定义聚合逻辑。但 Ftrace 仍然是 eBPF 的底层基础。

**Q2:** ftrace 和 eBPF 在内核追踪中的定位分别是什么？

> ftrace：内核内建，零依赖，适合快速函数追踪和事件记录。eBPF：可编程，适合复杂条件过滤、聚合统计、自定义分析。简单追踪用 ftrace，复杂分析用 eBPF。两者可以共存——eBPF 可以 attach 到 ftrace 的 tracepoint。

**Q3:** HFT 延迟分析应该用 ftrace 还是 eBPF？

> 初步定位用 ftrace function_graph（快速看调用链和时间分布）。深入分析用 eBPF（如统计某函数的延迟直方图、过滤特定条件）。ftrace 的 function_graph 对 HFT 最重要的用途是找到交易路径上的调度延迟和锁等待。

**Q4:** eBPF 的验证器有什么作用？

> 验证器在加载 eBPF 程序时检查安全性：(1) 确保程序在有限时间内结束（无死循环）；(2) 确保内存访问安全（不越界）；(3) 确保不会崩溃内核。这使得 eBPF 可以安全地在生产环境运行，不需要 root 权限安装内核模块。

**Q5:** Ftrace 的 function_graph 为什么不可被 eBPF 替代？

> function_graph tracer 提供的调用链可视化（缩进树形 + 耗时标注）是 eBPF 无法直接实现的。eBPF 可以测量单个函数的耗时，但无法自动展示完整的嵌套调用树。KernelShark 的 GUI 可视化也是 ftrace 独有的优势。

</details>

## 交叉引用

- [05.6 ch09 Ftrace 架构与 tracefs](chapter-09-ftrace/notes/01-ftrace-architecture-tracefs.md)
- [05.6 ch09 函数图追踪 function_graph](chapter-09-ftrace/notes/03-function-graph-tracer.md)
- [05.6 ch09 perf-tools ftrace wrapper](chapter-09-ftrace/notes/07-perf-tools-ftrace.md)
- [15-bpf-observability](../../../15-bpf-observability/)
