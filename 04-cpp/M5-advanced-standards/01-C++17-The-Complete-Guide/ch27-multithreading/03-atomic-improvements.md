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

<details>
<summary>参考答案</summary>

1. `is_always_lock_free` 是 **C++17 起**的 `static constexpr bool` 成员常量：**编译期**就确定，含义是「该原子类型**在所有目标平台/所有实例上**都无锁」。
`is_lock_free()` 是 **C++11 起**的非静态成员函数：**运行期**求值，含义是「**当前这个对象**的原子操作是否无锁」——它可能依赖运行时探测（如 ARM 上是否支持 LDREXD、是否 8 字节对齐等）。
也就是说 `is_always_lock_free` 为 `true` 蕴含 `is_lock_free()` 为 `true`，反之不然。
2. 因为热路径需要**编译期决策**，而不是运行期分支：
   - 能在 `static_assert` 里把「这个类型不该被塞进热路径」变成编译错误，而不是上线后才发现有隐藏 mutex；
   - 能用 `if constexpr` 在编译期选择完全不同的实现（无锁路径 vs 加锁/拆分字段的回退路径），避免运行期分支与未使用代码的开销；
   - 还能参与模板重载/偏特化，让「无锁性」成为类型层面的契约。
3. 可以：`is_always_lock_free` 是 `static constexpr bool`，`static_assert(std::atomic<int>::is_always_lock_free, "...")` 完全合法，也是它的主要用途。
`is_lock_free()` 不行：它是非静态成员函数（不是 `constexpr`），只能在运行期 `if (a.is_lock_free())` 判断，无法写进 `static_assert`。
4. 严格说，C++17 **并没有**把默认构造的原子初始化为 0：C++17 的默认构造函数仍是 trivial 的，不做任何初始化（只有 static 存储期和 thread_local 对象会被零初始化，这是语言规则而非 atomic 的改动）。真正改成「默认构造即值初始化 `T()`」是 **C++20**（P0883）。
所以笔记里「C++17 起 `std::atomic<int> a;` 就是 0」的说法不准确，正确做法始终是**显式初始化**：
```cpp
std::atomic<int> a{0};     // 或 std::atomic<int> a = 0;
```
另外 C++20 起 `std::atomic_init` 与 `ATOMIC_VAR_INIT` 也被移除/弃用，不再需要它们来补初始化。
5. 在类型定义处直接用 `static_assert` 把「无锁」钉成编译期契约，必要时用 `if constexpr` 分流：
```cpp
struct OrderSeq {
    std::atomic<std::uint64_t> seq_num{0};
    std::atomic<int>           state{0};
};
static_assert(decltype(OrderSeq::seq_num)::is_always_lock_free);
static_assert(decltype(OrderSeq::state)::is_always_lock_free);

template <typename T>
void hot_path_publish(T& a) {
    if constexpr (std::atomic<T>::is_always_lock_free) {
        a.store(/* ... */, std::memory_order_release);   // 无锁快路径
    } else {
        /* 回退：拆字段 / 用 mutex / 改数据结构 */
    }
}
```
这样一旦有人把热路径类型换成大结构（如 24 字节以上的 `OrderContext`，`is_always_lock_free` 未必为 `true`），编译期就会暴露出来，而不是悄悄引入隐藏 mutex。

</details>
