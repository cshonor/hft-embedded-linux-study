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

<details>
<summary>参考答案</summary>

1. 写法是在 lambda 的**参数列表前**加一个显式模板参数列表：
```cpp
auto f = []<typename T>(T x) { /* ... */ };
auto g = []<typename... Ts>(Ts&&... xs) { /* ... */ };
```
用途：
   1. 在 lambda 体内拿到**具名类型 `T`**，可用于 `if constexpr`、`static_assert`、声明局部变量——C++14 的 `auto` 泛型 lambda 拿不到类型名；
   2. 可以完美转发（`[]<typename T>(T&& x){ ... std::forward<T>(x) ... }`），`auto&&` 做不到这一点；
   3. 处理可变参数包、约束参数（`[]<std::integral T>(T x)`）都更自然。
2. C++20 **弃用了 `[=]` 隐式捕获 `this`**。
以前写 `[=]` 时，`this` 会被隐式地按指针捕获（成员访问实际走 `this->x`），这在异步回调里很容易造成悬垂却看不出来。
C++20 起应**显式**写出意图：
   - `[=, this]`：明确"其他按值、`this` 按指针捕获"；
   - `[=, *this]`：明确"其他按值、对象整体按值拷贝"。
编译器的弃用警告会提示把隐式的 `this` 捕获改写成上面两种之一。
3. `[this]`：捕获 **`this` 指针**，闭包里存的是指针，成员访问等价于 `this->m`——闭包**不能**比对象活得久，对象一销毁再调用就是未定义行为。
`[*this]`（C++17 起）：**按值拷贝整个对象** `*this` 到闭包里，闭包持有一份副本，成员访问落在这份副本上。
用 `[*this]` 的场景：闭包要"带走"当前对象并在对象销毁后继续用——异步回调、定时器、投递到别的工作线程、注册到事件循环。
```cpp
auto get_timer_cb() { return [*this]() { check_timeout(cfg); } };  // c 销毁后仍安全
```
代价是一次对象拷贝、闭包更大，且副本与原对象不再同步。
4. **可以**（C++20 起）。无捕获（无状态）lambda 的闭包类型在 C++20 拥有了**默认构造函数**和**拷贝赋值运算符**——C++20 之前只能拷贝构造，不能默认构造，也不能赋值。
```cpp
using Cmp = decltype([](int a, int b) { return a < b; });
Cmp c1;            // 默认构造（C++20 起合法）
Cmp c2 = c1;       // 拷贝构造
c1 = c2;           // 拷贝赋值（C++20 起合法）
```
这让 lambda 类型可以直接当模板实参用（如 `std::set<int, Cmp>`、`std::priority_queue<..., Cmp>`），而不必先造一个对象再 `decltype`。
注意：**有捕获**的 lambda 仍然不能默认构造，也不能赋值。
5. 把模板参数列表和 `if constexpr` 组合，一个 lambda 即为多种类型各生成一份专用代码：
```cpp
auto process = []<typename T>(const T& data) {
    if constexpr (std::is_same_v<T, Tick>)       handle_tick(data);
    else if constexpr (std::is_same_v<T, Trade>) handle_trade(data);
    else if constexpr (std::is_same_v<T, OrderBook>) handle_book(data);
};
```
要点：`[]<typename T>` 让 `T` 在 lambda 体内可见，`if constexpr` 保证只有匹配分支被实例化（其余分支对不相关类型不会报错），因此一个回调对象就能处理整个行情类型族，且是**编译期静态分派**，没有虚调用开销。

</details>
