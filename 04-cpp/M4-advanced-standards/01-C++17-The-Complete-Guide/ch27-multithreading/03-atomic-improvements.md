# C++17 原子改进

## is_always_lock_free

```cpp
#include <atomic>

// C++17：编译期常量，保证该原子类型在所有平台上都无锁
static_assert(std::atomic<int>::is_always_lock_free);
// int 的原子操作在所有主流平台上都是无锁的

// 大结构可能不是无锁的
struct BigStruct { int data[16]; };
// std::atomic<BigStruct>::is_always_lock_free 可能是 false
// → 内部可能有 mutex

// 编译期检查
template <typename T>
void process_atomic() {
    static_assert(std::atomic<T>::is_always_lock_free,
        "T must be lock-free atomic");
    // ...
}
```

## is_always_lock_free vs is_lock_free()

```cpp
// C++11：is_lock_free() — 运行期
std::atomic<BigStruct> a;
if (a.is_lock_free()) {
    // 运行时才知道——可能无锁，可能有内部 mutex
    // 取决于 CPU 和实现
}

// C++17：is_always_lock_free — 编译期常量
if constexpr (std::atomic<BigStruct>::is_always_lock_free) {
    // 编译期确定——所有平台上都无锁
} else {
    // 可能有内部锁——热路径不应使用
}
```

**区别**：
| 特性 | `is_lock_free()` | `is_always_lock_free` |
|------|-----------------|---------------------|
| 求值时机 | 运行期 | 编译期 |
| 能否 static_assert | ❌ | ✅ |
| 语义 | 当前实例是否无锁 | 该类型在所有平台是否无锁 |

## 原子初始化

```cpp
// C++17：原子默认初始化为 0（以前是未初始化）
std::atomic<int> a;  // C++17 起：a == 0
// C++11/14：a 的值未定义！需要 atomic_init 或构造参数

// 但最好还是显式初始化
std::atomic<int> b{0};
```

## HFT 应用

```cpp
// 1. 编译期保证热路径原子无锁
struct OrderSeq {
    std::atomic<uint64_t> seq_num{0};
    std::atomic<int> state{0};
};
static_assert(decltype(OrderSeq::seq_num)::is_always_lock_free);
static_assert(decltype(OrderSeq::state)::is_always_lock_free);
// 编译期保证——热路径不会有隐藏 mutex

// 2. 大结构原子的检查
struct OrderContext {
    int sym_id;
    double price;
    int qty;
    uint32_t flags;
};
static_assert(sizeof(OrderContext) == 24);  // 3 个 cache line 内
// std::atomic<OrderContext>::is_always_lock_free？
// 24 字节——x86 上可能用 CMPXCHG16B（需要 16 字节对齐）
// 检查：
static_assert(std::atomic<OrderContext>::is_always_lock_free);
// 如果 false，改用 relaxed memory order + 手动 CAS 或拆分字段
```

## 自测题

1. `is_always_lock_free` 和 `is_lock_free()` 的区别？
2. 为什么需要编译期版本的 `is_always_lock_free`？
3. `is_always_lock_free` 能用于 `static_assert` 吗？`is_lock_free()` 呢？
4. C++17 对原子默认初始化做了什么改变？
5. HFT 热路径如何用 `is_always_lock_free` 保证无锁？
