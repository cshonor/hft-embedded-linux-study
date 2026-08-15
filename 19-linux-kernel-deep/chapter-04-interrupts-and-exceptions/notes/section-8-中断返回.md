## 8. 从中断与异常中返回

> 返回用户态前，内核要做一系列 **关键检查**

---

### 一、返回路径

处理完毕后经：

- **`ret_from_intr()`** — 从中断返回  
- **`ret_from_exception()`** — 从异常返回  

---

### 二、返回前检查什么

| 检查项 | 后果 |
|--------|------|
| **Pending reschedule** | 是否需要 **进程调度**（`need_resched`） |
| **挂起的信号** | 是否投递给当前进程 |
| **单步调试** | 调试器相关状态 |

这就是为什么 **定时器中断** 能驱动调度 — tick 里可能置 `need_resched`，返回时切进程。

→ 深潜：[Ch 7 进程调度](../chapter-07-process-scheduling.md) · [Ch 11 信号](../chapter-11-signals.md)

---

### 三、后续章节索引

| Ch 4 主题 | 继续读 |
|-----------|--------|
| 锁、临界区、SMP | [Ch 5 内核同步](../chapter-05-kernel-synchronization.md) 🔴 |
| 调度、tick | [Ch 7 进程调度](../chapter-07-process-scheduling.md) 🔴 |
| `int 0x80` / syscall | [Ch 10 系统调用](../chapter-10-system-calls.md) 🔴 |
| 信号投递 | [Ch 11 信号](../chapter-11-signals.md) 🟡 |
| 设备驱动、IRQ 注册 | [Ch 13 I/O 架构](../chapter-13-io-architecture.md) ⚪ |
| 内核路径 profiling | [04 BPF](../../../16-bpf-observability/) |
| 用户态绕过中断 | [14 DPDK](../../../14-dpdk/) |

### 常见陷阱

1. 以为中断返回就是简单的 IRET——x86-64 用 `IRET` 但还需要处理 preempt count、need_resched、信号传递
2. 混淆中断返回到用户态和返回到内核态——返回用户态要检查 `TIF_NEED_RESCHED`/信号，返回内核态一般不重新调度
3. 以为 `local_irq_enable()` 立即响应所有 pending 中断——需要先处理 preempt count 再开中断

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** 中断返回时内核检查哪些条件？

<details><summary>答案</summary>

① `preempt_count` 归零（所有中断/锁计数退出）。② `need_resched` 标志（`TIF_NEED_RESCHED`）→ 触发 `schedule()`。③ `need_resched` + 返回用户态 → 检查 pending 信号 → `do_signal()`。④ 返回用户态 → 可能需要 `audit`/`seccomp` 检查。⑤ x86 上还要检查 `TIF_NOTIFY_RESUME`（task work）。

</details>

**Q2.** 为什么中断返回到内核态一般不重新调度？

<details><summary>答案</summary>

中断打断的是内核代码，内核代码通常持有锁或处于临界区。如果中断返回时调度到其他进程，可能导致锁持有时间过长或死锁。只有在 `preempt_count == 0`（无锁）且 `need_resched` 时才允许内核态抢占调度。`CONFIG_PREEMPT=y` 启用内核抢占，`CONFIG_PREEMPT_NONE` 禁用（服务器默认）。

</details>

**Q3.** HFT 如何利用 `nohz_full` 减少定时器中断？

<details><summary>答案</summary>

`nohz_full=N` 标记 N 号 CPU 为 full nohz，该 CPU 上只有一个任务运行时，内核停止周期性定时器中断（tickless）。效果：① 消除每秒 100/250/1000 次的 `scheduler_tick()`。② 减少上下文切换。③ 降低 cache 污染。配置：`nohz_full=2-3 isolcpus=2-3 rcu_nocbs=2-3`。注意：该 CPU 上不能有多个竞争 CPU 的任务。

</details>

</details>

---

← [7. 可延迟函数](./section-7-可延迟函数与工作队列.md) · 下一章 [Ch 5 内核同步](../chapter-05-kernel-synchronization.md)
