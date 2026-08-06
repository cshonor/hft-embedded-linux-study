# D.3 atomic 原子操作

> 附录 D · 上一节：[D.2 mutex 互斥锁](02-mutex.md) · 下一节：[D.4 future 异步结果](04-future.md)

## 这节讲什么

`<atomic>` 头文件提供原子操作和内存序。本节是速查参考——`std::atomic<T>` 的接口、六种内存序、`atomic_flag`、以及 `is_lock_free` 检查。

---

## 核心规则（代码+表格）

### `std::atomic<T>` 接口

| 接口 | 说明 |
|------|------|
| `load(order)` | 原子读 |
| `store(val, order)` | 原子写 |
| `exchange(val, order)` | 原子交换（返回旧值） |
| `compare_exchange_weak/exp(expected, desired, ...)` | CAS |
| `fetch_add/sub/and/or/xor(val, order)` | 原子算术（返回旧值） |
| `is_lock_free()` | 是否无锁实现 |
| `wait(old, order)` | 阻塞直到值变化（C++20） |
| `notify_one/all()` | 唤醒等待者（C++20） |

```cpp
std::atomic<int> counter{0};

// 读
int v = counter.load(std::memory_order_acquire);
int v2 = counter;  // 等价于 load(seq_cst)

// 写
counter.store(42, std::memory_order_release);
counter = 42;  // 等价于 store(seq_cst)

// 交换
int old = counter.exchange(99);

// 算术
int prev = counter.fetch_add(1);  // counter += 1，返回旧值
counter++;  // 等价于 fetch_add(1, seq_cst)
counter += 5;  // 等价于 fetch_add(5, seq_cst)

// CAS
int expected = 42;
bool success = counter.compare_exchange_weak(
    expected, 100,  // 如果 counter==42，改成 100
    std::memory_order_acq_rel,
    std::memory_order_relaxed);
if (!success) {
    // counter != 42，expected 被更新为当前值
}
```

### 六种内存序

| 内存序 | 语义 | 开销 | 用途 |
|--------|------|------|------|
| `relaxed` | 无同步，仅原子 | 最低 | 计数器、统计 |
| `consume` | 数据依赖（极少用） | 低 | 几乎不用 |
| `acquire` | 读后不重排 | 低 | 读取同步信号 |
| `release` | 写前不重排 | 低 | 发布数据 |
| `acq_rel` | acquire + release | 中 | CAS（读写都做） |
| `seq_cst` | 全局顺序（默认） | 最高 | 默认，最安全 |

```cpp
// release/acquire 配对：发布-消费模式
std::atomic<bool> ready{false};
int data = 0;

// 线程1（生产者）
data = 42;
ready.store(true, std::memory_order_release);  // release：data 的写入对消费者可见

// 线程2（消费者）
while (!ready.load(std::memory_order_acquire));  // acquire：看到 ready=true 后，data 一定是 42
assert(data == 42);  // 保证成立

// relaxed：无同步
std::atomic<int> counter{0};
counter.fetch_add(1, std::memory_order_relaxed);  // 只保证原子，不保证可见性

// seq_cst：最安全但最慢（默认）
ready.store(true);  // 等价于 seq_cst
```

### `atomic_flag`：最简单的原子类型

```cpp
// atomic_flag：只能 test-and-set / clear
// 保证无锁（is_lock_free == true）
std::atomic_flag flag = ATOMIC_FLAG_INIT;

// test-and-set：设为 true，返回旧值
bool was_set = flag.test_and_set(std::memory_order_acquire);
if (!was_set) {
    // 之前是 false，现在被设为 true
    // 可以用来实现自旋锁
}

// clear：设为 false
flag.clear(std::memory_order_release);

// 自旋锁实现
class spinlock {
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
public:
    void lock() {
        while (flag.test_and_set(std::memory_order_acquire)) {
            // 自旋等待
        }
    }
    void unlock() {
        flag.clear(std::memory_order_release);
    }
};
```

### `is_lock_free` 检查

```cpp
std::atomic<int> a;
std::atomic<BigStruct> b;  // 大结构体

if (a.is_lock_free()) {
    // int 通常是 lock-free（单条 CPU 指令）
}

if (b.is_lock_free()) {
    // 大结构体可能不是 lock-free（内部用 mutex）
} else {
    // 这种情况下 atomic 有锁开销，和 mutex 差不多
}

// C++17: is_always_lock_free（编译期常量）
static_assert(std::atomic<int>::is_always_lock_free);
```

### `wait` / `notify`（C++20）

```cpp
std::atomic<int> value{0};

// 线程1：等待 value 变化
value.wait(0, std::memory_order_acquire);  // 如果 value==0，阻塞
// value != 0 时继续

// 线程2：修改并通知
value.store(42, std::memory_order_release);
value.notify_one();  // 唤醒一个等待者
// 或 notify_all() 唤醒所有
// 类似 condition_variable，但更轻量
```

---

## 新手要点（和 C 的区别）

- **C 用 `volatile` 或 `__sync_*` 内建函数**：C 的原子操作不标准——GCC 用 `__sync_fetch_and_add`，MSVC 用 `_InterlockedExchange`。C++11 的 `std::atomic` 统一了接口，跨平台。
- **C11 的 `_Atomic` 和 C++ 的 `atomic`**：C11 有 `_Atomic` 但实现不完善。C++ 的 `std::atomic` 更成熟、更广泛使用。C 程序员转型 C++ 后应该用 `std::atomic`。
- **内存序是 C 程序员的新概念**：C 程序员要么用 `volatile`（错误），要么用全屏障 `__sync_synchronize`（过度）。C++ 的六种 `memory_order` 给了精确控制——这是 C++ 相比 C 的巨大进步。
- **`atomic_flag` 保证无锁**：C 程序员可能觉得"原子操作肯定无锁"——但 `atomic<BigStruct>` 可能内部用 mutex。`atomic_flag` 是唯一保证无锁的原子类型。

---

## HFT 关联

- **`atomic` 是 HFT 的核心同步工具**：HFT 的计数器、序号、状态标志用 `atomic`——无锁、低延迟。
- **`acquire/release` 优于 `seq_cst`**：x86 上 acquire/release 编译为普通 `mov`（x86 TSO 天然满足），seq_cst 需要 `mfence`。HFT 热路径用 acquire/release。
- **`is_lock_free` 检查**：HFT 中如果用 `atomic<struct>`，要检查 `is_lock_free`——如果不 lock-free，等于用了隐藏的 mutex。
- **`wait/notify` 用于管理面**：C++20 的 `atomic::wait/notify` 比 `condition_variable` 轻量——HFT 管理面可以用。但热路径不用（阻塞操作）。
- **`atomic_flag` 自旋锁**：HFT 的低竞争临界区可以用 `atomic_flag` 自旋锁——比 mutex 轻（无系统调用）。但高竞争下自旋浪费 CPU。

---

## 自测题

1. `load`/`store`/`exchange`/`fetch_add` 各做什么？
2. 六种内存序中，`relaxed`/`acquire`/`release`/`seq_cst` 各有什么语义？
3. `compare_exchange_weak` 和 `strong` 有什么区别？
4. `atomic_flag` 和 `atomic<bool>` 有什么区别？哪个保证无锁？
5. 为什么 HFT 用 `acquire/release` 而非 `seq_cst`？

---

## 参考与延伸

- 下一节：[D.4 future 异步结果](04-future.md)
- 上一节：[D.2 mutex 互斥锁](02-mutex.md)
- 回到：[附录 D](README.md)
