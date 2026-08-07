# 10. BPF 单行命令 (One-Liners)

本章末尾示例 — 与 [Ch 5](../../chapter-05-bpftrace/)、[附录 A](../../appendix-A-bpftrace单行命令.md) 衔接：

```bash
# 全系统 CPU 栈采样（bpftrace）
bpftrace -e 'profile:hz:99 { @[kstack] = count(); }'

# 上下文切换时的内核栈
bpftrace -e 'tracepoint:sched:sched_switch { @[kstack] = count(); }'

# 某 PID 的 on-CPU 用户栈
bpftrace -e 'profile:hz:99 /pid == 1234/ { @[ustack] = count(); }'

# runqlat 等价思路（教学用；生产直接用 runqlat-bpfcc）
# 见 man runqlat-bpfcc
```

**原则：** 固定场景用 **BCC 工具**（已优化、有 man）；**验证假设** 用 bpftrace 单行。


### 常见陷阱

1. **直接复制 one-liner 不评估开销** — CPU 相关的 one-liner 有些开销较大（如全系统 sched_switch 追踪）；运行前应评估目标事件频率
2. **忽视 one-liner 的输出量** — 某些 one-liner（如逐事件打印 sched_switch）在繁忙系统上每秒产生数千行输出；应加 filter 或用 Map 聚合
3. **只记命令不理解 sched tracepoint 字段** — sched_switch 的 format 文件描述了 prev_comm/next_comm/prev_pid/next_pid 等字段；不看 format 写出的 one-liner 可能引用错误字段

<details>
<summary>📝 自测题（点击展开）</summary>

1. **CPU 分析最常用的 3 个 bpftrace one-liner 是什么？**

   <details>
   <summary>参考答案</summary>

   (1) 调度延迟直方图：`tracepoint:sched:sched_wakeup { @s[tid]=nsecs } tracepoint:sched:sched_switch /@s[args->next_pid]/ { @runqlat=hist(nsecs-@s[args->next_pid]); delete(@s[args->next_pid]) }`；(2) CPU 采样火焰图：`profile:hz:99 { @[kstack] = count() }`；(3) 上下文切换统计：`tracepoint:sched:sched_switch { @[args->prev_comm, args->next_comm] = count() }`。

   </details>

2. **如何查看 sched_switch tracepoint 有哪些可用字段？**

   <details>
   <summary>参考答案</summary>

   `cat /sys/kernel/debug/tracing/events/sched/sched_switch/format`。常见字段：prev_comm（切出线程名）、prev_pid、prev_prio、prev_state（线程状态）、next_comm（切入线程名）、next_pid、next_prio。在 bpftrace 中用 `args->prev_comm`、`args->next_pid` 等访问。

   </details>

3. **HFT 场景中，如何用 one-liner 快速判断调度问题？**

   <details>
   <summary>参考答案</summary>

   三步：(1) runqlat 直方图看调度延迟分布——如果尾部有毫秒级异常，说明有调度抖动；(2) sched_switch 按线程对统计——看 HFT 线程被谁抢占；(3) irq_handler 频率统计——看中断是否集中在 HFT 核上。三行命令快速定位「调度延迟→谁抢占→是否中断导致」。

   </details>

</details>

---
