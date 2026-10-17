# 6.3 泛型 lambda 与模板语法

> 第 6 章 Lambda 扩展 · 上一节：[6.2 捕获 this 值](02-捕获this值.md)

## 这节讲什么

C++14 引入了泛型 lambda（`auto` 参数），C++17 对其做了小幅改进。本节讲泛型 lambda 的实际用法和限制，以及 C++20 的模板 lambda 语法预告。

## C++14 泛型 lambda 回顾

```cpp
// C++14：auto 参数 = 模板参数
auto add = [](auto a, auto b) { return a + b; };

add(1, 2);          // int + int
add(1.5, 2.5);      // double + double
add(std::string("a"), std::string("b"));  // string + string
```

编译器为每种参数组合生成一个实例，等价于：

```cpp
struct __lambda {
    template<typename T1, typename T2>
    auto operator()(T1 a, T2 b) const { return a + b; }
};
```

## C++17 的改进

### constexpr 泛型 lambda

```cpp
// C++17：泛型 lambda 可以是 constexpr
constexpr auto max = [](auto a, auto b) {
    return a > b ? a : b;
};

static_assert(max(3, 5) == 5);
static_assert(max(3.0, 5.0) == 5.0);
```

### 捕获泛型 lambda

```cpp
// 把泛型 lambda 存入变量再传递
auto cmp = [](auto a, auto b) { return a < b; };

std::sort(v.begin(), v.end(), cmp);      // int 排序
std::sort(s.begin(), s.end(), cmp);      // string 排序
```

## 泛型 lambda 的实用模式

### 1. 类型擦除的打印

```cpp
auto print = [](const auto& x) {
    std::cout << x << "\n";
};

print(42);
print("hello");
print(3.14);
```

### 2. 通用访问器

```cpp
// 访问 variant 的通用 lambda
std::variant<int, double, std::string> v = 42;

std::visit([](const auto& x) {
    std::cout << "Value: " << x << "\n";
}, v);
```

### 3. 链式调用

```cpp
auto map = [](auto f) {
    return [f](auto container) {
        for (auto& x : container) x = f(x);
        return container;
    };
};

auto filter = [](auto pred) {
    return [pred](auto container) {
        container.erase(
            std::remove_if(container.begin(), container.end(),
                           [&](const auto& x) { return !pred(x); }),
            container.end());
        return container;
    };
};

std::vector<int> v = {1, 2, 3, 4, 5};
auto result = filter([](int x) { return x % 2; })(
              map([](int x) { return x * x; })(v));
// result = {1, 9, 25}
```

## 泛型 lambda 的限制

### 不能直接写模板参数

```cpp
// C++17 泛型 lambda：只能用 auto
auto f = [](auto x) { return x; };

// 不能显式指定模板参数
// f<int>(42);  // ❌ 语法不支持

// C++20 才支持模板 lambda 语法
// auto f = []<typename T>(T x) { return x; };  // C++20
// f<int>(42);  // ✅ C++20
```

### 不能对参数做 SFINAE

```cpp
// C++17：不能在 lambda 参数上做 SFINAE
// auto f = [](auto x, std::enable_if_t<std::is_integral_v<decltype(x)>>* = nullptr) { ... };
// ❌ 语法不直接支持

// 变通：用 if constexpr
auto f = [](auto x) {
    if constexpr (std::is_integral_v<decltype(x)>) {
        return x * 2;
    } else {
        return x;
    }
};
```

## C++20 模板 lambda 预告

```cpp
// C++20：显式模板参数语法
auto f = []<typename T>(T x) { return x; };

// 可以指定模板参数
f<int>(42);

// 可以约束
auto g = []<std::integral T>(T x) { return x * 2; };  // C++20 Concepts

// 可以转发包
auto h = []<typename... Args>(Args&&... args) {
    return foo(std::forward<Args>(args)...);
};
```

## HFT 关联

```cpp
// 通用数值解析
auto parse = [](const std::string& s, auto out) {
    if constexpr (std::is_integral_v<decltype(out)>) {
        return std::stoi(s);
    } else if constexpr (std::is_floating_point_v<decltype(out)>) {
        return std::stod(s);
    }
};

// 通用比较器（避免模板函数的冗长）
auto less = [](const auto& a, const auto& b) { return a < b; };

// variant 访问（消息分发）
std::visit([](const auto& msg) {
    using T = std::decay_t<decltype(msg)>;
    if constexpr (std::is_same_v<T, OrderMsg>) {
        handle_order(msg);
    } else if constexpr (std::is_same_v<T, TradeMsg>) {
        handle_trade(msg);
    }
}, message);
```

## 小结

| 特性 | C++14 | C++17 | C++20 |
|------|-------|-------|-------|
| 泛型 lambda（auto 参数） | ✅ | ✅ | ✅ |
| constexpr 泛型 lambda | ❌ | ✅ | ✅ |
| 模板参数语法 `[]<T>(T)` | ❌ | ❌ | ✅ |
| if constexpr 配合 | ❌ | ✅ | ✅ |

---

← [上一节](02-捕获this值.md) · [本章导读](./README.md)
