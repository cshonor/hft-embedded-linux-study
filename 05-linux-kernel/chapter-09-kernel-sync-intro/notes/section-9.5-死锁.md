## ⑤ 死锁 · Deadlocks

**死锁（deadlock）** = 一个或多个执行线程 **永久阻塞** — 各自 **持有** 一些资源并 **等待** 对方释放，形成 **无法推进** 的环。

#### 必要条件（四者齐备才可能死锁）

| 条件 | 内核中的例子 |
|------|--------------|
| **互斥** | 锁一次只能一人持 |
| **持有并等待** | 持 `lock A` 再等 `lock B` |
| **不可抢占** | 锁不能被强制剥夺 |
| **循环等待** | A 等 B，B 等 A |

#### 常见类型

| 类型 | 场景 | 例子 |
|------|------|------|
| **自死锁（self-deadlock）** | 线程 **再次获取** 已持有的 **非递归锁** | 进程持 `mutex` → 触发 fault → fault 路径再 `mutex_lock` 同锁 |
| **ABBA 死锁（deadly embrace）** | 两线程 **锁顺序相反** | 线程1: `A→B`；线程2: `B→A` |
| **ISR 死锁** | 进程持锁 → 被 ISR 打断 → ISR 要同锁 | 用 **`spin_lock_irqsave`** 防 ISR |
| **BH 死锁** | 进程持锁 → tasklet 要同锁 | 用 **`spin_lock_bh`** |

```
ABBA：
  线程1:  lock(A) ──────────► 等待 lock(B)
  线程2:         lock(B) ───► 等待 lock(A)
                      ↑_______________|
                            环 — 永久等待
```

#### 与 Ch 7/8 相关的死锁场景

| 场景 | 错误 | 修复 |
|------|------|------|
| ISR 与进程共享 `spinlock` | 进程 `spin_lock` 时被 ISR 打断 | **`spin_lock_irqsave`** |
| tasklet 与进程共享锁 | 进程持锁时 tasklet 运行 | **`spin_lock_bh`** |
| **`mutex` 在 IRQ 里** | ISR `mutex_lock` | **禁止** — 换 spinlock 或 defer |
| **disable_irq 太久** | 间接导致 **watchdog** | 缩短临界区 |

```c
/* 错误 — ISR 可能自死锁 */
spin_lock(&dev->lock);   /* 进程 */
/* --- IRQ 插入，ISR 也 spin_lock(&dev->lock) --- */

/* 正确 */
spin_lock_irqsave(&dev->lock, flags);
```

#### 无死锁编码规则（LKD 作者）

| 规则 | 说明 |
|------|------|
| **嵌套锁固定顺序** | 全局约定：始终 **先 A 后 B** — 所有路径一致 |
| **防止饥饿** | 公平锁或 **限重试** — 避免某线程永远拿不到 |
| **不要重复获取同锁** | 除非明确使用 **递归 mutex** |
| **保持方案简单** | 锁越多、层次越深，ABBA 风险越大 |
| **持锁时不调用外部代码** | 回调可能 **反向要锁** |

#### 调试死锁

| 手段 | 说明 |
|------|------|
| **`CONFIG_LOCKDEP`** | 启动时检测 **锁顺序违规** |
| **sysrq / panic 栈** | 看 **谁持谁等** |
| **`/proc/lockdep`** | 锁依赖图 |

#### 活锁 vs 死锁（区分）

| | 死锁 | 活锁 |
|---|------|------|
| **状态** | 都不推进 | 都在 **忙** 但无进展 |
| **例子** | ABBA 互等 | 两线程 **礼貌退让** 重复重试 |

**HFT：** 低概率 **ABBA** 在压测才出现 — 最糟的 P99。用 **Lockdep** 在开发板开；生产 **固定锁序** + **避免持锁 I/O**。

→ **Ch 8.8** `spin_lock_irqsave` · **Ch 10** Lockdep · [Ch 9.2](section-9.2-加锁.md) 加锁策略

### 常见陷阱

1. 以为死锁只会发生在多锁场景——单锁也能死锁（如递归加锁同一 spinlock）
2. 混淆死锁的四种条件——互斥、持有并等待、不可剥夺、循环等待
3. 忽略 lockdep——lockdep 能在开发阶段检测潜在死锁，生产阶段关掉

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** 死锁的四个必要条件？

<details><summary>答案</summary>

① 互斥：资源同一时间只能被一个执行者使用。② 持有并等待：持有资源的执行者可以请求新资源。③ 不可剥夺：资源不能被强制夺走（只能持有者主动释放）。④ 循环等待：存在执行者的循环等待链。打破任一条件即可预防死锁。最常用：打破循环等待——规定锁的全局获取顺序。

</details>

**Q2.** 内核中常见的死锁场景？

<details><summary>答案</summary>

① 递归加锁：同一 spinlock 在持锁期间再次 lock → 自死锁。② AB-BA 死锁：线程1 lock(A)→lock(B)，线程2 lock(B)→lock(A)。③ 中断死锁：进程持 lock()，被中断，中断处理函数也 lock() → 死锁。解决：中断访问的锁用 spin_lock_irqsave()。④ softirq 死锁：用 spin_lock_bh()。预防：lockdep + 全局锁顺序。

</details>

**Q3.** lockdep 怎么使用？能检测什么？

<details><summary>答案</summary>

`CONFIG_LOCKDEP=y` 编译内核。开启：`echo 1 > /proc/sys/kernel/lock_stat`。检测：① AB-BA 死锁（锁顺序反转）。② 递归加锁。③ 中断上下文持有可睡眠锁。④ IRQ 安全性不匹配。开销：~10% 性能下降，仅开发阶段开启。用户态类似工具：ThreadSanitizer (`-fsanitize=thread`)。HFT 开发应 CI 中跑 lockdep/TSan。

</details>

</details>

---
