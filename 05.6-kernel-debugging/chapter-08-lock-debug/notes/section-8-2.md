# 8.2 LOCKDEP：锁依赖检测器

> 🔴 精读

## 本节要点

### LOCKDEP 工作原理

LOCKDEP 在每次锁获取/释放时记录锁的依赖关系，构建**锁依赖图**。如果图中出现环，说明存在潜在死锁。

```
锁依赖图示例:
  A → B  (持有 A 时获取 B)
  B → C  (持有 B 时获取 C)
  C → A  (持有 C 时获取 A)  ← 环! 报告死锁
```

### 启用 LOCKDEP

```bash
# 内核配置
CONFIG_LOCKDEP=y
CONFIG_LOCKDEP_SUPPORT=y

# boot 参数
# lockdep                 — 启用
# nolockdep               — 禁用

# 性能开销: 每次加锁额外 ~100-200ns，适合开发/测试
# 不适合生产环境 (除非 HFT staging)
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
[  123.456880] other info that might help us debug this:
[  123.456885]  Possible unsafe locking scenario:
[  123.456890]        CPU0                    CPU1
[  123.456895]        ----                    ----
[  123.456900]   lock(&my_lock_a);
[  123.456905]                                lock(&my_lock_b);
[  123.456910]                                lock(&my_lock_a);
[  123.456915]   lock(&my_lock_b);  ← DEADLOCK!
```

### LOCKDEP 检测的问题类型

| 问题 | 说明 |
|------|------|
| **Circular dependency** | AB-BA 死锁 |
| **Reacquire** | AA 死锁 |
| **IRQ lock inversion** | 进程上下文和中断上下文锁序冲突 |
| **Lock used after free** | 释放后继续使用锁 |
| **Incorrect lock class** | 锁分类错误 |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** LOCKDEP 能检测尚未实际发生的死锁吗？

> 能。LOCKDEP 构建全局锁依赖图，只要两个线程执行了相反的锁顺序（即使没有同时执行），LOCKDEP 就会报告 "possible circular locking dependency"。这比等死锁实际发生再检测更安全——因为死锁可能只在特定时序下触发。

**Q2:** LOCKDEP 的锁分类 (lock class) 是什么概念？

> 每个 `spinlock_init()` / `mutex_init()` 创建一个新的锁分类。LOCKDEP 按分类而非实例追踪依赖——同一分类的多个实例共享同一依赖关系。这减少内存开销，但要求同类的锁确实有相同的获取顺序。用 `lockdep_set_class()` 可以为同一实例设置不同分类。

</details>
