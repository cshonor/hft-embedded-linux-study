# if constexpr 替代 SFINAE

## C++14 SFINAE 分派

```cpp
// C++14：用 enable_if 做条件重载
template <typename T,
          std::enable_if_t<std::is_integral_v<T>, int> = 0>
void process(T x) {
    std::cout << "integral: " << x;
}

template <typename T,
          std::enable_if_t<!std::is_integral_v<T>, int> = 0>
void process(T x) {
    std::cout << "other: " << x;
}

// 问题：
// 1. 两个函数签名几乎相同，冗长
// 2. enable_if 语法晦涩
// 3. 不容易扩展到更多分支
```

## C++17 if constexpr

```cpp
// C++17：编译期分支，不满足的分支不实例化
template <typename T>
void process(T x) {
    if constexpr (std::is_integral_v<T>) {
        std::cout << "integral: " << x;
        x += 1;  // 只有 T 是整数时才编译
    } else if constexpr (std::is_floating_point_v<T>) {
        std::cout << "float: " << x;
        x *= 1.1;  // 只有 T 是浮点时才编译
    } else {
        std::cout << "other: " << x;
        // x += 1 不会编译（除非走到这分支）
    }
}

process(42);    // integral: 42
process(3.14);  // float: 3.14
process("hi");  // other: hi
```

## 关键区别

```cpp
// if constexpr：不满足的分支**不实例化**
template <typename T>
void foo(T x) {
    if constexpr (std::is_integral_v<T>) {
        x.something_only_for_ints();  // T 不是 int 时不编译
    }
}

// 普通 if：两个分支都要编译
template <typename T>
void bar(T x) {
    if (std::is_integral_v<T>) {
        x.something_only_for_ints();  // ❌ T 是 string 时也编译 → 错误
    }
}
```

## 结合泛型 lambda

```cpp
// variant 访问器
std::variant<int, double, std::string> v = 42;

std::visit([](auto&& x) {
    using T = std::decay_t<decltype(x)>;
    if constexpr (std::is_same_v<T, int>) {
        std::cout << "int: " << x;
    } else if constexpr (std::is_same_v<T, double>) {
        std::cout << "double: " << x;
    } else if constexpr (std::is_same_v<T, std::string>) {
        std::cout << "string: " << x;
    }
}, v);
```

## HFT 应用

```cpp
// 行情类型分派
template <typename MarketData>
void on_data(MarketData&& data) {
    if constexpr (std::is_same_v<std::decay_t<MarketData>, Tick>) {
        process_tick(data);
    } else if constexpr (std::is_same_v<std::decay_t<MarketData>, OrderBook>) {
        process_orderbook(data);
    } else if constexpr (std::is_same_v<std::decay_t<MarketData>, Trade>) {
        process_trade(data);
    }
    // 编译期分派，无虚函数开销
}
```

## 自测题

1. `if constexpr` 和普通 `if` 的关键区别是什么？
2. `if constexpr` 不满足的分支会实例化吗？
3. C++14 SFINAE 分派有什么问题？`if constexpr` 如何解决？
4. 如何用泛型 lambda + `if constexpr` 做 variant 访问器？
5. HFT 行情类型分派如何用 `if constexpr` 实现零虚函数开销？

<details>
<summary>参考答案</summary>

1. 关键区别是**实例化时机**：`if constexpr` 的条件必须是编译期常量，且在模板实例化时，**未被选中的分支（discarded statement）不会被实例化**——因此其中对当前 `T` 不成立的代码根本不会被检查。
普通 `if` 的两个分支都要通过类型检查，即使运行期永远不会执行（如 `T` 是 `std::string` 时 `x.something_only_for_ints()` 依然会编译失败）。
补充：这个"不实例化"的优待只在**模板实体实例化期间**生效；在非模板函数中，被丢弃的分支仍会被完整检查。
2. 不会。在模板（函数模板、类模板成员、泛型 lambda 等）实例化过程中，一旦 `if constexpr` 的条件在实例化后确定，被丢弃的子语句就**不参与实例化**。
这也是它能替代 SFINAE 分派的根本原因：`if constexpr (std::is_integral_v<T>)` 的 `else` 分支里写只对浮点成立的代码，在 `T` 为整型时不会报错。
3. C++14 的 SFINAE 分派要为每个分支单独写一个重载/特化（`enable_if`、tag dispatch、类模板偏特化 + 递归终止特化），问题是：
   - 代码被**拆散**到多处，读一个函数的逻辑要在多个重载间跳转；
   - 返回类型要靠 `decltype` /  trailing return 手动统一，容易写错；
   - 重载集庞大，报错信息是几十行候选列表，可读性极差；
   - 函数**内部**的局部逻辑无法用它分派（SFINAE 只能作用于签名级别）。
`if constexpr` 让分派写在一个函数体内，顺序直观、被丢弃分支直接不实例化，也不需要为"终止条件"写额外特化。
4. 用泛型 lambda（`auto&&` 参数）+ 在 lambda 体内按 `std::decay_t<decltype(x)>` 做编译期分支：
```cpp
std::variant<int, double, std::string> v = 42;
std::visit([](auto&& x) {
    using T = std::decay_t<decltype(x)>;
    if constexpr (std::is_same_v<T, int>)         std::cout << "int: " << x;
    else if constexpr (std::is_same_v<T, double>) std::cout << "double: " << x;
    else if constexpr (std::is_same_v<T, std::string>) std::cout << "string: " << x;
}, v);
```
`std::visit` 会为每种替代类型实例化一次 lambda，各自只保留匹配的分支——一个 lambda 就覆盖了整个访问器。
5. 把类型判断放到编译期，直接分派到具体处理函数，不经过虚表和函数指针：
```cpp
template <typename MarketData>
void on_data(MarketData&& data) {
    using T = std::decay_t<MarketData>;
    if constexpr (std::is_same_v<T, Tick>)       process_tick(data);
    else if constexpr (std::is_same_v<T, OrderBook>) process_orderbook(data);
    else if constexpr (std::is_same_v<T, Trade>)  process_trade(data);
}
```
每个 `T` 只实例化出一条具体路径，调用是**静态绑定**、可被内联，没有虚函数调用的间接跳转与 icache 污染，也没有动态多态的对象布局要求——等价于为每种行情类型手写一份专用函数。

</details>
