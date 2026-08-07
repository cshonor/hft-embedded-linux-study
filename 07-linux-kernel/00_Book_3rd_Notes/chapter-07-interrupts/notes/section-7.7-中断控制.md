## ⑦ 中断控制 · Interrupt Control

访问与 **ISR / 下半部 / 进程** 共享的数据时，必须保证 **临界区** 不被中断打断 — 内核提供 **关/开中断** API。这是 **驱动同步** 的基础手段之一（与自旋锁、禁止 BH 组合 — Ch 9/10）。

#### 本地全部中断

| API | 说明 |
|-----|------|
| **`local_irq_disable()`** | 禁止 **本 CPU** 上 **可屏蔽硬件中断** |
| **`local_irq_enable()`** | 重新开启 |

| 特点 | 说明 |
|------|------|
| **只影响当前 CPU** | 其他 CPU 仍收中断 — SMP 下还需 **自旋锁** |
| **可嵌套问题** | 若外层已关、内层 `enable` 会 **误开** — 见 save/restore |

#### 推荐：保存/恢复（可嵌套安全）

| API | 说明 |
|-----|------|
| **`local_irq_save(flags)`** | 关中断前 **保存** 原中断状态到 `flags` |
| **`local_irq_restore(flags)`** | **恢复** 之前状态 — 不误开原本就关的中断 |

```c
unsigned long flags;

local_irq_save(flags);
/* 临界区 — 本 CPU 不会被硬 IRQ 打断 */
shared_data_update();
local_irq_restore(flags);
```

#### 与自旋锁的组合（最常用）

```c
spin_lock_irqsave(&lock, flags);
/* 临界区 — 其他 CPU 自旋 + 本 CPU 不收硬 IRQ */
spin_unlock_irqrestore(&lock, flags);
```

| 组合 | 防什么 |
|------|--------|
| **`spin_lock`  alone** | 其他 CPU 并发 |
| **`+ local_irq_save`** | 本 CPU 上 **ISR 抢同一把锁** → 死锁 |
| **`spin_lock_bh`** | 关 **softirq/tasklet** 但不关硬 IRQ（见 Ch 8.8） |

#### 单条 IRQ 线（全局）

| API | 作用 |
|-----|------|
| **`disable_irq(irq)`** | **全局** 屏蔽 **特定 IRQ 线** — 所有 CPU 上该线不再触发 handler |
| **`disable_irq_nosync(irq)`** | 屏蔽但不 **等待** 当前 handler 结束 |
| **`enable_irq(irq)`** | 重新启用该线 |
| **`synchronize_irq(irq)`** | **等待** 该 IRQ 上所有 in-flight handler 完成 |

```
disable_irq() 使用场景（概念）：
  驱动 remove / 热 unplug
      │
      ▼
  disable_irq() ──► 不再有新 ISR
      │
      ▼
  synchronize_irq() ──► 等正在跑的 ISR 结束
      │
      ▼
  free_irq() 安全
```

| API 对比 | 范围 | 典型用途 |
|----------|------|----------|
| **`local_irq_*`** | **本 CPU** 全部 IRQ | 短临界区、与 spinlock 配合 |
| **`disable_irq()`** | **全局** 单条 IRQ 线 | 卸载驱动、固件更新前 quiet 设备 |

#### 自检宏

| 宏 | 为真时 |
|----|--------|
| **`in_interrupt()`** | 在 **硬 IRQ 或 softirq** 相关上下文 |
| **`in_irq()`** | 在 **硬 IRQ** handler 中 |
| **`in_softirq()`** | 在 **softirq** 中 |
| **`irqs_disabled()`** | 本 CPU 硬中断当前 **关闭** |

#### 使用原则

| 原则 | 原因 |
|------|------|
| **临界区尽量短** | 关中断 = 提高 **系统 IRQ 延迟** |
| **优先 `*_save/restore`** | 可嵌套、不误开 |
| **SMP 下关中断不够** | 还要 **spinlock** 防其他 CPU |
| **不要用户态关中断** | 这些 API **仅内核** |

**HFT：** 错误地在热路径 **长时间 `local_irq_disable`** 会拖慢 **同一核** 上的 **网卡 IRQ** 与 **TSC 采样**。锁 IRQ 的临界区应 **数条指令级**；毫秒级工作放 **无锁 ring** 或 **per-CPU**。

→ **Ch 9–10** 自旋锁 + `local_irq_save` 组合 · [Ch 8.8](../../chapter-08-bottom-halves/notes/section-8.8-锁定与禁用下半部.md) `local_bh_disable` · [01 Day 14 临界区](../../../../05-os-from-scratch/thirty-days-os/day-14-keyboard/)

### 常见陷阱

1. 混淆 local_irq_disable() 和 local_irq_save()——前者不保存状态，后者保存
2. 以为 local_irq_disable() 禁用所有 CPU 的中断——只禁本地 CPU
3. 在 spin_lock_irqsave() 后手动 local_irq_enable()——会破坏锁的 IRQ 保护

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** local_irq_disable() 和 local_irq_save(flags) 的区别？什么时候用哪个？

<details><summary>答案</summary>

local_irq_disable()：无条件关中断，不保存之前状态。如果你不知道调用前中断是否已关，用这个可能破坏调用者的状态。local_irq_save(flags)：保存 RFLAGS.IF 到 flags 再关中断。local_irq_restore(flags) 恢复。内核代码应始终用 _save/_restore 版本。

</details>

**Q2.** spin_lock() / spin_lock_irq() / spin_lock_irqsave() / spin_lock_bh() 的选择？

<details><summary>答案</summary>

spin_lock()：进程上下文 + 短临界区。spin_lock_irq()：知道中断当前开着时用。spin_lock_irqsave(flags)：最安全，保存+关中断（推荐）。spin_lock_bh()：禁 softirq 但不禁 hard IRQ。选择原则：如果中断也访问同一锁 → spin_lock_irqsave。如果只有 softirq 访问 → spin_lock_bh。进程上下文独享 → spin_lock。

</details>

**Q3.** HFT 如何减少中断控制的副作用？

<details><summary>答案</summary>

① 避免 `local_irq_disable()` 在用户态（用户态不能关中断）。② 用 `isolcpus` + 中断重定向代替手动关中断。③ `SCHED_FIFO` + 绑核让交易线程不受中断影响。④ 如果必须在内核模块中关中断，临界区 <1us（否则影响系统响应）。⑤ `preempt_disable()` 比 `local_irq_disable()` 开销小，优先考虑。

</details>

</details>

---
