# 8.3 用 LOCKDEP 发现潜在死锁

> 🔴 精读

## 本节要点

### 实际案例分析

```c
// 案例一: 中断上下文死锁
static DEFINE_SPINLOCK(my_lock);

void my_function(void) {
    spin_lock(&my_lock);    // 进程上下文获取
    // ...
    spin_unlock(&my_lock);
}

irqreturn_t my_irq_handler(int irq, void *dev) {
    spin_lock(&my_lock);    // 中断上下文也需要获取
    // 如果中断在此处发生，进程上下文已持有锁
    // → 死锁!
    spin_unlock(&my_lock);
    return IRQ_HANDLED;
}

// LOCKDEP 报告:
// WARNING: inconsistent lock state
// 4. just locked:  my_lock (process context)
// 5. already locked: my_lock (interrupt context)
// 解决: 用 spin_lock_irqsave() 在进程上下文中禁用中断
```

```c
// 案例二: 锁序矛盾
static DEFINE_SPINLOCK(lock_a);
static DEFINE_SPINLOCK(lock_b);

void func1(void) {
    spin_lock(&lock_a);
    spin_lock(&lock_b);  // A → B
    spin_unlock(&lock_b);
    spin_unlock(&lock_a);
}

void func2(void) {
    spin_lock(&lock_b);
    spin_lock(&lock_a);  // B → A  ← 与 A → B 矛盾!
    spin_unlock(&lock_a);
    spin_unlock(&lock_b);
}

// LOCKDEP 报告: possible circular locking dependency
// 解决: 全局统一锁序，所有地方都先 A 后 B
```

### 修复模式

| 问题 | 修复 |
|------|------|
| 中断-进程死锁 | `spin_lock_irqsave()` / `spin_unlock_irqrestore()` |
| 锁序矛盾 | 全局统一锁获取顺序 |
| AA 死锁 | 检查逻辑，避免重入；或用递归锁（内核无标准递归锁） |
| 锁释放后使用 | 检查生命周期，确保释放后不再访问 |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** `spin_lock_irqsave()` 如何避免中断-进程死锁？

> `spin_lock_irqsave()` 在获取自旋锁的同时禁用当前 CPU 的中断（保存中断状态到 flags）。这样在持有锁期间不会发生中断，中断处理函数不会尝试获取同一锁。释放时 `spin_unlock_irqrestore()` 恢复中断状态。

**Q2:** 为什么内核不提供递归自旋锁来解决 AA 死锁？

> 递归锁需要记录持有者（哪个线程持有）和递归计数，增加开销。自旋锁设计为极低开销（通常 1-2 条原子指令），递归锁的开销违背了这一目标。内核要求开发者通过正确的代码结构避免重入，而非依赖递归锁。

</details>
