# ftrace 现代增强：6.x 追踪框架

> 对标旧书: LKD3 Ch18 (ftrace 刚出现)
> 6.x 变化: tracer 种类大增, per-instance, hist trigger, BPF 集成

---

## ftrace 在 6.x 的增强

ftrace (Function Tracer) 是内核内置的追踪框架，通过 `/sys/kernel/tracing/` 接口操作，不需要安装任何用户态工具。

| 特性 | 2.6 (LKD3 时代) | 6.x |
|------|-----------------|-----|
| tracer 种类 | function, sched_switch | function, function_graph, wakeup, hwlat, irqsoff, preemptoff, wakeup_rt |
| 事件追踪 | 少量 tracepoint | 数百个 tracepoint (sched/irq/net/block/syscalls) |
| 过滤 | 基本函数过滤 | per-event filter、触发器 (trigger) |
| 实例 | 全局唯一 | per-instance (`trace_instances`) |
| BPF 集成 | 无 | ftrace event 可被 BPF 程序消费 |
| histogram | 无 | `hist` trigger（直方图统计） |

---

## 常用操作

```bash
# 1. 查看可用 tracer
cat /sys/kernel/tracing/available_tracers
# hwlat blk function_graph wakeup_rt wakeup function nocount ...

# 2. 追踪特定函数的调用者
echo function > /sys/kernel/tracing/current_tracer
echo ixgbe_xmit_frame > /sys/kernel/tracing/set_ftrace_filter
echo 1 > /sys/kernel/tracing/tracing_on
cat /sys/kernel/tracing/trace

# 3. 函数调用图（含子函数+执行时间）
echo function_graph > /sys/kernel/tracing/current_tracer
echo ixgbe_xmit_frame > /sys/kernel/tracing/set_graph_function
cat /sys/kernel/tracing/trace
# 输出示例:
# 1) ! 283.412 us  |  ixgbe_xmit_frame() {
# 2)   2.130 us    |    netdev_tx_sent_queue();
# 3) ! 285.631 us  |  }

# 4. 追踪调度切换事件
echo sched:sched_switch > /sys/kernel/tracing/set_event
echo 1 > /sys/kernel/tracing/tracing_on
cat /sys/kernel/tracing/trace_pipe

# 5. 中断关闭延迟（HFT 关键指标）
echo irqsoff > /sys/kernel/tracing/current_tracer
cat /sys/kernel/tracing/tracing_max_latency
# 如果 > 100μs 说明有中断被关闭太久

# 6. 直方图统计（6.x 新增）
echo 'hist:keys=common_pid:vals=hitcount' > /sys/kernel/tracing/events/sched/sched_switch/trigger
cat /sys/kernel/tracing/events/sched/sched_switch/hist
```

### per-instance (trace_instances)

```bash
# 6.x: 创建独立追踪实例, 互不干扰
mkdir /sys/kernel/tracing/instances/hft_trace
# 现在可以在 hft_trace 目录下独立设置 tracer
echo function > /sys/kernel/tracing/instances/hft_trace/current_tracer
echo 1 > /sys/kernel/tracing/instances/hft_trace/tracing_on

# 另一个实例同时用不同 tracer
mkdir /sys/kernel/tracing/instances/irq_trace
echo irqsoff > /sys/kernel/tracing/instances/irq_trace/current_tracer
```

---

## HFT 关键 tracer

| Tracer | 追踪什么 | HFT 用途 | 阈值 |
|--------|----------|----------|------|
| `wakeup` | 进程唤醒到被调度的延迟 | 交易线程被唤醒后多久开始跑 | < 50μs |
| `wakeup_rt` | 实时线程唤醒延迟 | SCHED_FIFO 交易线程的调度延迟 | < 50μs |
| `irqsoff` | 中断关闭时长 | 关中断太久导致丢包/延迟 | < 100μs |
| `preemptoff` | 抢占关闭时长 | 不可抢占区段导致调度延迟 | < 100μs |
| `function_graph` | 函数调用链+耗时 | 驱动/内核热路径耗时分析 | — |
| `hwlat` | 硬件延迟（SMI 干扰） | BIOS/SMI 偷走 CPU 时间 | < 50μs |

```bash
# HFT 延迟排查: wakeup_rt
echo wakeup_rt > /sys/kernel/tracing/current_tracer
echo 1 > /sys/kernel/tracing/tracing_on
# 运行交易引擎...
cat /sys/kernel/tracing/tracing_max_latency
# 查看 trace 内容, 定位延迟发生在哪个函数

# HFT 延迟排查: irqsoff
echo irqsoff > /sys/kernel/tracing/current_tracer
echo 1 > /sys/kernel/tracing/tracing_on
# 运行交易引擎...
cat /sys/kernel/tracing/tracing_max_latency
# 如果 > 100μs, trace 会显示关中断的代码路径
```

> **HFT 生产必备：** `wakeup_rt` + `irqsoff` 是交易系统延迟排查的两大法宝。`wakeup_rt > 50μs` 说明调度器有问题；`irqsoff > 100μs` 说明关中断太久。

---

## 与 trace-cmd / KernelShark 的关系

```bash
# trace-cmd 是 ftrace 的命令行前端 (封装 tracefs 操作)
trace-cmd record -e sched:sched_switch -e sched:sched_wakeup sleep 10
trace-cmd report > trace.txt

# KernelShark 是 GUI 前端
trace-cmd record -e sched:* sleep 10
kernelshark trace.dat

# 对比 05.6-kernel-debugging/chapter-09-ftrace 的 trace-cmd 章节
```

---

## 自测题

<details>
<summary>Q1: ftrace 的 wakeup_rt tracer 和 wakeup tracer 有什么区别？</summary>

`wakeup` 追踪普通进程（SCHED_NORMAL）的唤醒延迟。`wakeup_rt` 只追踪实时进程（SCHED_FIFO/SCHED_RR）的唤醒延迟。HFT 交易线程用 SCHED_FIFO，所以用 `wakeup_rt`。两者使用相同的机制（记录唤醒时间戳 → 等到被调度 → 计算差值），只是过滤的调度策略不同。
</details>

<details>
<summary>Q2: trace_instances 有什么用？为什么 HFT 需要它？</summary>

trace_instances 允许创建多个独立的追踪实例，每个实例有自己的 tracer 设置、事件过滤和 trace buffer。HFT 场景中可以同时运行多个 tracer：一个实例用 wakeup_rt 追踪调度延迟，另一个用 function_graph 追踪网卡驱动耗时。互不干扰，各自有独立的 buffer。
</details>

<details>
<summary>Q3: irqsoff tracer 显示 max latency = 350μs，如何定位是哪段代码？</summary>

irqsoff tracer 在记录 max latency 时会保存当时的 trace，包含关中断和开中断的函数调用栈。`cat /sys/kernel/tracing/trace` 可以看到延迟发生在哪个函数。常见的关中断过长的原因：spin_lock_irqsave 临界区过长、IRQ handler 执行太多工作、softirq 处理链过长。PREEMPT_RT 通过中断线程化解决此问题。
</details>

---

## 交叉引用

- [01-vdso.md](./01-vdso.md) — vDSO 与系统调用加速
- [03-ebpf-observability.md](./03-ebpf-observability.md) — eBPF 可编程追踪
- [05.6-kernel-debugging/chapter-09-ftrace](../../05.6-kernel-debugging/chapter-09-ftrace/) — ftrace 完整教程
