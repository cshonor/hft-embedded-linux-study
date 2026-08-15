# 并发 Bug 的类型

> 🔴 精读 · Part 3: Diagnostics & Advanced Tools

## 概念详解

### 并发 Bug 分类

| 类型 | 描述 | 后果 | 检测工具 | 典型场景 |
|------|------|------|---------|---------|
| **死锁** (Deadlock) | 线程互相等待锁 | 系统挂死 | LOCKDEP | AB-BA 锁序反序 |
| **活锁** (Livelock) | 线程不断重试但无法前进 | CPU 100% 但无进展 | 观察 | 退避算法冲突 |
| **数据竞争** (Race) | 无同步的并发访问 | 数据损坏 | KCSAN | 忘记加锁 |
| **优先级反转** | 低优先级持锁阻塞高优先级 | 延迟飙升 | 观察 | 实时系统 |

### 死锁的四种类型

#### 1. AA 死锁（递归锁）

```c
static DEFINE_SPINLOCK(my_lock);

void func_a(void) {
    spin_lock(&my_lock);
    func_b();        // func_b 也获取 my_lock
    spin_unlock(&my_lock);
}

void func_b(void) {
    spin_lock(&my_lock);  // AA 死锁! 同一线程再次获取
    spin_unlock(&my_lock);
}
// LOCKDEP 报告: possible recursive locking detected
```

#### 2. AB-BA 死锁（经典死锁）

```c
static DEFINE_SPINLOCK(lock_a);
static DEFINE_SPINLOCK(lock_b);

// CPU0                    CPU1
void thread1(void) {       void thread2(void) {
    spin_lock(&lock_a);        spin_lock(&lock_b);
    spin_lock(&lock_b);  ←     spin_lock(&lock_a);  ← 死锁!
}                            }
// LOCKDEP 报告: possible circular locking dependency detected
```

#### 3. AB-CA 死锁（环形死锁）

```
线程1: A → B
线程2: B → C
线程3: C → A   ← 形成环 A→B→C→A
```

#### 4. 中断-进程死锁

```c
static DEFINE_SPINLOCK(my_lock);

void my_function(void) {
    spin_lock(&my_lock);     // 进程上下文获取锁
    // ... 此时中断发生 ...
    // 中断处理函数也尝试获取 my_lock → 死锁!
    spin_unlock(&my_lock);
}

irqreturn_t my_irq_handler(int irq, void *dev) {
    spin_lock(&my_lock);     // 中断上下文也需要获取 → 死锁!
    spin_unlock(&my_lock);
    return IRQ_HANDLED;
}

// 修复: 进程上下文用 spin_lock_irqsave() 禁用中断
void my_function(void) {
    unsigned long flags;
    spin_lock_irqsave(&my_lock, flags);  // 获取锁 + 禁用中断
    spin_unlock_irqrestore(&my_lock, flags);
}
```

### 活锁 (Livelock)

```c
// 活锁: 两个线程不断退避但无法前进
void thread1(void) {
    while (!try_lock(&lock_a)) {
        backoff();
        continue;
    }
    // 永远获取不到 lock_a
}
// 区别: 死锁时线程睡眠，活锁时线程运行（CPU 100%）
```

### 数据竞争 (Race Condition)

```c
// 数据竞争示例
static int shared_counter = 0;

void increment(void) { shared_counter++; }     // 非原子操作
void decrement(void) { shared_counter--; }      // 并发执行 → 数据竞争

// 修复 1: 原子操作
static atomic_t shared_counter = ATOMIC_INIT(0);
void increment(void) { atomic_inc(&shared_counter); }

// 修复 2: 自旋锁
static DEFINE_SPINLOCK(counter_lock);
void increment(void) {
    spin_lock(&counter_lock);
    shared_counter++;
    spin_unlock(&counter_lock);
}

// 修复 3: READ_ONCE/WRITE_ONCE (接受非原子但消除编译器优化)
int read_counter(void) { return READ_ONCE(shared_counter); }
```

