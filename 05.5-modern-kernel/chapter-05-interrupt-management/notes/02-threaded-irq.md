# Threaded IRQ — 中断线程化

> **原文:** [Threaded interrupt handlers](https://lwn.net/Articles/302043/) (LWN, 2008)
> **作者:** Thomas Gleixner
> **内核版本:** 2.6.30+ (引入), 6.x (PREEMPT_RT 依赖)
> **对标旧书:** ULK3 Ch4 (中断上下文处理)

---

## 核心观点

传统中断处理在**硬中断上下文** (hardirq) 中执行，不能睡眠、不能持锁过久。Threaded IRQ 将中断处理拆分为**硬中断上半部** (acknowledge + 唤醒) 和**内核线程下半部** (实际处理)，使中断处理可以睡眠和被抢占。

### 传统中断处理的问题

```c
// 传统方式 — 硬中断上下文中处理
irqreturn_t my_handler(int irq, void *dev) {
    // 在硬中断上下文执行
    // 不能睡眠！不能持互斥锁！不能做耗时操作！
    process_data(dev);  // 如果耗时，会阻塞其他中断
    return IRQ_HANDLED;
}
request_irq(irq, my_handler, 0, "my_dev", dev);
```

问题：
- 硬中断上下文不能睡眠，限制了处理逻辑
- 耗时的中断处理会阻塞同核其他中断
- 不支持 PREEMPT_RT（实时内核要求中断可抢占）

### Threaded IRQ 模型

```c
// 硿中断上半部 — 只做最少的 ack + 返回
irqreturn_t my_hardirq(int irq, void *dev) {
    if (!check_interrupt_status(dev))
        return IRQ_NONE;
    disable_irq_nosync(irq);  // 禁止中断
    return IRQ_WAKE_THREAD;    // 唤醒线程
}

// 内核线程下半部 — 可以睡眠、可以持锁
irqreturn_t my_thread_fn(int irq, void *dev) {
    // 在内核线程中执行，可以睡眠！
    process_data(dev);
    enable_irq(irq);  // 重新使能中断
    return IRQ_HANDLED;
}

request_threaded_irq(irq, my_hardirq, my_thread_fn,
                     IRQF_ONESHOT, "my_dev", dev);
```

### 关键标志

| 标志 | 含义 |
|------|------|
| `IRQF_ONESHOT` | 硬中断后保持中断禁用直到线程处理完成 |
| `IRQF_TRIGGER_*` | 触发类型（边沿/电平） |
| 0 (无标志) | 默认行为 |

### PREEMPT_RT 的依赖

PREEMPT_RT (实时内核) **强制将所有中断线程化**，因为：
- 实时内核要求高优先级线程能抢占中断处理
- 线程化的中断有调度优先级，可以被 RT 线程抢占
- 线程名通常为 `irq/XX-devname`，可通过 `/proc/<pid>/` 查看和调整优先级

```
# 查看中断线程
$ ps -eo pid,comm,ni,pri | grep irq/
  42 irq/29-brcmv7   0  20
  43 irq/31-mmc1     0  20
```

---

## 与旧书差异

| ULK3 讲的 | 6.x 现代实现 |
|-----------|-------------|
| 中断处理全在 hardirq | 拆分为 hardirq + thread |
| `request_irq()` | `request_threaded_irq()` |
| tasklet / softirq 做下半部 | threaded IRQ 替代大部分 tasklet |
| 不支持 PREEMPT_RT | PREEMPT_RT 强制线程化所有中断 |
| 中断不能被抢占 | 线程化中断可被 RT 线程抢占 |

### tasklet 的废弃

6.x 内核逐步废弃 tasklet，推荐用 threaded IRQ 替代：

```c
// 旧方式 (废弃中)
void my_tasklet_func(unsigned long data) { ... }
DECLARE_TASKLET(my_tasklet, my_tasklet_func, 0);
// 在 hardirq 中: tasklet_schedule(&my_tasklet);

// 新方式 (推荐)
// 用 request_threaded_irq() 替代
```

---

## HFT 关联

| 场景 | Threaded IRQ 影响 |
|------|-------------------|
| **网卡中断** | 线程化后网卡中断可被交易线程抢占，减少延迟 |
| **PREEMPT_RT** | 实时内核依赖 threaded IRQ，交易线程优先级 > 中断线程 |
| **中断绑核** | 中断线程可设 CPU affinity，与交易线程隔离 |
| **IRQ 优先级** | `chrt -f 80` 提高中断线程优先级，或降低让交易线程优先 |

> **HFT 实盘：** PREEMPT_RT 内核下，网卡中断线程设为 SCHED_FIFO 优先级 50，交易线程设为 SCHED_FIFO 优先级 80。这样**交易线程可以抢占网卡中断处理**，保证行情处理不受网卡中断干扰。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** Threaded IRQ 的硬中断上半部为什么只做 `return IRQ_WAKE_THREAD`？

> 硬中断上下文不能睡眠、不能持互斥锁。上半部只做最小工作（检查中断源、ack 硬件），然后唤醒内核线程处理实际逻辑。这样上半部极短，不会阻塞其他中断，下半部可以睡眠和持锁。

**Q2:** `IRQF_ONESHOT` 标志的作用是什么？不加会怎样？

> `IRQF_ONESHOT` 保持中断在硬中断处理后禁用，直到线程处理完成。不加的话，如果中断是电平触发的，硬件会持续触发中断导致线程被反复唤醒。对于边沿触发中断可能不需要，但电平触发必须加。

**Q3:** PREEMPT_RT 为什么强制所有中断线程化？

> 实时内核需要保证高优先级线程的延迟可预测。如果中断在 hardirq 上下文执行，即使最高优先级的 RT 线程也无法抢占它。线程化后中断有调度优先级（通常 SCHED_FIFO 50），RT 线程（SCHED_FIFO 80+）可以抢占它。

</details>
