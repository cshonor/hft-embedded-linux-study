# 4.4 动态注册 Kprobes (通过 /sys)

> 🔴 精读 · Part 2: Instrumentation & Memory Debugging

## 本节要点

通过 kprobe_events 接口动态注册探针，不需要编写内核模块，是 HFT 调试中最常用的 kprobe 使用方式。

## kprobe_events 格式

```bash
# 语法:
# p:<event-name> <symbol> [offset] [<args>]
# r:<event-name> <symbol> [<args>]

# 参数格式:
# <argname>=<type>        — 简单寄存器
# <argname>=+offset(%reg) — 内存引用
# <argname>=+offset(%reg):string — 字符串
# $retval                 — 返回值 (仅 kretprobe)
# $stack, $stackN         — 栈值
# $comm                   — 当前进程名
# $arg1, $arg2, ...       — 函数参数（6.x 自动解析）
```

## 实用示例

```bash
# 1. 追踪 open 系统调用的文件名
echo 'p:my_open do_sys_openat2 dfd=%arg1 file=+0(%arg2):string' \
    > /sys/kernel/tracing/kprobe_events
echo 1 > /sys/kernel/tracing/events/kprobes/my_open/enable
cat /sys/kernel/tracing/trace_pipe

# 2. 追踪特定进程的函数调用
echo 'p:my_sched schedule' > /sys/kernel/tracing/kprobe_events
echo 'common_pid == 1234' > /sys/kernel/tracing/events/kprobes/my_sched/filter
echo 1 > /sys/kernel/tracing/events/kprobes/my_sched/enable

# 3. 测量函数耗时（entry + return 配对）
echo 'p:my_in schedule' >> /sys/kernel/tracing/kprobe_events
echo 'r:my_out schedule $retval' >> /sys/kernel/tracing/kprobe_events
echo 'hist:keys=common_pid:vals=$wallclock_ns:sort=common_pid' > \
    /sys/kernel/tracing/events/kprobes/my_out/trigger

# 4. 追踪内核函数的调用者（栈回溯）
echo 'p:my_probe schedule' > /sys/kernel/tracing/kprobe_events
echo 1 > /sys/kernel/tracing/options/stacktrace
echo 1 > /sys/kernel/tracing/events/kprobes/my_probe/enable

# 5. 条件过滤
echo 'p:my_alloc __kmalloc size=%arg1' > /sys/kernel/tracing/kprobe_events
echo 'size > 1048576' > /sys/kernel/tracing/events/kprobes/my_alloc/filter
echo 1 > /sys/kernel/tracing/events/kprobes/my_alloc/enable
# 只追踪分配 > 1MB 的请求
```

## 使用 perf probe 注册

```bash
# perf probe 提供更友好的接口
sudo perf probe --add 'schedule'
sudo perf probe --add 'schedule%return'
sudo perf probe --add 'do_sys_openat2 file=+0(%x1):string'

# 查看已注册探针
perf probe -l

# 使用 perf record 追踪
sudo perf record -e probe:schedule -a sleep 5
sudo perf report

# 删除探针
sudo perf probe --del 'schedule'
```

## filter 过滤语法

```bash
# 支持的操作符
# ==  !=  >  <  >=  <=  &&  ||

# 按进程过滤
echo 'common_pid == 1234' > filter
echo 'common_pid != 0' > filter

# 按参数过滤
echo 'size > 65536' > filter
echo 'size > 4096 && size < 65536' > filter

# 按进程名过滤
echo 'comm == "my_hft_app"' > filter

# 组合条件
echo 'common_pid == 1234 && size > 1024' > filter
```

## hist trigger 直方图

```bash
# 按进程统计调用次数
echo 'hist:keys=common_pid:vals=hitcount' > trigger

# 按进程统计耗时分布
echo 'hist:keys=common_pid:vals=$wallclock_ns' > trigger

# 按大小统计分配次数
echo 'hist:keys=size:vals=hitcount:sort=hitcount:desc' > trigger

# 按进程+大小二维统计
echo 'hist:keys=common_pid,size:vals=hitcount' > trigger

# 查看结果
cat hist
```

