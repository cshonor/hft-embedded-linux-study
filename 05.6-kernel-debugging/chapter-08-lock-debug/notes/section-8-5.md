# 8.5 KCSAN：数据竞争检测器

> 🔴 精读

## 本节要点

### KCSAN (Kernel Concurrency SANitizer)

KCSAN 检测**无锁的数据竞争**——两个线程并发访问同一变量且至少一个是写操作，没有同步机制保护。

### 工作原理

```
KCSAN 使用 watchpoint 机制:
1. 在变量访问前设置 watchpoint（记录地址）
2. 延迟一小段时间（让其他 CPU 有机会并发访问）
3. 重新检查变量值
4. 如果值变了且没有同步 → 数据竞争
```

### 启用 KCSAN

```bash
# 内核配置
CONFIG_KCSAN=y
CONFIG_KCSAN_STRICT=y    # 严格模式 (更多误报但更彻底)
CONFIG_KCSAN_REPORT_ONCE_PER_MS=1000  # 每秒最多报告1次同类

# boot 参数
# kcsan.bounded=0      — 全覆盖
# kcsan.udelay_task=80  — 任务上下文延迟 (微秒)
# kcsan.udelay_irq=80   — 中断上下文延迟
```

### KCSAN 报告示例

```
[  123.456789] ==================================================================
[  123.456790] BUG: KCSAN: data-race in my_counter_increment / my_counter_read
[  123.456795]
[  123.456800] write to 0xffff000012345678 of 4 bytes by task 1234 on cpu 0:
[  123.456805]  my_counter_increment+0x1c/0x30
[  123.456810]  my_timer_callback+0x28/0x50
[  123.456815]  call_timer_fn+0x34/0x1a0
[  123.456820]
[  123.456825] read to 0xffff000012345678 of 4 bytes by task 5678 on cpu 2:
[  123.456830]  my_counter_read+0x10/0x20
[  123.456835]  my_ioctl+0x48/0x100
[  123.456840]  __arm64_sys_ioctl+0xa4/0xc0
[  123.456845]
[  123.456850] Reported by Kernel Concurrency Sanitizer on:
[  123.456855] CPU: 2 PID: 5678 Comm: my_app
[  123.456860] Hardware name: Raspberry Pi 5
[  123.456865] ==================================================================
```

### 修复数据竞争

```c
// 错误: 无同步的共享变量
static int my_counter;
void increment(void) { my_counter++; }     // 数据竞争!
int read_counter(void) { return my_counter; } // 数据竞争!

// 修复 1: 原子操作
static atomic_t my_counter = ATOMIC_INIT(0);
void increment(void) { atomic_inc(&my_counter); }
int read_counter(void) { return atomic_read(&my_counter); }

// 修复 2: 自旋锁
static DEFINE_SPINLOCK(counter_lock);
static int my_counter;
void increment(void) { spin_lock(&counter_lock); my_counter++; spin_unlock(&counter_lock); }
int read_counter(void) { int v; spin_lock(&counter_lock); v = my_counter; spin_unlock(&counter_lock); return v; }

// 修复 3: READ_ONCE / WRITE_ONCE (如果接受非原子性，只需避免编译器优化)
int read_counter(void) { return READ_ONCE(my_counter); }
void increment(void) { WRITE_ONCE(my_counter, READ_ONCE(my_counter) + 1); }
// 注意: READ_ONCE/WRITE_ONCE 只消除编译器优化，不保证原子性
// 但 KCSAN 不会报告 READ_ONCE/WRITE_ONCE 对之间的竞争
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** KCSAN 和 LOCKDEP 检测的并发问题有什么区别？

> LOCKDEP 检测**有锁但锁序错误**的问题（死锁）。KCSAN 检测**完全无锁**的数据竞争（两个线程访问同一变量但没有同步）。LOCKDEP 需要开发者使用了锁，KCSAN 能发现开发者忘记加锁的地方。两者互补。

**Q2:** `READ_ONCE()` / `WRITE_ONCE()` 如何消除 KCSAN 报告？

> KCSAN 不会报告 `READ_ONCE()` 和 `WRITE_ONCE()` 之间的竞争——内核社区认为通过 `READ_ONCE` / `WRITE_ONCE` 标记的访问是有意为之的无锁读取。但这不保证原子性，只是告诉 KCSAN "我知道这里有竞争，这是设计如此"。


**Q:** KCSAN 和 LOCKDEP 的检测侧重点有什么不同？

> LOCKDEP 检测锁的**顺序**问题（死锁/上下文错误）。KCSAN 检测**数据竞争**——即使没有死锁，两个线程无同步地读写同一变量也是 bug。KCSAN 通过编译时插桩 + 运行时延迟观察竞争。

**Q:** KCSAN 如何检测 "已经加了锁但仍有数据竞争" 的情况？

> 如果加了锁但用了错误的锁（如读路径用 spinlock 保护但写路径忘了加锁），KCSAN 仍会报告。LOCKDEP 只看锁的顺序不看数据访问，无法发现这种"部分加锁"的 bug。两者互补。

</details>

## 交叉引用

- [05.6 ch08 LOCKDEP](chapter-08-lock-debug/notes/section-8-2.md)
