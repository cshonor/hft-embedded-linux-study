# 对象语义 Concept

## 核心 Concept

```cpp
// movable：可移动
std::movable<T>  // 有 move 构造/赋值，move 构造不抛异常

// copyable：可拷贝
std::copyable<T> // 有 copy 构造/赋值

// default_initializable：可默认构造
std::default_initializable<T>  // T{} 或 T() 合法

// semiregular：半正则 = copyable + default_initializable
std::semiregular<T>

// regular：正则 = semiregular + equality_comparable
std::regular<T>  // 像 int 一样行为
```

## 层次关系

```
movable
  └── copyable
        └── semiregular (= copyable + default_initializable)
              └── regular (= semiregular + equality_comparable)
```

## 实际应用

```cpp
// 约束容器元素
template <typename T>
requires std::regular<T>
class Vector {
    // T 可以默认构造、拷贝、比较
};

// 约束策略对象
template <typename S>
requires std::movable<S>
class Engine {
    S strategy;
    // S 可以移动（不能拷贝的独占资源策略）
};

// HFT：行情数据通常是 regular
struct Tick {
    int sym_id;
    double price;
    int qty;

    auto operator<=>(const Tick&) const = default;
};
static_assert(std::regular<Tick>);  // 默认构造 + 拷贝 + 比较
```

## 比较 Concept

```cpp
// equality_comparable：有 == 和 !=
std::equality_comparable<T>

// totally_ordered：有 <, >, <=, >= 且全序
std::totally_ordered<T>

// 三路比较
std::three_way_comparable<T>
std::three_way_comparable<T, std::strong_ordering>

// 层次：
// regular = semiregular + equality_comparable
// totally_ordered 蕴含 equality_comparable
```

## 可调用 Concept

```cpp
// invocable：可调用
std::invocable<F, Args...>  // F(Args...) 合法

// regular_invocable：可调用且不修改参数（纯函数）
std::regular_invocable<F, Args...>

// predicate：返回 bool 的可调用对象
std::predicate<F, Args...>  // F(Args...) 返回 bool

// 使用
template <typename F, typename... Args>
requires std::invocable<F, Args...>
auto call(F&& f, Args&&... args) {
    return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
}

// 约束回调
template <typename CB>
requires std::predicate<CB, const Tick&>
void on_tick(CB&& cb, const Tick& t) {
    if (cb(t)) { /* trade */ }
}
```

## 自测题

1. `movable`、`copyable`、`semiregular`、`regular` 的层次关系？
2. `regular` 要求哪些能力？
3. `equality_comparable` 和 `totally_ordered` 的区别？
4. `invocable` 和 `predicate` 的区别？
5. 如何约束一个回调必须是返回 bool 的可调用对象？
