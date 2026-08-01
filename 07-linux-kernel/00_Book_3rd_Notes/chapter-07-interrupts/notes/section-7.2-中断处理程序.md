## ② 中断处理程序 · Interrupt Handlers (ISR)

**中断处理程序（interrupt handler）** 也叫 **ISR（Interrupt Service Routine）** — 是 **设备驱动** 里响应硬件 IRQ 的 **C 函数**，由内核在中断到来时 **直接调用**。

| 项 | 说明 |
|----|------|
| **谁写** | **设备驱动作者** — 每个使用 IRQ 的设备至少一个 handler |
| **本质** | 普通 **C 函数**，签名固定（见下） |
| **运行环境** | **中断上下文（interrupt context / hardirq context）** |
| **核心要求** | **尽可能快** — 尽快 ACK 硬件、恢复被中断代码 |
| **返回值** | `irqreturn_t` — `IRQ_HANDLED` / `IRQ_NONE`（共享 IRQ 时区分是否本设备） |

#### 典型函数签名

```c
static irqreturn_t my_interrupt(int irq, void *dev_id)
{
    struct my_dev *dev = dev_id;

    if (!my_hw_is_ours(dev))
        return IRQ_NONE;          /* 共享 IRQ：不是本设备 */

    my_hw_ack(dev);               /* 应答硬件 — 通常在上半部最前 */
    /* 最小工作：读状态寄存器、摘环、入队… */
    tasklet_schedule(&dev->t);    /* 重活推到下半部 — Ch 8 */

    return IRQ_HANDLED;
}
```

#### ISR 里该做什么 / 不该做什么

| 应该做 | 不应该做 |
|--------|----------|
| **应答（ACK）** 中断控制器/设备 | **`mutex_lock`**、等信号量 — 会睡眠 |
| 读 **少量** 硬件寄存器确认事件 | **大块 `kmalloc(GFP_KERNEL)`** — 可能触发回收 |
| 把数据 **摘环** 或 **入队** 到 lock-free/per-CPU 缓冲 | **`printk` 大量输出** — 慢且可能死锁 |
| **调度下半部**（tasklet / softirq / workqueue） | **用户态拷贝**、复杂协议解析 |

```
设备 ──IRQ 线──► 中断控制器 ──向量──► CPU
                                      │
                                      ▼
                              ISR（驱动注册）
                                      │
                    ┌─────────────────┼─────────────────┐
                    ▼                 ▼                 ▼
               ACK 硬件          读状态/摘环        schedule 下半部
                    │                 │                 │
                    └─────────────────┴─────────────────┘
                                      │
                              尽快 return ──► 恢复被中断代码
```

#### 共享 IRQ 线

| 场景 | 行为 |
|------|------|
| **多设备共用一条物理线** | 注册时设 **`IRQF_SHARED`** — 内核 **链式调用** 所有 handler |
| **每个 handler 必须快速判断** | 不是本设备 → 立刻 `IRQ_NONE` |
| **至少一个返回 `IRQ_HANDLED`** | 否则内核认为 IRQ 未被服务（spurious 调试） |

#### 性能与正确性

| 事实 | 推论 |
|------|------|
| 处理某 IRQ 线时 **该线通常被屏蔽** | 同一条线上 **不会重入** 同一 handler |
| **不同 IRQ 线**、**多 CPU** 可并发 | 共享数据结构 **必须加锁**（Ch 9） |
| ISR 与 **下半部、进程上下文** 共享数据 | 需要 `spin_lock_irqsave` 等（Ch 7.7 / Ch 8.8） |

**HFT：** 网卡 **每包一次 IRQ**（或未开合并中断）时，ISR + 上半部过长 → **P99 尾延迟**、L1/L2 cache 被冲刷。生产环境常见：**IRQ affinity 绑专用核**、**中断合并**、**NAPI 在 softirq 批量收包**。

→ [03 SysPerf §1.5 IRQ 与策略同核](../../../../19-systems-performance/chapter-01-intro/notes/section-1.5-排障案例与性能挑战.md) · [Ch 8](../../chapter-08-bottom-halves/) 下半部 · [Ch 7.4](section-7.4-注册与编写中断处理程序.md) `request_irq`

---
