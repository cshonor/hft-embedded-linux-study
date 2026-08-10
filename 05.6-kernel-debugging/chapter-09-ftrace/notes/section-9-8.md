# 9.8 Ftrace 与 eBPF 的关系

> 🔴 精读

## 本节要点

### Ftrace vs eBPF 对比

| 特性 | Ftrace | eBPF |
|------|--------|------|
| 内核版本 | 2.6+ (广泛) | 4.x+ (5.x+ 完整) |
| 编程 | 预定义 tracer + 事件 | 自定义程序 |
| 数据处理 | 简单过滤 | map 聚合 + 直方图 |
| 性能开销 | ~100-300ns/事件 | ~50-100ns/事件 |
| 安全性 | 需 root | 验证器保证 |
| 输出 | trace 文件 | map + pipe + 事件 |

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
```

### 何时用 Ftrace vs eBPF

- **快速排查**：Ftrace（tracefs 即用，无需安装额外工具）
- **复杂数据处理**：eBPF（map 聚合、直方图、条件过滤）
- **低开销生产环境**：eBPF（开销更低，验证器保证安全）
- **内核版本 < 4.x**：Ftrace（eBPF 功能不全）

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么 eBPF 正在逐步替代 Ftrace 的部分功能？

> eBPF 提供更灵活的数据处理（map 聚合、直方图、条件逻辑）、更低的开销（JIT 编译）、更好的安全性（验证器）。Ftrace 的预定义 tracer 功能固定，无法自定义聚合逻辑。但 Ftrace 仍然是 eBPF 的底层基础——eBPF 的 kprobe/tracepoint 机制复用了 Ftrace 的基础设施。

</details>
