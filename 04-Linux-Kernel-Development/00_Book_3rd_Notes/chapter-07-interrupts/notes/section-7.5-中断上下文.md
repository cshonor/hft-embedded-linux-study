## ⑤ 中断上下文 · Interrupt Context

当 CPU 执行 **ISR** 时，内核处于 **中断上下文（interrupt context）** — 也称 **硬 IRQ 上下文（hardirq context）**。这与 **进程上下文（process context）** 有本质区别，驱动作者 **必须** 牢记。

| 对比项 | 进程上下文（Ch 5） | 中断上下文 |
|--------|-------------------|------------|
| **关联进程** | **`current` 有意义** — 指向被中断的用户/内核线程 | **不与具体进程绑定** — `current` 可能是被中断的任务，但 ISR **不能依赖** 它 |
| **睡眠/阻塞** | **可以** — `mutex_lock`、`wait_event`、缺页 | **绝对禁止** — 无「后备进程」可调度，睡眠 → BUG |
| **抢占** | 视 **内核抢占** 配置（Ch 4） | 中断本身即 **抢占** 当前执行 |
| **调度** | 可主动/被动 `schedule()` | **不能** 调用会睡眠的 API |
| **栈** | 进程 **内核栈** | 专用 **中断栈** 或独立栈帧（见下） |

#### 为什么中断上下文不能睡眠

```
ISR 尝试 mutex_lock（持有者未释放）
        │
        ▼
  mutex 不可获得 → 当前「任务」应 sleep
        │
        ▼
  但 ISR 不是 schedulable task — 没有 task_struct 可挂起
        │
        ▼
  内核死锁 / BUG_ON — 系统 hang 或 oops
```

| 禁止的操作 | 原因 |
|------------|------|
| **`mutex_lock` / `down()`** | 可能睡眠等待 |
| **`kmalloc(GFP_KERNEL)`** | 可能触发 direct reclaim、等 I/O |
| **`copy_to_user` 大块** | 可能缺页睡眠 |
| **`printk` 带 console 锁** | 极端情况下与持锁者形成依赖 |

| 通常允许 | 注意 |
|----------|------|
| **`spin_lock` / `spin_trylock`** | 临界区必须 **极短** |
| **`kmalloc(GFP_ATOMIC)`** | 不睡眠，但 **可能失败** — 应预分配 |
| **`printk`（少量）** | 热路径避免 |
| **读硬件 MMIO 寄存器** | 视硬件延迟 |

#### 栈布局演进

| 时代 | 栈布局 | 风险 |
|------|--------|------|
| **早期** | ISR **共享** 被中断进程的 **内核栈**（8KB/16KB） | ISR 栈帧 + 进程内核栈 **共用** — 深递归易溢出 |
| **2.6+ 细粒度栈** | 每 CPU 独立 **中断栈（interrupt stack）**（如 4KB） | ISR 有 **独立栈空间**，仍不宜大数组 |

```
每 CPU（概念）：
  ┌──────────────────┐
  │ 进程 A 内核栈     │  ← 被中断时正在用
  ├──────────────────┤
  │ 中断栈（per-CPU） │  ← ISR 在此展开栈帧
  └──────────────────┘
```

#### 上下文检测宏

| 宏 | 为真时表示 |
|----|------------|
| **`in_irq()`** | 在 **硬 IRQ** handler 中 |
| **`in_interrupt()`** | 在 **硬 IRQ 或 softirq** 相关上下文 |
| **`in_softirq()`** | 在 **softirq** 处理中 |

```c
if (in_interrupt())
    /* 不能睡眠的路径 */;
else
    /* 进程上下文 — 可用 mutex */;
```

#### 与下半部的衔接

| 上下文 | 典型代码位置 |
|--------|--------------|
| **hardirq** | `request_irq` 注册的 handler |
| **softirq / tasklet** | 仍 **不能睡眠** — 不是进程上下文 |
| **workqueue** | **进程上下文** — 可睡眠（→ [Ch 8.5](../../chapter-08-bottom-halves/notes/section-8.5-工作队列.md)） |

**HFT：** ISR 里 **大数组、递归、`mutex_lock`** 均为禁忌 — 与 **Ch 2 小栈** 规则叠加。低延迟路径应：**ISR 只做 ACK + 入 lock-free ring**，策略在用户态或 **专用核 pinned 线程** 消费。

→ [Ch 5](../../chapter-05-system-calls/) 进程上下文 · [Ch 7.2](section-7.2-中断处理程序.md) ISR 编写 · [Ch 8](../../chapter-08-bottom-halves/) 下半部选型

---
