# LOCKDEP：锁依赖检测器

> 🔴 精读

## 概念详解

### LOCKDEP 是什么

LOCKDEP (Lock Dependency Detector) 是内核的运行时锁依赖分析器。它在每次锁获取/释放时记录锁的依赖关系，构建**锁依赖图**。如果图中出现环，说明存在潜在死锁。

### 工作原理

```
每次 lock_acquire() 时:
  1. 记录当前 CPU 持有的锁链表
  2. 对每个已持有的锁 A 和新获取的锁 B，添加依赖 A → B
  3. 检查是否形成环: B → ... → A
  4. 如果形成环 → 报告 "possible circular locking dependency"

锁依赖图示例:
  A → B  (持有 A 时获取 B)
  B → C  (持有 B 时获取 C)
  C → A  (持有 C 时获取 A)  ← 环! 报告死锁
```

### 锁分类 (Lock Class)

LOCKDEP 按**分类**而非实例追踪依赖：

```c
// 每个 spinlock_init() / mutex_init() 创建新的 lock class
// class 基于初始化代码的调用位置 (__LINE__)

static DEFINE_SPINLOCK(lock1);  // class A
static DEFINE_SPINLOCK(lock2);  // class B

void init(void) {
    spinlock_t lock3, lock4;
    spin_lock_init(&lock3);  // class C
    spin_lock_init(&lock4);  // class C (同一行 → 同一 class)
    // lock3 和 lock4 共享同一 class C 的依赖关系
}

// 手动设置不同 class
lockdep_set_class(&lock4, &my_key);
```

### 启用 LOCKDEP

```bash
# 内核配置
CONFIG_LOCKDEP=y
CONFIG_LOCKDEP_SUPPORT=y
CONFIG_PROVE_LOCKING=y     # 启用锁依赖检测
CONFIG_LOCK_STAT=y         # 启用锁竞争统计

# boot 参数
# lockdep  — 启用
# nolockdep — 禁用

# 运行时控制
echo 1 > /proc/sys/kernel/lock_stat  # 启用统计
echo 0 > /proc/sys/kernel/lock_stat  # 禁用统计

# 性能开销: 每次加锁额外 ~100-200ns，整体 slowdown ~2-5x
```

### LOCKDEP 报告示例

```
[  123.456789] =====================================================
[  123.456790] WARNING: possible circular locking dependency detected
[  123.456795] -----------------------------------------------------
[  123.456800] my_app/1234 is trying to acquire lock:
[  123.456805]  ffff000012345678 (&my_lock_b){+.+.}-{2:2}, at: func_b+0x1c/0x40
[  123.456810]
[  123.456815] but task is already holding lock:
[  123.456820]  ffff000012345000 (&my_lock_a){+.+.}-{2:2}, at: func_a+0x20/0x50
[  123.456825]
[  123.456830] which lock already depends on the new lock.
[  123.456835]
[  123.456840] the existing dependency chain (in reverse order) is:
[  123.456845] -> #1 (&my_lock_a){+.+.}-{2:2}:
[  123.456850]        lock_acquire+0x68/0xa0
[  123.456855]        func_a+0x20/0x50
[  123.456860] -> #0 (&my_lock_b){+.+.}-{2:2}:
[  123.456865]        lock_acquire+0x68/0xa0
[  123.456870]        func_b+0x1c/0x40
[  123.456875]
[  123.456880]  Possible unsafe locking scenario:
[  123.456885]        CPU0                    CPU1
[  123.456890]        ----                    ----
[  123.456895]   lock(&my_lock_a);
[  123.456900]                                lock(&my_lock_b);
[  123.456905]                                lock(&my_lock_a);
[  123.456910]   lock(&my_lock_b);  ← DEADLOCK!
```

### LOCKDEP 检测的问题类型

| 问题 | 说明 | 报告关键词 |
|------|------|-----------|
| **Circular dependency** | AB-BA 死锁 | `possible circular locking dependency` |
| **Reacquire** | AA 死锁 | `possible recursive locking detected` |
| **IRQ lock inversion** | 进程和中断上下文锁序冲突 | `inconsistent lock state` |
| **Lock used after free** | 释放后继续使用锁 | `lock used after free` |
| **Scheduling while atomic** | spinlock 中睡眠 | `scheduling while atomic` |

### 锁状态标志

```
(&my_lock_b){+.+.}-{2:2}
         || ||
         || |+-- softirq context
         || +--- hardirq context
         |+---- reclaim context
         +----- process context

+ = 曾在此上下文中获取
- = 未在此上下文中获取
```

### HFT 关联应用

```c
// HFT 模块中用 lockdep_assert 验证锁状态
void update_order_book_locked(struct order_book *ob) {
    lockdep_assert_held(&ob->lock);  // 运行时断言
    // 如果调用时未持有锁 → LOCKDEP 报告
}
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** LOCKDEP 能检测尚未实际发生的死锁吗？

> 能。LOCKDEP 构建全局锁依赖图，只要两个线程执行了相反的锁顺序（即使没有同时执行），LOCKDEP 就会报告。这比等死锁实际发生再检测更安全。

**Q2:** LOCKDEP 的锁分类 (lock class) 是什么概念？

> 每个 `spinlock_init()` / `mutex_init()` 创建一个新的锁分类。LOCKDEP 按分类而非实例追踪依赖——同一分类的多个实例共享同一依赖关系。用 `lockdep_set_class()` 可以为同一实例设置不同分类。

**Q3:** LOCKDEP 报告 "possible circular locking dependency" 但程序正常运行，这正常吗？

> 正常。LOCKDEP 检测的是**潜在**死锁——两个线程执行了相反的锁序，即使它们没有同时执行。应该立即修复，不要等到生产环境死锁。

**Q4:** `lockdep_assert_held()` 有什么用？

> 运行时断言当前线程持有指定锁。如果调用时未持有锁，LOCKDEP 报告错误。用于在代码中明确函数的锁前提条件。零开销（CONFIG_LOCKDEP=n 时编译为空）。

**Q5:** LOCKDEP 的 lock class 基于什么创建？

> 基于初始化代码的调用位置（`__LINE__`）。同一行代码初始化的多个锁实例共享同一 class。静态定义的锁（`DEFINE_SPINLOCK`）各自有独立的 class。

</details>

## 交叉引用

- [05.6 ch08 并发 Bug 类型](chapter-08-lock-debug/notes/01-concurrency-bug-types.md)
- [05.6 ch08 LOCKDEP 死锁检测实践](chapter-08-lock-debug/notes/03-lockdep-deadlock-detection.md)
- [05.6 ch08 lock_stat 锁竞争统计](chapter-08-lock-debug/notes/04-lock-stat.md)
- [05.6 ch08 树莓派启用 LOCKDEP/KCSAN](chapter-08-lock-debug/notes/06-rpi-lockdep-kcsan.md)
