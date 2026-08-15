# KCSAN：数据竞争检测器

> 🔴 精读

## 概念详解

### KCSAN 是什么

KCSAN (Kernel Concurrency SANitizer) 检测**无锁的数据竞争**——两个线程并发访问同一变量且至少一个是写操作，没有同步机制保护。与 LOCKDEP 互补。

### 工作原理

```
KCSAN 使用 watchpoint 机制:
1. 在变量访问前设置 watchpoint（记录地址和当前值）
2. 延迟一小段时间（让其他 CPU 有机会并发访问）
3. 重新检查变量值
4. 如果值变了且没有同步 → 数据竞争!

延迟时间: udelay_task=80μs, udelay_irq=80μs
```

### KCSAN vs KASAN

| 特性 | KCSAN | KASAN |
|------|-------|-------|
| 检测目标 | 数据竞争（并发访问） | 内存错误（越界/use-after-free） |
| 需要并发 | 是（需要多线程触发） | 否（单线程即可） |
| 插桩方式 | 编译时插桩 | 编译时插桩 |

### 启用 KCSAN

```bash
# 内核配置
CONFIG_KCSAN=y
CONFIG_KCSAN_STRICT=y              # 严格模式
CONFIG_KCSAN_REPORT_ONCE_PER_MS=1000  # 每秒最多报告1次同类

# boot 参数
# kcsan.udelay_task=80     — 任务上下文延迟 (微秒)
# kcsan.udelay_irq=80      — 中断上下文延迟
# kcsan.skip_watch=4000    — 每 N 次访问检查一次
```

### KCSAN 报告示例

```
[  123.456789] ==================================================================
[  123.456790] BUG: KCSAN: data-race in my_counter_increment / my_counter_read
[  123.456800] write to 0xffff000012345678 of 4 bytes by task 1234 on cpu 0:
[  123.456805]  my_counter_increment+0x1c/0x30
[  123.456810]  my_timer_callback+0x28/0x50
[  123.456825] read to 0xffff000012345678 of 4 bytes by task 5678 on cpu 2:
[  123.456830]  my_counter_read+0x10/0x20
[  123.456835]  my_ioctl+0x48/0x100
```

### 修复数据竞争

```c
// 错误: 无同步的共享变量
static int my_counter;
void increment(void) { my_counter++; }      // 数据竞争!

// 修复 1: 原子操作 (推荐)
static atomic_t my_counter = ATOMIC_INIT(0);
void increment(void) { atomic_inc(&my_counter); }
int read_counter(void) { return atomic_read(&my_counter); }

// 修复 2: 自旋锁
static DEFINE_SPINLOCK(counter_lock);
void increment(void) {
    spin_lock(&counter_lock);
    my_counter++;
    spin_unlock(&counter_lock);
}

// 修复 3: READ_ONCE / WRITE_ONCE (接受非原子性)
int read_counter(void) { return READ_ONCE(my_counter); }
void increment(void) {
    WRITE_ONCE(my_counter, READ_ONCE(my_counter) + 1);
}
// KCSAN 不会报告 READ_ONCE/WRITE_ONCE 对之间的竞争
```

### READ_ONCE / WRITE_ONCE 详解

```c
// 作用:
// 1. 防止编译器优化（合并多次读/写、推测性读）
// 2. 告诉 KCSAN "我知道这里有竞争，这是设计如此"
// 3. 不保证原子性（仍可能读到中间值）

// 适用: 轮询标志位、统计计数器、RCU 保护的数据读取
// 不适用: 需要精确计数、多字段一致性
```

### HFT 关联应用

```c
// HFT 模块中常见的数据竞争场景

// 场景 1: 统计计数器（用 per-CPU）
static DEFINE_PER_CPU(u64, packets_received);
void on_packet(void) {
    this_cpu_inc(packets_received);  // 无竞争
}

// 场景 2: 序列号（用 atomic）
static atomic64_t seq_num = ATOMIC64_INIT(0);
u64 get_seq(void) { return atomic64_inc_return(&seq_num); }

// 场景 3: 配置标志（用 READ_ONCE/WRITE_ONCE）
static bool reload_flag = false;
void trigger_reload(void) { WRITE_ONCE(reload_flag, true); }
bool check_reload(void) { return READ_ONCE(reload_flag); }

// 场景 4: 共享数据结构（用 RCU 或锁）
static struct config __rcu *current_config;
void update_config(struct config *new) {
    old = rcu_dereference_protected(current_config, ...);
    rcu_assign_pointer(current_config, new);
    synchronize_rcu();
    kfree(old);
}
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** KCSAN 和 LOCKDEP 检测的并发问题有什么区别？

> LOCKDEP 检测**有锁但锁序错误**的问题（死锁）。KCSAN 检测**完全无锁**的数据竞争。LOCKDEP 需要开发者使用了锁，KCSAN 能发现开发者忘记加锁的地方。两者互补。

**Q2:** `READ_ONCE()` / `WRITE_ONCE()` 如何消除 KCSAN 报告？

> KCSAN 不会报告 `READ_ONCE()` 和 `WRITE_ONCE()` 之间的竞争——内核社区认为通过标记的访问是有意为之的无锁读取。但不保证原子性。

**Q3:** KCSAN 如何检测 "已经加了锁但仍有数据竞争" 的情况？

> 如果加了锁但用了错误的锁（如读路径用 spinlock 保护但写路径忘了加锁），KCSAN 仍会报告。LOCKDEP 只看锁的顺序不看数据访问，无法发现这种"部分加锁"的 bug。

**Q4:** HFT 模块中什么时候用 atomic_t vs READ_ONCE/WRITE_ONCE vs spinlock？

> (1) `atomic_t`：需要精确计数（如序号、引用计数）。(2) `READ_ONCE/WRITE_ONCE`：偶尔丢更新可接受（如统计计数器）。(3) `spinlock`：需要保护多个相关变量。(4) `RCU`：读多写少的数据结构。

**Q5:** KCSAN 的工作原理是什么？

> 使用 watchpoint 机制：(1) 在变量访问前设置 watchpoint 记录地址和当前值；(2) 延迟一小段时间让其他 CPU 有机会并发访问；(3) 重新检查变量值；(4) 如果值变了且没有同步机制 → 报告数据竞争。

</details>

## 交叉引用

- [05.6 ch08 并发 Bug 类型](chapter-08-lock-debug/notes/01-concurrency-bug-types.md)
- [05.6 ch08 LOCKDEP 锁依赖检测器](chapter-08-lock-debug/notes/02-lockdep.md)
- [05.6 ch08 树莓派启用 LOCKDEP/KCSAN](chapter-08-lock-debug/notes/06-rpi-lockdep-kcsan.md)
