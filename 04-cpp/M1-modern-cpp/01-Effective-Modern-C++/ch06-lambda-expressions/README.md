# 第 6 章 Lambda 表达式

**Lambda Expressions** — Items 31–34

## 本章讲什么

Lambda 是 C++11 最受欢迎的特性之一——它让"在调用点定义回调"成为可能，替代了冗长的仿函数类。但 lambda 的捕获、闭包生命周期、`auto&&` 参数转发有不少陷阱。本章讲清捕获语义、初始化捕获（C++14）、泛型 lambda（C++14）与 `std::bind` 的取舍。

---

## 各 Item 要点

### Item 31：避免默认捕获模式（`[=]` / `[&]`）

默认捕获有两大问题：
1. **`[=]` 的误导**：看着像"按值捕获一切"，但**只捕获用到的变量**；且对 `static` 局部变量、成员变量的捕获语义反直觉（成员变量实际捕获的是 `this` 指针，按引用！）。
2. **`[&]` 的悬垂**：按引用捕获的局部变量，闭包存活超过作用域时引用悬垂——UB。

```cpp
class Widget {
    int data;
    auto make_cb() { return [=]{ return data; }; }  // 捕获的是 this，不是 data 的拷贝！
};
```

**建议**：显式列出捕获变量（`[x]`、`[&mtx]`），避免默认模式；尤其警惕 `[=]` 捕获 `this` 的隐式行为。C++17 可用 `[*this]` 按值捕获对象本身。

### Item 32：用初始化捕获将对象移入闭包（C++14）

初始化捕获（init capture）能在捕获时执行表达式并命名：

```cpp
auto pw = std::make_unique<Widget>();
auto cb = [up = std::move(pw)]{ up->doSomething(); };  // 把 unique_ptr 移入闭包
```

`up = std::move(pw)` 在闭包里创建 `up`（按值，即移动），彻底解决"想捕获移动语义"的需求——C++11 做不到。C++11 的变通是 `std::bind` 包裹，但更绕。

### Item 33：对 `auto&&` 形参用 `decltype` + `std::forward`（泛型 lambda）

C++14 泛型 lambda 用 `auto&&` 参数实现完美转发：

```cpp
auto f = [](auto&& x){ func(std::forward<decltype(x)>(x)); };
```

`decltype(x)` 对 `auto&&`（万能引用）参数保留左右值性，`forward` 原样转发。这让 lambda 能当泛型转发器用。

### Item 34：优先 lambda 而非 `std::bind`

`std::bind` 的缺陷：
- 无法内联（C++11 的 bind 是函数调用，lambda 可内联）；
- 参数占位符晦涩（`_1`、`_2`）；
- 重载函数 / 模板函数传给 bind 需要显式类型转换，lambda 不需要；
- bind 的值传递语义对 `move`-only 类型（`unique_ptr`）不友好。

C++14 起几乎所有 `bind` 场景都该用 lambda 替代。`bind` 仅在极少数"运行时组合调用链"的场景仍有价值。

---

## HFT 关联

- **lambda 作策略回调**：HFT 策略引擎常注册 lambda 作回调（`engine.on_tick([this](const Tick& t){ ... })`）。注意捕获 `this` 的生命周期——策略对象销毁后引擎仍调闭包 = 悬垂。配合 `weak_ptr` 或显式注销回调规避。
- **`[&]` 悬垂是热路径隐患**：行情循环里 `[&]` 捕获局部变量，若闭包被异步保存到队列稍后执行，局部变量已销毁——这是 HFT 异步日志/事件队列里最难查的 UAF。规则：跨作用域存储的闭包用值捕获或 `shared_ptr`。
- **lambda 内联**：STL 算法（`sort`/`for_each`/`transform`）传 lambda 比 `bind`/函数指针更易内联——HFT 回测里对 tick 数组批量处理时，内联 lambda 的性能显著优于函数指针。

---

## 自测题

1. `[=]` 在成员函数里捕获成员变量时，实际捕获的是什么？为什么这是隐患？
2. C++14 初始化捕获 `[up = std::move(pw)]` 解决了 C++11 的什么限制？
3. 泛型 lambda 的 `auto&&` 参数如何配合 `std::forward` 实现完美转发？
4. `std::bind` 相比 lambda 有哪些缺陷？为什么 C++14 起几乎都该用 lambda？
5. 异步保存的闭包为什么不能用 `[&]`？该用什么替代？



## 代码自测

### Q1: [=] 捕获 this

```cpp
class Engine {
    int data = 42;
public:
    auto get_cb() {
        return [=]() { return data; };  // 捕获的是什么？
    }
};
```

> `[=]` 捕获的是 `data` 的副本吗？

<details>
<summary>答案与复习指引</summary>

**不是。** `[=]` 在成员函数中捕获的是 `this` 指针（按值），不是 `data` 的副本。闭包通过 `this->data` 访问成员——如果 `Engine` 对象先于闭包销毁，`this` 悬垂，UB。

**修复（C++17）：** `[*this]()` 按值捕获对象本身的副本。
**修复（C++11/14）：** `auto d = data; return [d]() { return d; };`（先拷贝到局部变量再捕获）。

**教训：** 避免用 `[=]` 默认捕获，显式列出要捕获的变量。

**复习：** → [Item 31：避免默认捕获模式](item31-avoid-default-capture.md)
</details>

### Q2: 初始化捕获（C++14）

```cpp
auto pw = std::make_unique<Widget>();
auto cb = [up = std::move(pw)]() {
    up->doSomething();
};
// pw 现在是什么状态？
```

> `pw` 在 lambda 创建后是什么状态？这个模式叫什么？

<details>
<summary>答案与复习指引</summary>

**`pw` = nullptr（空）。** `up = std::move(pw)` 把 `unique_ptr` 移入闭包。这叫"初始化捕获"（init capture），C++14 新特性。

**解决的问题：** C++11 的 `[=]`/`[&]` 无法捕获移动语义——只能拷贝或引用。初始化捕获允许在捕获时执行任意表达式（包括 `move`），把结果存入闭包。

**C++11 变通：** `std::bind` + `std::move`，但更绕。

**复习：** → [Item 32：用初始化捕获将对象移入闭包](item32-init-capture.md)
</details>