### 优先级反转 (Priority Inversion)

```
场景:
  高优先级线程 H 需要锁 L
  低优先级线程 L 持有锁 L
  中优先级线程 M 抢占 L

结果: H 等 L → L 被 M 抢占 → M 运行 → L 无法运行 → H 一直等

解决: 优先级继承 (Priority Inheritance)
  当 H 等待 L 的锁时，临时提升 L 的优先级到 H 的级别
```

### 检测工具对比

| 工具 | 检测类型 | 原理 | 开销 | 生产可用 |
|------|---------|------|------|---------|
| LOCKDEP | 死锁（锁序错误） | 构建锁依赖图，检测环 | ~100-200ns/锁操作 | 否 |
| lock_stat | 锁竞争 | 统计等待/持有时间 | 同 LOCKDEP | 否 |
| KCSAN | 数据竞争 | watchpoint + 延迟观察 | 编译时插桩 | 否 |
| DEBUG_ATOMIC_SLEEP | 原子上下文睡眠 | might_sleep() 检查 | 极小 | 可用 |

### HFT 关联应用

HFT 自定义内核模块最容易遇到的并发问题：

1. **AA 死锁**：回调函数中重复获取同一锁
2. **中断-进程死锁**：中断处理和进程上下文共用锁
3. **数据竞争**：统计计数器忘记用原子操作
4. **锁竞争**：高频率路径的 spinlock 争用导致延迟毛刺

```c
// HFT 常见错误: 回调中重复加锁
static DEFINE_SPINLOCK(trade_lock);

void on_market_data(void) {
    spin_lock(&trade_lock);
    update_order_book();   // 此函数内部又获取 trade_lock → AA 死锁
    spin_unlock(&trade_lock);
}

void update_order_book(void) {
    spin_lock(&trade_lock);  // AA 死锁!
    spin_unlock(&trade_lock);
}

// 修复: 拆分为 locked/unlocked 版本
void update_order_book_locked(void) {
    // 调用者已持锁
}
void update_order_book(void) {
    spin_lock(&trade_lock);
    update_order_book_locked();
    spin_unlock(&trade_lock);
}
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** AA 死锁和 AB-BA 死锁哪个更容易被 LOCKDEP 检测？

> 两者都能被 LOCKDEP 检测。AA 死锁在第二次获取同一锁时立即报告。AB-BA 死锁在 LOCKDEP 检测到锁序矛盾时报告。AA 更快被发现，因为不需要等待实际并发。

**Q2:** 内核并发 bug 的两大类是什么？分别用什么工具检测？

> (1) 死锁（deadlock）：AB-BA 锁序反序 → LOCKDEP。(2) 竞态（race）：缺乏同步的并发访问 → KCSAN。

**Q3:** 优先级反转和死锁有什么区别？

> 死锁是线程互相等待锁，无法前进。优先级反转是低优先级线程持有高优先级线程需要的锁，中间优先级线程抢占低优先级线程，导致高优先级线程间接被阻塞。内核通过优先级继承（rt_mutex）解决。

**Q4:** 活锁和死锁有什么区别？

> 死锁时线程阻塞睡眠（不消耗 CPU）。活锁时线程不断运行（CPU 100%）但无法前进——如两个线程不断退避重试但总是同时重试。

**Q5:** HFT 模块中为什么 AA 死锁特别常见？

> HFT 模块常有回调机制（定时器回调、中断回调、网络数据回调）。如果回调函数调用的辅助函数也获取同一锁，就形成 AA 死锁。解决：拆分 `_locked` 和 `_unlocked` 版本的函数。

</details>

## 交叉引用

- [05.6 ch08 LOCKDEP 锁依赖检测器](chapter-08-lock-debug/notes/02-lockdep.md)
- [05.6 ch08 LOCKDEP 死锁检测实践](chapter-08-lock-debug/notes/03-lockdep-deadlock-detection.md)
- [05.6 ch08 KCSAN 数据竞争检测器](chapter-08-lock-debug/notes/05-kcsan.md)
