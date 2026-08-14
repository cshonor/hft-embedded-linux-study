# C++20 Lambda 扩展

## 模板 lambda

```cpp
// C++20：lambda 可以有模板参数
auto cmp = []<typename T>(const T& a, const T& b) {
    return a < b;
};

cmp(1, 2);         // T = int
cmp(1.0, 2.0);     // T = double
cmp(std::string("a"), std::string("b"));  // T = string

// 配合 Concept
auto safe_add = []<std::arithmetic T>(T a, T b) {
    return a + b;
};
safe_add(1, 2);     // ✅
// safe_add("a", "b"); // ❌ 不满足 arithmetic
```

## 捕获结构化绑定

```cpp
// C++20：lambda 可以捕获结构化绑定
auto [x, y] = std::make_pair(1, 2);

// C++17：不能捕获 x, y
// auto f = [x, y] { ... };  // 可能有问题

// C++20：合法
auto f = [x, y] { return x + y; };
```

## 捕获 [=] 的弃用

```cpp
// C++20：[=] 捕获 this 被弃用
struct Foo {
    int x;
    auto get_lambda() {
        // C++20：[=] 隐式捕获 this → 弃用警告
        // return [=] { return x; };

        // 正确：显式捕获 this
        return [this] { return x; };

        // 或 C++17：[*this] 按值捕获对象
        return [*this] { return x; };
    }
};
```

## 按值捕获 *this

```cpp
struct Counter {
    int count = 0;

    auto get_callback() {
        // [*this] 按值拷贝当前对象
        // 回调执行时即使原对象销毁也安全
        return [*this]() mutable {
            return ++count;
        };
    }
};

Counter c;
auto cb = c.get_callback();
// c 销毁后 cb 仍然安全（持有拷贝）
```

## 无状态 lambda 默认构造

```cpp
// C++20：无捕获 lambda 可默认构造和赋值
using Cmp = decltype([](int a, int b) { return a < b; });

Cmp c1;       // 默认构造（C++20）
Cmp c2 = c1;  // 拷贝构造

// 用于模板参数
std::less<int> old_cmp;  // 需要类型
Cmp new_cmp;             // 直接用 lambda 类型
```

## HFT 应用

```cpp
// 模板 lambda 做泛型回调
auto process = []<typename T>(const T& data) {
    if constexpr (std::is_same_v<T, Tick>) {
        handle_tick(data);
    } else if constexpr (std::is_same_v<T, Trade>) {
        handle_trade(data);
    }
};

// [*this] 安全回调
class Strategy {
    Config cfg;
public:
    auto get_timer_cb() {
        return [*this]() {
            // 即使 Strategy 销毁也安全
            check_timeout(cfg);
        };
    }
};
```

## 自测题

1. C++20 lambda 模板参数怎么写？有什么用？
2. `[=]` 捕获 `this` 在 C++20 有什么变化？
3. `[*this]` 和 `[this]` 的区别？什么时候用 `[*this]`？
4. 无状态 lambda 在 C++20 能默认构造吗？
5. 模板 lambda + `if constexpr` 如何做泛型回调？
