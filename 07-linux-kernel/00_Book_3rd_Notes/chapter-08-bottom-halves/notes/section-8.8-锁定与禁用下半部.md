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

### 常见陷阱

1. 混淆 spin_lock_bh() 和 local_bh_disable()——前者锁+禁 softirq，后者只禁 softirq
2. 在 spin_lock_bh() 后手动 local_bh_enable()——会破坏锁保护
3. 以为 local_bh_disable() 禁用了 hard IRQ——只禁 softirq，hard IRQ 仍可触发

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** spin_lock_bh() 和 local_bh_disable() 的区别？

<details><summary>答案</summary>

spin_lock_bh(lock)：① 获取 spinlock。② 禁用本地 softirq（递增 preempt_count 的 softirq 位）。用于保护 softirq 和进程上下文都访问的数据。local_bh_disable()：只禁 softirq，不锁。用于 softirq 临界区不需要锁但需要禁 softirq 的场景（如 per-CPU 统计）。对应的恢复：spin_unlock_bh() / local_bh_enable()。

</details>

**Q2.** local_bh_disable() 禁用 softirq 后，hard IRQ 还能触发吗？

<details><summary>答案</summary>

能。local_bh_disable() 只递增 preempt_count 的 softirq 位（SOFTIRQ_OFFSET），不影响 hard IRQ。hard IRQ 仍可触发和执行。如果需要同时禁 hard IRQ 和 softirq，用 local_irq_disable()。如果需要禁 softirq 但允许 hard IRQ，用 local_bh_disable()。

</details>

**Q3.** HFT 为什么需要关心 softirq 禁用？

<details><summary>答案</summary>

HFT 内核模块（如定制 NIC 驱动）可能需要在 softirq 上下文操作共享数据。spin_lock_bh() 确保 softirq 不会在持锁期间重入。但 HFT 用户态通常不直接处理 softirq——通过 `isolcpus` + 中断重定向 + RPS/RFS 把 softirq 推到非交易核。测量：`perf stat -e softirq` 统计 softirq 频率。

</details>

</details>

---
