# perf-tools ftrace wrapper

> 🔴 精读

## 概念详解

### perf-tools 简介

perf-tools 是 Brendan Gregg 开发的 ftrace 封装脚本集合，简化常用追踪操作。它将复杂的 tracefs 操作封装为一键命令，降低了 ftrace 的使用门槛。

### 获取 perf-tools

```bash
git clone https://github.com/brendangregg/perf-tools.git
cd perf-tools/bin
```

### 常用工具

```bash
# 1. 函数调用频率统计
./funccount schedule         # 统计 schedule() 调用次数
./funccount 'vfs_*'         # 通配符统计
./funccount -d 5             # 运行 5 秒

# 2. 函数耗时
./funclatency schedule       # schedule() 耗时直方图
./funclatency -m vfs_write   # vfs_write() 耗时 (毫秒)

# 3. 调用路径
./kprobe schedule 'cpu=$cpu' # 追踪 schedule 调用时的 CPU 号

# 4. 系统调用追踪
./syscount                   # 系统调用频率统计
./syssize                    # 系统调用参数大小

# 5. I/O 工具
./iolatency                  # I/O 延迟直方图
./iosnoop                    # I/O 事件实时追踪

# 6. 前端工具
./bashreadelf                # 追踪 ELF 读取
```

### 工具对比

| 工具 | 功能 | ftrace 等价操作 |
|------|------|----------------|
| `funccount` | 函数调用频率 | set_ftrace_filter + 计数 |
| `funclatency` | 函数耗时直方图 | function_graph + 手动计算 |
| `kprobe` | kprobe 探针 | kprobe_events + filter |
| `syscount` | 系统调用统计 | trace events syscalls |
| `iolatency` | I/O 延迟 | block events + 计算 |

### funclatency 示例

```bash
./funclatency schedule
# 输出:
# Tracing schedule()... Ctrl-C to quit.
#
#   nsecs           : count     distribution
#   0 -> 999        : 0        |                                      |
#   1000 -> 1999    : 12       |******                                |
#   2000 -> 2999    : 45       |**********************                |
#   3000 -> 3999    : 78       |**************************************|
#   4000 -> 4999    : 56       |***************************           |
#   5000 -> 5999    : 23       |***********                           |
#   6000 -> 6999    : 8        |****                                  |
#   7000 -> 7999    : 3        |*                                     |
#   8000 -> 8999    : 1        |                                      |
```

### HFT 关联应用

```bash
# HFT: 分析交易路径上关键函数的调用频率和耗时
cd perf-tools/bin

# 统计网络发送频率
./funccount tcp_sendmsg -d 10

# 测量网络发送耗时
./funclatency tcp_sendmsg

# 测量调度延迟
./funclatency schedule

# 统计锁操作频率
./funccount 'spin_lock*' -d 10

# 追踪 I/O 延迟
./iolatency -D
```

### perf-tools vs trace-cmd

| 特性 | perf-tools | trace-cmd |
|------|-----------|-----------|
| 定位 | 高层封装，一键脚本 | 完整封装，支持所有 ftrace 功能 |
| 易用性 | 更易用（一条命令出结果） | 需要理解 ftrace 概念 |
| 功能 | 常用场景 | 全面 |
| 输出 | 直方图/统计 | 原始 trace 数据 |
| 适用 | 快速分析 | 深度分析 |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** perf-tools 和 trace-cmd 的定位有什么不同？

> trace-cmd 是 tracefs 的完整封装，支持所有 ftrace 功能但接口较复杂。perf-tools 是高层封装，提供常用场景的一键脚本（如 funclatency 直接输出直方图），更易用但功能有限。快速分析用 perf-tools，深度分析用 trace-cmd。

**Q2:** perf-tools 中的 funclatency 如何工作？

> 利用 ftrace 的 function_graph tracer 记录函数入口和出口时间戳，计算时间差，然后按区间分桶统计，输出直方图。一条命令完成"设 tracer → 设 filter → 追踪 → 计算 → 输出直方图"。

**Q3:** funccount 和 funclatency 分别适合什么场景？

> `funccount` 统计函数调用**次数**——适合分析函数调用频率（如 schedule 被调用多少次）。`funclatency` 统计函数**耗时**——适合分析函数执行时间分布（如 schedule 耗时直方图）。

**Q4:** perf-tools 在 HFT 中的典型用法？

> (1) `funclatency tcp_sendmsg` 测量网络发送延迟；(2) `funclatency schedule` 测量调度延迟；(3) `funccount 'spin_lock*'` 统计锁操作频率；(4) `iolatency` 分析 I/O 延迟。一键出结果，适合快速排查。

**Q5:** perf-tools 需要安装吗？依赖什么？

> 不需要安装，只需 `git clone` 后直接运行 `bin/` 目录下的脚本。依赖 bash 和 ftrace（内核内建）。某些工具可能需要特定的内核配置（如 function tracer 支持）。

</details>

## 交叉引用

- [05.6 ch09 Ftrace 架构与 tracefs](../../chapter-09-ftrace/notes/01-ftrace-architecture-tracefs.md)
- [05.6 ch09 trace-cmd 命令行前端](../../chapter-09-ftrace/notes/05-trace-cmd.md)
- [05.6 ch09 Ftrace 与 eBPF 的关系](../../chapter-09-ftrace/notes/08-ftrace-ebpf-relation.md)
