## ④ 注册与编写中断处理程序

驱动通过 **`request_irq()`** 向内核 **注册** ISR；卸载时用 **`free_irq()`** 注销。这是 **probe/open** 与 **remove/close** 的标准配对操作。

#### 注册 / 注销 API

```c
/* 注册 */
int request_irq(unsigned int irq,
                irqreturn_t (*handler)(int, void *),
                unsigned long flags,
                const char *name,
                void *dev);

/* 注销 — dev 必须与注册时相同（共享 IRQ 时用于匹配） */
void free_irq(unsigned int irq, void *dev);
```

| 参数 | 含义 |
|------|------|
| **irq** | 中断号 — 来自 `platform_get_irq()`、PCI、设备树 |
| **handler** | ISR 函数指针 |
| **flags** | 行为标志（见下表） |
| **name** | 字符串 — `/proc/interrupts` 里可见，排障用 |
| **dev** | 传给 handler 的 **`void *`** — 通常传 **设备私有结构体指针** |

#### 常用 flags（LKD 3rd 时代 + 现代常见）

| 标志 | 含义 |
|------|------|
| **`IRQF_SHARED`** | 允许多个驱动 **共享** 同一 IRQ 线 — handler 必须能区分是否本设备 |
| **`IRQF_DISABLED`**（书中） | 执行 handler 时 **禁用本地所有中断** — 历史标志；现代内核已演进，新驱动少用 |
| **`IRQF_ONESHOT`**（ threaded IRQ 相关） | 线级 ONESHOT — 与 **threaded handler** 配合（书中未详述，驱动实践常见） |
| **0** | 默认 — 不共享、不禁全局中断 |

#### 返回值 · `irqreturn_t`

| 返回值 | 含义 | 使用场景 |
|--------|------|----------|
| **`IRQ_HANDLED`** | 确认是本设备触发且 **已处理** | 正常路径 |
| **`IRQ_NONE`** | **不是** 本设备或未处理 | **共享 IRQ** 链上其他设备可能处理 |
| **`IRQ_WAKE_THREAD`** | hardirq 已 ACK，唤醒 **thread fn**（threaded IRQ） | 现代驱动把重活放内核线程 |

#### 共享 IRQ 注册示例

```c
ret = request_irq(dev->irq, my_isr,
                  IRQF_SHARED, "mydev", dev);
if (ret)
    return ret;

/* 卸载 */
free_irq(dev->irq, dev);
```

#### 编写 ISR 的原则

| 原则 | 说明 |
|------|------|
| **快进快出** | 只留硬件必需操作 |
| **先 ACK 再干活**（视硬件手册） | 部分设备要求特定顺序 |
| **用 `dev_id` 取私有数据** | 避免全局变量 |
| **共享 IRQ 先判归属** | 非本设备立刻 `IRQ_NONE` |
| **重活 defer** | `tasklet_schedule` / `schedule_work` |

#### 可重入性

| 事实 | 推论 |
|------|------|
| 正在跑某 **IRQ 线** 的 ISR 时，该线 **全局屏蔽** | 同一条线上 **不会并发** 进同一 handler |
| | 相对 **该 IRQ 线**，ISR **不必写成可重入** |
| **不同 IRQ 线** 可同时进不同 ISR | 共享 **全局数据结构** 要锁 |
| **SMP 多 CPU** | 不同 CPU 可同时处理 **不同 IRQ** |

#### 常见错误

| 错误 | 后果 |
|------|------|
| `free_irq` 的 **dev** 与注册不一致 | 共享 IRQ 下无法正确注销 |
| ISR 里 **`mutex_lock`** | 睡眠 → **内核 BUG / 死锁** |
| 共享 IRQ 未设 **`IRQF_SHARED`** | `request_irq` 失败 |
| 所有 handler 都返回 **`IRQ_NONE`** | 内核 spurious IRQ 警告 |

> 仍须注意：**不同 IRQ 线**、**多 CPU** 与 **共享数据** — 要锁（→ [Ch 9](../../chapter-09-kernel-sync-intro/)）。

**HFT：** 注册 IRQ 后可用 **`/proc/interrupts`** 看 **每 CPU 计数** — 若 IRQ 全堆在 CPU0 而策略跑在 CPU0，考虑 **`/proc/irq/N/smp_affinity`** 迁核。

→ [Ch 7.5](section-7.5-中断上下文.md) 中断上下文 · [Ch 8.4](../../chapter-08-bottom-halves/notes/section-8.4-tasklet.md) tasklet · [Ch 6](../../chapter-06-kernel-data-structures/) kfifo 入队模式

---
