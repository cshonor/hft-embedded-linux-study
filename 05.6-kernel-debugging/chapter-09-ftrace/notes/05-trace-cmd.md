# trace-cmd：命令行前端

> 🔴 精读

## 概念详解

### trace-cmd 是什么

trace-cmd 是 ftrace 的命令行封装工具，简化 tracefs 操作流程。一条命令完成"选 tracer → 设 filter → 启动 → 停止 → 导出"的全过程。

### 基本用法

```bash
# 安装
sudo apt install trace-cmd

# 记录调度事件
trace-cmd record -e sched sched_switch -e irq irq_handler_entry sleep 5
trace-cmd report > trace.txt

# 函数追踪
trace-cmd record -p function -l schedule sleep 1
trace-cmd report

# function_graph
trace-cmd record -p function_graph -l my_driver_write sleep 1
trace-cmd report

# kprobe
trace-cmd record -e p:my_probe schedule sleep 1
trace-cmd report

# 带过滤
trace-cmd record -e sched_switch -f 'prev_pid == 1234' sleep 5
```

### trace-cmd 常用选项

| 选项 | 含义 | 示例 |
|------|------|------|
| `record` | 记录追踪数据 | `trace-cmd record -p function sleep 1` |
| `report` | 报告追踪数据 | `trace-cmd report trace.dat` |
| `-p TRACER` | 选择 tracer | `-p function` / `-p function_graph` |
| `-l FUNC` | 过滤函数 | `-l schedule` |
| `-e EVENT` | 启用事件 | `-e sched_switch` |
| `-f FILTER` | 事件过滤 | `-f 'prev_pid == 1234'` |
| `-P PID` | 进程过滤 | `-P 1234` |
| `-o FILE` | 输出文件 | `-o trace.dat` |

### trace-cmd 优势

| 特性 | 直接操作 tracefs | trace-cmd |
|------|-----------------|-----------|
| 事件配置 | 手动 echo | 命令行参数 |
| 数据保存 | cat trace > file | 自动保存 .dat |
| 多实例 | 复杂 | trace-cmd stream |
| 报告生成 | 手动解析 | trace-cmd report |
| 多事件同时 | 多个 echo | 一条命令 |

### trace-cmd 工作流

```bash
# 1. 记录（自动保存到 trace.dat）
trace-cmd record -e sched -e irq -e kmem sleep 10

# 2. 查看报告
trace-cmd report | less

# 3. 提取特定事件
trace-cmd report | grep sched_switch

# 4. 保存为文本
trace-cmd report > trace.txt

# 5. 重新分析（不需要重新采集）
trace-cmd report trace.dat
```

### trace-cmd stream（实时流）

```bash
# 实时流式输出（不保存到文件）
trace-cmd stream -e sched_switch

# 适合实时监控场景
```

### trace-cmd extract（从运行中的 ftrace 提取）

```bash
# 如果已经手动设置了 ftrace 并在运行
echo function > /sys/kernel/tracing/current_tracer
echo 1 > /sys/kernel/tracing/tracing_on
sleep 5
echo 0 > /sys/kernel/tracing/tracing_on

# 用 trace-cmd 提取数据
trace-cmd extract -o trace.dat
trace-cmd report trace.dat
```

### HFT 关联应用

```bash
# HFT: 一条命令采集完整的交易延迟分析数据
trace-cmd record \
    -e sched_switch \
    -e irq_handler_entry \
    -e irq_handler_exit \
    -e netif_receive_skb \
    -p function_graph \
    -l my_trade_handler \
    -P $(pidof trade_app) \
    sleep 10

trace-cmd report > /tmp/hft_trace.txt

# 分析调度延迟
grep sched_switch /tmp/hft_trace.txt | head -20

# 分析函数耗时
grep 'us |' /tmp/hft_trace.txt | sort -rn | head -20
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** trace-cmd 生成的 .dat 文件如何分析？

> 用 `trace-cmd report trace.dat` 转为可读文本。也可以用 KernelShark（GUI 工具）打开 .dat 文件进行可视化分析。.dat 格式是二进制的，包含完整的事件数据和元数据。

**Q2:** trace-cmd record 和直接操作 tracefs 相比有什么优势？

> trace-cmd 封装了 tracefs 操作流程（选 tracer → 设 filter → 启动 → 停止 → 导出），一条命令完成。还支持多事件同时记录、自动保存元数据。

**Q3:** trace-cmd extract 有什么用？

> 如果已经手动设置了 ftrace 并在运行，可以用 `trace-cmd extract` 将当前 ring buffer 中的数据提取到 .dat 文件。不需要重新采集。

**Q4:** trace-cmd stream 和 record 的区别？

> `record` 将数据保存到 .dat 文件（适合事后分析）。`stream` 实时输出到终端（适合实时监控），不保存文件。

**Q5:** HFT 中如何用 trace-cmd 一次性采集多种数据？

> 用多个 `-e` 选项启用事件，同时用 `-p` 选择 tracer，用 `-l` 过滤函数，用 `-P` 过滤进程。一条命令同时采集调度事件、中断事件、网络事件和函数调用图。

</details>

## 交叉引用

- [05.6 ch09 Ftrace 架构与 tracefs](../../chapter-09-ftrace/notes/01-ftrace-architecture-tracefs.md)
- [05.6 ch09 KernelShark GUI 前端](../../chapter-09-ftrace/notes/06-kernelshark.md)
- [05.6 ch09 perf-tools ftrace wrapper](../../chapter-09-ftrace/notes/07-perf-tools-ftrace.md)
