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

<details>
<summary>参考答案</summary>

1. 三种主要用法：
   1. **约束模板参数**：`template <std::integral T> void f(T);`（constrained template parameter），或 `template <typename T> requires std::integral<T> void f(T);`（requires 子句）。
   2. **requires 子句**：可出现在模板参数列表之后（`template<class T> requires C<T>`）或函数声明符之后（尾置 requires 子句），也能约束类模板：`template<typename T> requires std::movable<T> class Container {...};`
   3. **约束 `auto`（占位类型）**：变量、函数参数、返回类型、lambda 参数都能写 `std::integral auto`。
2. 靠**约束的偏序（subsumption）**：先看约束是否满足，不满足的候选直接被剔除；若仍有多于一个可行候选，且它们其他方面等价，则**约束更严格（subsumes 另一个）的那个胜出**。
```cpp
void f(std::integral auto);        // 宽松
void f(std::unsigned_integral auto); // 更严格
f(1u);   // 选更严格的版本
```
这与"函数模板偏特化排序"并存，是 C++20 新增的一条重载消解规则；如果两者的约束互不蕴含，就是二义。
3. 两者都是"约束了类型"的写法，但粒度不同：
   - `std::integral auto a`：使用**被约束的占位类型**，推导出的类型仍由实参决定，只是必须满足约束；同时它是简写形式，**不能显式指定模板实参**（写不了 `add<int>`）。
   - `template<std::integral T> T a`：显式引入了一个**具名模板参数** `T`，可以在函数体内引用 `T`、可以显式指定（`add<int>(...)`）、可以再加其他模板参数。
需要"引用类型本身"或"显式指定/特化"时用后者；只是想"要求它是整数"时前者更简洁。
4. 可以。约束 `auto` 适用于变量声明、函数参数、返回类型，也适用于 lambda 参数：
```cpp
std::integral auto x = 42;        // ✅；3.14 会编译失败

auto cmp = [](std::totally_ordered auto a,
              std::totally_ordered auto b) { return a < b; };

std::integral auto f(std::integral auto v) { return v * 2; }   // 参数与返回都约束
```
这让"接口要求"直接出现在签名里，相当于自带文档。
5. ```cpp
template <typename S>
concept Strategy = requires(S s, const Tick& t) {
    { s.on_tick(t) }     -> std::same_as<void>;          // on_tick 返回 void
    { s.should_trade() } -> std::convertible_to<bool>;   // should_trade 返回可转 bool
};

template <Strategy S>
class Engine {
    S strategy;
public:
    void run(const std::vector<Tick>& ticks) {
        for (const auto& t : ticks) {
            strategy.on_tick(t);
            if (strategy.should_trade()) execute();
        }
    }
};
```
不满足 `Strategy` 的策略类型在实例化 `Engine` 时就会报错，并指出是哪一项要求没满足。

</details>
