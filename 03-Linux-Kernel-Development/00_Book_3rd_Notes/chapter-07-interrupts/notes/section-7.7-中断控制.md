## ⑦ 中断控制 · Interrupt Control

为 **同步数据、避免竞态**，内核提供关/开中断 API。

#### 本地全部中断

| API | 说明 |
|-----|------|
| **`local_irq_disable()`** | 禁止 **本 CPU** 上所有中断 |
| **`local_irq_enable()`** | 开启 |

#### 推荐：保存/恢复

| API | 说明 |
|-----|------|
| **`local_irq_save(flags)`** | 关中断前 **保存** 原状态到 `flags` |
| **`local_irq_restore(flags)`** | **恢复** 之前状态 — 不误开原本就关的中断 |

```c
unsigned long flags;
local_irq_save(flags);
/* 临界区 — 不会被本 CPU 中断打断 */
local_irq_restore(flags);
```

#### 单条 IRQ 线

| API | 作用 |
|-----|------|
| **`disable_irq()` / `enable_irq()`** | **全局** 屏蔽/启用 **特定 IRQ 线** |

#### 自检宏

| 宏 | 作用 |
|----|------|
| **`in_interrupt()`** | 是否在中断（含软中断）相关上下文 |
| **`in_irq()`** | 是否在 **硬 IRQ** 处理中 |

→ **Ch 9–10** 自旋锁 + `local_irq_save` 组合 · [01 Day 14 临界区](../../../../07-system-low-level-hands-on/02-30days-os/day-14-keyboard/)

---
