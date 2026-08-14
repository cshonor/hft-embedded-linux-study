# 7.3 属性命名空间与 using

> 第 7 章 新属性与属性扩展 · 上一节：[7.2 [[nodiscard]]](02-nodiscard.md)

## 这节讲什么

C++17 允许属性带命名空间前缀（如 `[[ns::attr]]`），并通过 `using` 简化。本节讲解属性命名空间机制和编译器扩展属性的使用。

## 属性命名空间

```cpp
// 带命名空间的属性
[[gnu::always_inline]]
[[gnu::hot]]
[[clang::fallthrough]]
[[msvc::forceinline]]

// 使用示例
[[gnu::always_inline]] inline int compute(int x) {
    return x * 2;
}
```

## 属性 using 声明

C++17 允许在属性中使用 `using` 引入命名空间，后续省略前缀：

```cpp
// C++17 之前：每个属性都要写命名空间
[[gnu::always_inline]] void f();
[[gnu::hot]] void g();
[[gnu::always_inline]] void h();

// C++17：using 引入命名空间
[[using gnu: always_inline, hot]]
void f();

// 等价于
[[gnu::always_inline, gnu::hot]]
void f();
```

### 语法

```cpp
[[using ns: attr1, attr2, attr3]]

// 等价于
[[ns::attr1, ns::attr2, ns::attr3]]
```

## 常见编译器扩展属性

### GCC/Clang (gnu namespace)

```cpp
// 常见 gnu 属性
[[gnu::always_inline]]      // 强制内联
[[gnu::hot]]                 // 标记为热点代码（优化器优先）
[[gnu::cold]]                // 标记为冷路径
[[gnu::noinline]]            // 禁止内联
[[gnu::pure]]                // 纯函数（不修改全局状态，相同输入相同输出）
[[gnu::const]]               // const 函数（连内存都不读，只依赖参数）
[[gnu::malloc]]              // 返回值是新分配的内存
[[gnu::deprecated("use X")]] // 弃用提示
[[gnu::aligned(64)]]         // 对齐到 64 字节
```

### HFT 实用属性

```cpp
// 标记热路径
[[gnu::hot]] void on_tick(const Tick& t) {
    // 高频调用，优化器优先优化
}

// 标记冷路径
[[gnu::cold]] void handle_error() {
    // 错误处理，不优化也行
}

// 强制内联
[[gnu::always_inline]] inline int fast_hash(int x) {
    return x * 2654435761u;
}

// 对齐到 cache line
struct alignas(64) [[gnu::aligned(64)]] CacheLineAligned {
    std::atomic<int> counter;
};
```

## 标准属性 vs 编译器属性

| 属性 | 来源 | 可移植性 |
|------|------|---------|
| `[[nodiscard]]` | C++17 标准 | 全平台 |
| `[[maybe_unused]]` | C++17 标准 | 全平台 |
| `[[fallthrough]]` | C++17 标准 | 全平台 |
| `[[gnu::hot]]` | GCC/Clang | GCC/Clang |
| `[[msvc::forceinline]]` | MSVC | MSVC |

### 可移植写法

```cpp
#if defined(__GNUC__) || defined(__clang__)
#define HOT [[gnu::hot]]
#define COLD [[gnu::cold]]
#else
#define HOT
#define COLD
#endif

HOT void on_tick(const Tick& t) { ... }
COLD void on_error() { ... }
```

## 属性的位置规则

属性可以出现在不同位置，含义不同：

```cpp
// 函数属性
[[nodiscard]] int f();           // 整个函数的属性

// 返回类型属性
auto f() -> [[nodiscard]] int;   // ❌ 不能在返回类型上

// 参数属性
void f([[maybe_unused]] int x);  // 参数属性

// 变量属性
[[maybe_unused]] int x = 0;      // 变量属性

// 类属性（C++20）
struct [[nodiscard]] Result {};  // 类型属性
```

## C++17 标准属性完整列表

| 属性 | 用途 |
|------|------|
| `[[noreturn]]` | 函数不会返回（`abort`、`exit`、`throw`） |
| `[[carries_dependency]]` | 内存序优化提示（少用） |
| `[[deprecated]]` | 弃用标记（C++14） |
| `[[deprecated("msg")]]` | 弃用标记 + 消息（C++14） |
| `[[fallthrough]]` | switch 穿透标记（C++17） |
| `[[nodiscard]]` | 返回值不可丢弃（C++17） |
| `[[nodiscard("msg")]]` | 返回值不可丢弃 + 消息（C++20） |
| `[[maybe_unused]]` | 抑制未使用警告（C++17） |
| `[[likely]]` / `[[unlikely]]` | 分支预测提示（C++20） |
| `[[no_unique_address]]` | 空基类优化（C++20） |

## HFT 关联

```cpp
// 热路径标记
[[gnu::hot]] void process_tick(const Tick& t) {
    // 编译器知道这是热点，更激进地优化
}

// 冷路径标记
[[gnu::cold]] void log_error(const char* msg) {
    // 错误处理不常执行，可以放在远处
}

// 强制内联热路径函数
[[gnu::always_inline]] inline double compute_pnl(Position pos, double price) {
    return pos.qty * (price - pos.avg_price);
}

// 对齐 cache line 防止 false sharing
struct alignas(64) PaddedCounter {
    alignas(64) std::atomic<uint64_t> value{0};
};
```

## 小结

| 概念 | 说明 |
|------|------|
| `[[ns::attr]]` | 命名空间属性 |
| `[[using ns: a, b]]` | using 简化多个同命名空间属性 |
| 标准属性 | 全平台可移植 |
| 编译器属性 | 平台特定，需条件编译 |

---

← [上一节](02-nodiscard.md) · [本章导读](./README.md)
