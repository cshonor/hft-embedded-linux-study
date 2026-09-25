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

<details>
<summary>参考答案</summary>

1. 自下而上递进：
   - **`std::movable`**：可移动构造/赋值 + 可交换（`swappable`）。
   - **`std::copyable`** = `movable` + 可拷贝构造/赋值（拷贝后是独立副本）。
   - **`std::semiregular`** = `copyable` + `default_initializable`（可默认构造）。
   - **`std::regular`** = `semiregular` + `equality_comparable`（可 `==` 比较）。
即：`movable` → `copyable` → `semiregular` → `regular`，每一层都在上一层基础上加一项能力。
2. `std::regular` 要求该类型"**像 int 一样**"，即：
   1. 可默认构造（`default_initializable`）；
   2. 可拷贝构造与拷贝赋值（`copyable`）；
   3. 可移动、可析构、可交换；
   4. 可用 `==` 比较（`equality_comparable`）。
```cpp
static_assert(std::regular<Tick>);   // 默认构造 + 拷贝 + 相等比较
```
满足 `regular` 的类型可以安全地放进任何容器、参与任何泛型算法，是"值语义类型"的标准画像。
3. `std::equality_comparable<T>`：支持 `==`（以及由此得到的 `!=`），且 `==` 构成**等价关系**（自反、对称、传递）。
`std::totally_ordered<T>`：在相等比较之上还支持 `<`、`>`、`<=`、`>=`，且构成**全序**（严格弱序 + 任意两个值可比）。
所以 `totally_ordered` **蕴含** `equality_comparable`——能全序比较的一定能判等，反之不成立。需要排序/放入有序容器时用 `totally_ordered`，只需去重/查找时用 `equality_comparable`。
4. `std::invocable<F, Args...>`：只要求 `F` 能用 `Args...` 调用（INVOKE 语义），**不关心返回类型**（返回 `void` 也算）。
`std::predicate<F, Args...>`：在 `invocable` 之上，要求返回类型**可转换为 bool**（boolean-testable），即"这是个判断式"。
另外 `std::regular_invocable` 在 `invocable` 之上还要求"**纯函数**语义"：同样参数调用结果相同、不修改函数对象、不修改参数——标准库算法据此才敢复制函数对象、多次调用。
5. 用 `std::predicate`：
```cpp
template <typename CB>
requires std::predicate<CB, const Tick&>
void on_tick(CB&& cb, const Tick& t) {
    if (cb(t)) { /* trade */ }
}
```
它等价于"可调用且返回可转 bool"。如果还想要求不修改状态，可用 `std::regular_invocable` 加 `std::convertible_to<std::invoke_result_t<CB, const Tick&>, bool>` 自行组合。

</details>
