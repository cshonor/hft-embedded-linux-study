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

---
