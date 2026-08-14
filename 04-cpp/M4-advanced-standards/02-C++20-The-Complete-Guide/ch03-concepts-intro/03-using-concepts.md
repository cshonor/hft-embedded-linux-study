# 使用 Concept

## 三种使用方式

```cpp
// 方式1：requires 子句
template <typename T>
requires std::integral<T>
T add(T a, T b) { return a + b; }

// 方式2：Concept 直接约束 auto
T add(std::integral auto a, std::integral auto b) {
    return a + b;
}

// 方式3：模板参数列表中
template <std::integral T>
T add(T a, T b) { return a + b; }
```

## 多参数约束

```cpp
// 两个参数都满足同一 Concept
template <typename T, typename U>
requires std::integral<T> && std::integral<U>
auto add(T a, U b) { return a + b; }

// 简写
auto add(std::integral auto a, std::integral auto b) {
    return a + b;
}

// 两参数类型必须相同
template <std::integral T>
T add(T a, T b) { return a + b; }
// add(1, 2L)  // ❌ int 和 long 不同
```

## 重载分派

```cpp
// 基于 Concept 的重载
void process(std::integral auto x) {
    std::cout << "integer: " << x;
}

void process(std::floating_point auto x) {
    std::cout << "float: " << x;
}

void process(const auto& x) {
    std::cout << "other: " << x;
}

process(42);    // integer
process(3.14);  // float
process("hi");  // other
```

## Concept 作为类型约束

```cpp
// 约束模板参数
template <typename T>
requires std::movable<T> && std::copyable<T>
class Container {
    T data;
};

// 约束 auto 变量
std::integral auto x = 42;     // ✅
// std::integral auto y = 3.14; // ❌ double 不是 integral

// 约束 lambda 参数
auto cmp = [](std::totally_ordered auto a, std::totally_ordered auto b) {
    return a < b;
};
```

## HFT 应用

```cpp
// 策略接口约束
template <typename S>
concept Strategy = requires(S s, const Tick& t) {
    { s.on_tick(t) } -> std::same_as<void>;
    { s.should_trade() } -> std::convertible_to<bool>;
};

// 泛型策略引擎
template <Strategy S>
class Engine {
    S strategy;
public:
    void run(const std::vector<Tick>& ticks) {
        for (const auto& t : ticks) {
            strategy.on_tick(t);
            if (strategy.should_trade()) {
                execute();
            }
        }
    }
};
```

## 自测题

1. Concept 的三种使用方式分别是什么？
2. Concept 如何实现重载分派？
3. `std::integral auto a` 和 `template<std::integral T> T a` 有什么区别？
4. Concept 能约束变量和 lambda 参数吗？
5. 用 Concept 定义一个策略接口约束，要求有 `on_tick` 和 `should_trade` 方法。
