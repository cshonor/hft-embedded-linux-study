## ⑧ 锁定与禁用下半部

下半部在 **中断返回后异步** 执行 — 与 **ISR**、**进程上下文** 可能 **并发访问同一数据**。除 **自旋锁** 外，还需 **禁止下半部（bottom half）** 防止 **同 CPU 死锁**。

#### 为何需要 `local_bh_*`

| 并发方 | 场景 |
|--------|------|
| **进程上下文** 持锁访问数据 | 若此时 **tasklet/softirq** 插入并抢 **同一把锁** → **死锁**（BH 不会睡眠，永远等） |
| **ISR** 与 **tasklet** | ISR 用 `spin_lock`，tasklet 也用它 — ISR 内 schedule tasklet 后仍可能交错 |

```
进程: spin_lock(A) ──► 被 softirq 打断 ──► softirq 也要 lock(A) ──► 死锁
         ↑                                      │
         └────────── 不会释放 A ────────────────┘
```

#### `local_bh_disable()` / `local_bh_enable()`

| API | 作用 |
|-----|------|
| **`local_bh_disable()`** | **本 CPU** 禁止 **softirq + tasklet** 处理 |
| **`local_bh_enable()`** | 重新启用 |
| **`local_bh_disable()` 可嵌套** | 内部计数 — 配对 enable |

| 注意 | 说明 |
|------|------|
| **不包括 workqueue** | worker 是 **进程上下文** — 用 **mutex** 等，不用 `local_bh_*` |
| **不代替 spinlock** | 只防 **本 CPU BH** — SMP 仍要 **spinlock** |
| **只影响本 CPU** | 其他 CPU 的 softirq 仍运行 |

#### 与 spinlock 的组合 API

| API | 等价效果 |
|-----|----------|
| **`spin_lock_bh(&lock)`** | 获取 spinlock + **`local_bh_disable()`** |
| **`spin_unlock_bh(&lock)`** | 释放 + **`local_bh_enable()`** |
| **`spin_lock_irqsave(&lock, flags)`** | spinlock + **关硬 IRQ**（Ch 7.7） |
| **`spin_unlock_irqrestore(&lock, flags)`** | 释放 + 恢复 IRQ |

```c
unsigned long flags;

/* 进程上下文 — 与 ISR + tasklet 都安全 */
spin_lock_irqsave(&dev->lock, flags);
dev->counter++;
spin_unlock_irqrestore(&dev->lock, flags);

/* 若确定 ISR 不会抢这把锁，只要防 BH： */
spin_lock_bh(&dev->lock);
/* … */
spin_unlock_bh(&dev->lock);
```

#### 选型：该用哪种锁组合

| 临界区竞争者 | 推荐 |
|--------------|------|
| **仅其他 CPU 进程** | `spin_lock` |
| **+ 本 CPU tasklet/softirq** | **`spin_lock_bh`** |
| **+ hardirq（ISR）** | **`spin_lock_irqsave`** |
| **+ 可能睡眠的线程（workqueue）** | **`mutex`**（不可在 ISR/BH 用） |

```
竞争者矩阵：
              │ 其他 CPU │ tasklet │ hardirq │ workqueue
──────────────┼──────────┼─────────┼─────────┼──────────
spin_lock     │    ✓     │   ✗     │    ✗    │  部分
spin_lock_bh  │    ✓     │   ✓     │    ✗    │  部分
spin_lock_irq │    ✓     │   ✓     │    ✓    │  部分
mutex         │    ✓     │   ✗     │    ✗    │    ✓
```

#### `disable_bh` 与全局 `softirq`（了解）

| API | 范围 |
|-----|------|
| **`local_bh_disable`** | **本 CPU** — 最常用 |
| 早期 **`global_cli`** 类 | 已废弃 — 不要在新代码模仿 |

#### 卸载与同步

| 操作 | 顺序 |
|------|------|
| **移除 tasklet** | `tasklet_disable` → `tasklet_kill` → 再释数据结构 |
| **移除 work** | `cancel_work_sync` |
| **与 `free_irq` 配合** | 先确保 **无 BH/work 再触数据结构** |

**HFT：** 在 **策略核** 上若驱动误用 **`spin_lock_bh` 长临界区**，会阻塞该核 **NET_RX softirq** — 表现为 **`%soft` 掉在别的核** 而 **本核延迟尖刺**。热路径锁应 **极短**；长段用 **per-CPU 副本 + RCU**（Ch 10）。

→ **Ch 9–10** 内核同步详解 · [Ch 7.7](../../chapter-07-interrupts/notes/section-7.7-中断控制.md) `local_irq_save` · [Ch 9.5](../../chapter-09-kernel-sync-intro/notes/section-9.5-死锁.md) 死锁

---