## HFT 关联

HFT 调试中 kprobe_events 的典型工作流：

```bash
#!/bin/bash
# hft_debug.sh: HFT 热路径调试脚本

TRACE=/sys/kernel/tracing

# 清除旧探针
echo > $TRACE/kprobe_events
echo nop > $TRACE/current_tracer

# 注册探针
echo 'p:hft_rx hft_process_packet' > $TRACE/kprobe_events
echo 'r:hft_rx_ret hft_process_packet $retval' > $TRACE/kprobe_events
echo 'p:net_rx __netif_receive_skb' > $TRACE/kprobe_events
echo 'r:net_rx_ret __netif_receive_skb $retval' > $TRACE/kprobe_events

# 启用耗时直方图
echo 'hist:keys=common_pid:vals=$wallclock_ns:sort=vals:desc' > \
    $TRACE/events/kprobes/hft_rx_ret/trigger
echo 'hist:keys=common_pid:vals=$wallclock_ns:sort=vals:desc' > \
    $TRACE/events/kprobes/net_rx_ret/trigger

# 开始追踪
echo 1 > $TRACE/tracing_on
echo "Tracing started. Press Enter to stop..."
read
echo 0 > $TRACE/tracing_on

# 查看结果
echo "=== HFT RX 耗时分布 ==="
cat $TRACE/events/kprobes/hft_rx_ret/hist
echo "=== Net RX 耗时分布 ==="
cat $TRACE/events/kprobes/net_rx_ret/hist

# 清理
echo > $TRACE/kprobe_events
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 如何只追踪特定 PID 的函数调用？

> 通过 ftrace 的 filter 机制：`echo 'common_pid == 1234' > /sys/kernel/tracing/events/kprobes/my_probe/filter`。只有 PID 1234 的进程触发 kprobe 时才会记录。支持 `==`, `!=`, `>`, `<`, `&&`, `||` 等操作符。

**Q2:** perf probe 和直接写 kprobe_events 有什么区别？

> perf probe 是 kprobe_events 的封装，提供更友好的命令行接口。它自动解析符号和行号（`perf probe --add 'schedule:15'` 可以在 schedule 函数第 15 行插入探针），支持 C 表达式（`perf probe --add 'kmalloc size'` 自动识别 size 参数）。底层仍使用 kprobe_events，但省去了手动计算偏移和参数位置的麻烦。

**Q3:** hist trigger 的 `vals=$wallclock_ns` 是什么意思？

> `$wallclock_ns` 是 kretprobe 事件的特殊变量，表示从对应的 kprobe（入口）到 kretprobe（返回）的墙钟时间差（纳秒）。`vals=$wallclock_ns` 在直方图中统计这个时间差，用于测量函数耗时分布。

**Q4:** kprobe_events 中 `>>` 和 `>` 的区别？

> `>` 覆盖写入（清除所有旧探针，只保留新的）。`>>` 追加写入（在现有探针基础上添加新的）。批量注册多个探针时用 `>>`，重新开始时用 `>`。

**Q5:** 如何在 kprobe_events 中追踪函数内部特定行？

> 直接写 kprobe_events 只支持函数名+偏移（`echo 'p:my_probe schedule+0x20' > kprobe_events`），不支持行号。需要行号级探针用 `perf probe --add 'schedule:42'`，perf probe 利用 DWARF 调试信息将行号转为地址。

</details>

## 交叉引用

- [05.6 ch04 kprobe 入口探针](chapter-04-kprobes/notes/02-kprobe-entry-handler.md)
- [05.6 ch04 perf probe](chapter-04-kprobes/notes/05-perf-probe-relation.md)
- [05.6 ch09 ftrace 事件](chapter-09-ftrace/notes/04-trace-events.md)
