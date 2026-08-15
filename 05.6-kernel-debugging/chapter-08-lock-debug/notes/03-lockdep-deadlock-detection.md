# 用 LOCKDEP 发现潜在死锁

> 🔴 精读

## 概念详解

### 案例一：中断上下文死锁

```c
static DEFINE_SPINLOCK(my_lock);

void my_function(void) {
    spin_lock(&my_lock);    // 进程上下文获取
    // ... 此时中断发生 ...
    spin_unlock(&my_lock);
}

irqreturn_t my_irq_handler(int irq, void *dev) {
    spin_lock(&my_lock);    // 中断上下文也需要获取 → 死锁!
    spin_unlock(&my_lock);
    return IRQ_HANDLED;
}
// LOCKDEP 报告: inconsistent lock state
```

修复：
```c
void my_function(void) {
    unsigned long flags;
    spin_lock_irqsave(&my_lock, flags);  // 获取锁 + 禁用中断
    spin_unlock_irqrestore(&my_lock, flags);
}
```

### 案例二：锁序矛盾

```c
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
```

修复：全局统一锁序，所有地方都先 A 后 B。

### 案例三：spinlock 中睡眠

```c
void bad_function(void) {
    spin_lock(&my_lock);
    kmem_cache_alloc(cache, GFP_KERNEL);  // 可能睡眠!
    copy_from_user(...);                    // 可能睡眠!
    msleep(1);                              // 必然睡眠!
    spin_unlock(&my_lock);
}
// LOCKDEP 报告: scheduling while atomic
```

修复：
```c
// 修复 1: 改用 mutex
static DEFINE_MUTEX(my_mutex);
void good_function(void) {
    mutex_lock(&my_mutex);
    kmem_cache_alloc(cache, GFP_KERNEL);  // 安全
    mutex_unlock(&my_mutex);
}

// 修复 2: spinlock 中使用 GFP_ATOMIC
void good_function2(void) {
    spin_lock(&my_lock);
    ptr = kmem_cache_alloc(cache, GFP_ATOMIC);  // 不睡眠
    spin_unlock(&my_lock);
}
```

### 修复模式总结

| 问题 | 修复 | 代码示例 |
|------|------|---------|
| 中断-进程死锁 | `spin_lock_irqsave()` | `spin_lock_irqsave(&lock, flags)` |
| 锁序矛盾 | 全局统一锁获取顺序 | 所有路径都 A→B |
| AA 死锁 | 拆分 locked/unlocked 函数 | `_locked()` 版本不加锁 |
| spinlock 中睡眠 | 改用 mutex 或 GFP_ATOMIC | `mutex_lock()` 或 `GFP_ATOMIC` |

### spin_lock 变体选择指南

| 函数 | 中断 | 软中断 | 适用场景 |
|------|------|--------|---------|
| `spin_lock()` | 不禁 | 不禁 | 锁不用于中断/软中断 |
| `spin_lock_bh()` | 不禁 | 禁 | 锁用于软中断 |
| `spin_lock_irq()` | 禁 | 禁(间接) | 锁用于硬中断 |
| `spin_lock_irqsave()` | 禁+保存 | 禁(间接) | **推荐**：不确定时用 |

### HFT 关联应用

```c
// HFT 模块中典型的锁序冲突
// 数据路径: 网卡IRQ → trade_engine → order_book
// 控制路径: ioctl → trade_engine → order_book

static DEFINE_SPINLOCK(trade_lock);
static DEFINE_SPINLOCK(control_lock);

// 数据路径: trade_lock → control_lock
void on_market_data(void) {
    spin_lock(&trade_lock);
    spin_lock(&control_lock);  // trade → control
    spin_unlock(&control_lock);
    spin_unlock(&trade_lock);
}

// 控制路径: 反序!
void on_ioctl(void) {
    spin_lock(&control_lock);
    spin_lock(&trade_lock);    // control → trade ← 死锁!
}

// 修复: 统一为 trade → control 顺序
void on_ioctl(void) {
    spin_lock(&trade_lock);     // 改为先 trade
    spin_lock(&control_lock);   // 后 control
}
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** `spin_lock_irqsave()` 如何避免中断-进程死锁？

> `spin_lock_irqsave()` 在获取自旋锁的同时禁用当前 CPU 的中断（保存中断状态到 flags）。这样在持有锁期间不会发生中断，中断处理函数不会尝试获取同一锁。

**Q2:** 为什么内核不提供递归自旋锁来解决 AA 死锁？

> 递归锁需要记录持有者和递归计数，增加开销。自旋锁设计为极低开销（通常 1-2 条原子指令），递归锁的开销违背了这一目标。内核要求开发者通过正确的代码结构避免重入。

**Q3:** LOCKDEP 如何检测 "在 spinlock 中睡眠" 的 bug？

> LOCKDEP 维护当前 CPU 持有的锁链表。当调用 `might_sleep()` 时，LOCKDEP 检查当前是否持有 spinlock，如果是则报告 "scheduling while atomic"。

**Q4:** `spin_lock_bh()` 和 `spin_lock_irqsave()` 什么时候分别使用？

> `spin_lock_bh()` 禁用软中断，适合锁只在进程上下文和软中断中使用。`spin_lock_irqsave()` 禁用硬中断，适合锁在硬中断中也使用。不确定时用 `spin_lock_irqsave()`。

**Q5:** HFT 模块中如何避免锁序冲突？

> (1) 定义全局锁获取顺序；(2) 用 `lockdep_assert_held()` 验证锁状态；(3) staging 环境启用 LOCKDEP 做压力测试；(4) 尽量减少锁嵌套，用无锁数据结构（如 RCU）替代。

</details>

## 交叉引用

- [05.6 ch08 并发 Bug 类型](../../chapter-08-lock-debug/notes/01-concurrency-bug-types.md)
- [05.6 ch08 LOCKDEP 锁依赖检测器](../../chapter-08-lock-debug/notes/02-lockdep.md)
- [05.6 ch08 lock_stat 锁竞争统计](../../chapter-08-lock-debug/notes/04-lock-stat.md)
