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
