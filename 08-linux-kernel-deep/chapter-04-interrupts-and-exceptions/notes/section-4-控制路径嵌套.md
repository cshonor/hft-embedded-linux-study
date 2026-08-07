## 4. 内核控制路径的嵌套执行

---

### 一、什么是内核控制路径

发生中断/异常时：

1. 暂停当前进程的用户态（或内核态）执行  
2. 在 **内核态** 跑一段处理代码  
3. 这段序列 = **内核控制路径 (Kernel control path)**

代表**当前进程**在内核里执行 — 与 Ch 3 的「进程在内核态」一致。

---

### 二、嵌套规则（Linux 2.6）

为提高设备吞吐量、简化 IRQ 优先级配置，Linux 允许 **控制路径嵌套**：

| 规则 | 说明 |
|------|------|
| **中断可打断中断** | 正在跑的 ISR 可被 **更高优先级硬件中断** 打断 |
| **异常不能抢占 ISR** | 异常处理程序 **永远不能** 抢占中断处理程序 |

→ 临界区与锁：[Ch 5 内核同步](../chapter-05-kernel-synchronization.md)

---

### 三、HFT 关联

- 网卡 **硬中断** → softirq → 协议栈：整条链都是嵌套控制路径  
- 中断关闭时间过长 → 丢包、抖动 — DPDK 等方案 **绕过** 部分内核中断路径（见 [14 DPDK](../../../18-dpdk/)）

### 常见陷阱

1. 以为中断可以无限嵌套——现代内核限制了嵌套深度，且 hard IRQ 中不可嵌套同号中断
2. 混淆「中断上下文」和「进程上下文」——中断上下文无 `task_struct`、不可睡眠、不可调度
3. 以为 `local_irq_disable()` 只禁当前 CPU——确实只禁本地 CPU，其他 CPU 仍可收中断

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** 内核如何跟踪中断嵌套深度？

<details><summary>答案</summary>

`current->preempt_count` 中有 `HARDIRQ_OFFSET`（8-12 位）和 `SOFTIRQ_OFFSET`（13-16 位）。每进一层 hard IRQ，`preempt_count` += `HARDIRQ_OFFSET`；退出时 -=。`in_irq()` 检查是否在 hard IRQ，`in_softirq()` 检查是否在 softirq。`in_interrupt()` 检查任意中断上下文。

</details>

**Q2.** `local_irq_disable()` / `local_irq_save()` 的区别？

<details><summary>答案</summary>

`local_irq_disable()` 无条件关中断，不保存之前的状态——如果你不知道调用前中断是否已关，用这个可能破坏调用者的状态。`local_irq_save(flags)` 保存 `RFLAGS.IF` 到 `flags` 再关中断，`local_irq_restore(flags)` 恢复。内核代码应始终用 `_save`/`_restore` 版本。

</details>

**Q3.** 为什么 HFT 在用户态也要关心中断嵌套？

<details><summary>答案</summary>

用户态虽然不直接处理中断，但中断会抢占用户线程的 CPU 执行。一次 NIC 硬中断 → softirq → 其他进程被调度，可能导致交易线程停顿数十微秒。解决方案：① `isolcpus` 隔离交易核，中断路由到其他核。② `preempt=full` + `SCHED_FIFO` 让交易线程不可被抢占。③ DPDK 用户态轮询完全绕过中断。

</details>

</details>

---

← [3. IDT](./section-3-IDT与门描述符.md) · 下一节 [5. 异常处理](./section-5-异常处理.md)
