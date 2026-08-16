## ② 自旋锁 · Spin Locks

内核 **最常见** 的锁：争用时 **忙等（自旋）** 直到锁可用 — **绝不睡眠**。

| 属性 | 说明 |
|------|------|
| 持有者 | **至多一个** 执行上下文 |
| 争用 | 不断重试获取（空转 CPU） |
| 上下文 | **进程 / 中断 / softirq** 都可能用（配合关中断/关 BH） |

#### 适用 vs 不适用

| 适合 | 不适合 |
|------|--------|
| **持有时间极短**（几条～几十条指令级） | 长临界区（浪费多核空转） |
| **不能睡眠** 的上下文 | 需要 `copy_from_user`、等 I/O、拿 mutex |
| 保护共享数据结构的瞬间更新 | 「大段业务逻辑」 |

#### 不可递归

| 规则 | 后果 |
|------|------|
| **同一 spinlock 不能二次 acquire** | **自死锁** — 永远等自己 |

嵌套多把锁必须 **全局固定加锁顺序**（Ch 9 死锁）。

#### 与中断配合（必背）

进程上下文持锁时若被中断，ISR 再抢 **同一把锁** → 死锁：

```
进程: spin_lock(L) ──► 临界区中
         │
      本地中断打进来
         │
ISR:  spin_lock(L) ──► 自旋等进程释放
         │
进程永远等 ISR 结束才能跑 ──► 死锁
```

| API | 作用 |
|-----|------|
| **`spin_lock()` / `spin_unlock()`** | 仅加锁；假设中断不会碰这把锁 |
| **`spin_lock_irqsave()` / `spin_unlock_irqrestore()`** | **关本地中断 + 加锁**；`flags` 保存/恢复 |
| **`spin_lock_bh()` / `spin_unlock_bh()`** | 禁 softirq（bottom half）+ 加锁 |
| **`spin_trylock()`** | 拿不到立即失败 — 不自旋 |

```c
unsigned long flags;
spin_lock_irqsave(&lock, flags);
/* 临界区：短！不可睡眠 */
spin_unlock_irqrestore(&lock, flags);
```

#### 现代内核：`spinlock_t` 与 ticket / queued

实现随版本演进（ticket lock、qspinlock）— **语义不变**：短、不睡、注意 IRQ。

**HFT：** 热路径用户态自旋锁同理；内核里 **spinlock 持有时间 ≈ 抖动预算**。`perf lock` / lockstat 看争用。驱动共享缓冲：进程侧与 NAPI/softirq 共享时常用 **`spin_lock_bh` 或 irqsave**。

→ **Ch 7–8** · [10.3 rwlock](./section-10.3-读-写自旋锁.md) · [10.11 选型](./section-10.11-选型速查Ch-9--Ch-10.md)

### 常见陷阱

1. 在持 spinlock 时调用睡眠函数——会 BUG: scheduling while atomic panic
2. 混淆 spin_lock() 和 spin_lock_irqsave()——前者不关中断，后者保存+关
3. 以为 spinlock 有公平性保证——spinlock 不保证公平，可能饿死某些等待者

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** spin_lock() / spin_lock_irqsave() / spin_lock_bh() 什么时候用哪个？

<details><summary>答案</summary>

spin_lock()：进程上下文 + 确认无中断/softirq 访问同一锁。spin_lock_irqsave(flags)：中断也可能访问同一锁。spin_lock_bh()：softirq 可能访问但 hard IRQ 不会。如果不确定 → 用 spin_lock_irqsave()（最安全）。UP 上 spin_lock() 退化为 preempt_disable()，spin_lock_irqsave() 退化为 local_irq_save()。

</details>

**Q2.** 持 spinlock 时为什么不能睡眠？

<details><summary>答案</summary>

Spinlock 假设等待者会忙等（spin）。如果持锁者睡眠（schedule()）：① 等待者无限 spin 浪费 CPU。② schedule() 检查 preempt_count > 0 → BUG: scheduling while atomic → panic。③ 如果切换到的进程也请求同一锁 → 死锁。RT 内核把 spinlock 变成可睡眠锁后这个限制不成立。

</details>

**Q3.** HFT 用户态的 spinlock 和内核有什么不同？

<details><summary>答案</summary>

用户态没有真正的 spinlock（不关中断/不禁抢占）。`std::atomic_flag` + test_and_set 自旋是最接近的。区别：① 用户态 spinlock 被调度器抢占后仍 spin（浪费 CPU 时间片）。② 不能关中断防止抢占。③ `sched_yield()` 可以让出 CPU 但不释放锁。HFT 用户态 spinlock 应限制在 <100ns 临界区，超时改用 futex/mutex。

</details>

</details>


> ↔ [ULK Ch5 §4 自旋锁](../../../16-linux-kernel-deep/chapter-05-kernel-synchronization/notes/section-4-自旋锁.md)
---
