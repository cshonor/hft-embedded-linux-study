# Item 34：优先 lambda 而非 std::bind

> 第 6 章 · Item 34 · 上一节：[Item 33 泛型 lambda](item33-generic-lambda.md)

## 为什么要学这个（先建立直觉）

C 程序员用函数指针做回调：

```c
// C：函数指针 + 手动绑定参数
void on_event(int priority, const char* msg, void* ctx) { ... }

// 想绑定 priority = 1 → C 做不到，得写包装函数
void on_event_priority1(const char* msg, void* ctx) {
    on_event(1, msg, ctx);
}
register_callback(on_event_priority1, ctx);
```

C++11 引入了 `std::bind`——可以绑定参数到函数对象：

```cpp
auto handler = std::bind(on_event, 1, std::placeholders::_1, ctx);
register_callback(handler);
```

但 `std::bind` 有很多问题：无法内联、占位符晦涩、对重载函数不友好。C++14 的 lambda 几乎在所有场景都更优：

```cpp
auto handler = [ctx](const char* msg) { on_event(1, msg, ctx); };
// 清晰、可内联、支持 move-only 类型
```

---

## 这节讲什么

C++14 起几乎所有 `std::bind` 场景都该用 lambda 替代——lambda 可内联、参数清晰、支持 move-only 类型。

---

## bind 的缺陷

### 1. 无法内联

```cpp
// bind：函数调用间接跳转，编译器难以内联
auto f = std::bind(&Widget::process, &w, _1);
f(data);  // 运行时通过函数指针调用

// lambda：编译器可内联
auto f2 = [&w](auto& data) { w.process(data); };
f2(data);  // 可能直接内联，零函数调用开销
```

### 2. 参数占位符晦涩

```cpp
// bind：_1, _2 不直观
auto f = std::bind(std::less<double>(), _1, 3.14);
f(2.0);  // 2.0 < 3.14 → true，但读 bind 表达式很难看出

// lambda：一目了然
auto f2 = [](double x) { return x < 3.14; };
```

### 3. 重载/模板函数

```cpp
// bind：传重载函数需要显式类型转换
void process(int x);
void process(double x);
auto f = std::bind(static_cast<void(*)(int)>(&process), _1);  // 难读

// lambda：不需要
auto f2 = [](int x) { process(x); };  // 清晰
```

### 4. move-only 类型

```cpp
// bind：对 unique_ptr 不友好
auto pw = std::make_unique<Widget>();
auto f = std::bind(&Widget::process, std::move(pw), _1);  // 能编译但语义绕

// lambda：直接 init capture
auto f2 = [pw = std::move(pw)](auto& data) { pw->process(data); };
```

---

## 常见错误（新手踩坑）

**错误 1：用 bind 绑定成员函数时忘了传 this**
```cpp
auto f = std::bind(&Widget::process, _1);  // 缺 this！
// 调用时需要传对象：f(widget, data) 但只设计了一个参数
```
**修正：** `auto f = std::bind(&Widget::process, &widget, _1);` 或用 lambda。

**错误 2：bind 占位符搞混参数顺序**
```cpp
auto f = std::bind(func, _2, _1);  // 第二个参数传给 func 的第一个参数
f(a, b);  // func(b, a)——反直觉
```
**修正：** 用 lambda：`auto f = [func](auto a, auto b) { func(b, a); };`

**错误 3：bind 传值导致不必要的拷贝**
```cpp
std::vector<int> big_data(10000);
auto f = std::bind(process, big_data);  // 拷贝 big_data！
// lambda 可以按引用或移动
auto f2 = [&big_data]{ process(big_data); };
auto f3 = [data = std::move(big_data)]{ process(data); };
```
**修正：** 用 lambda 按引用或移动捕获。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 回调 | 函数指针 + 包装函数 | lambda | 内联 + 类型安全 |
| 参数绑定 | 手写包装函数 | `std::bind` 或 lambda | lambda 更优 |
| 内联 | 函数指针不可内联 | lambda 可内联 | 性能 |
| move-only | 不适用 | lambda + init capture | C++14 |

**一句话总结：** C 程序员记住——新代码别用 `bind`，用 lambda。lambda 更易读、可内联、支持 move-only 类型。`bind` 是 C++11 过渡期的产物。

---

## HFT 关联

- **lambda 内联**：STL 算法传 lambda 比 `bind`/函数指针更易内联——回测里对 tick 数组批量处理时，内联 lambda 性能显著优于函数指针。
- **热路径回调**：HFT 热路径中用 lambda 做回调，编译器内联后零函数调用开销。
- **配置绑定**：`[config](const Tick& t){ check(t, config); }` 比 `bind(check, _1, config)` 更清晰且可内联。

---

## 自测题

1. `std::bind` 相比 lambda 有哪些缺陷？
2. 为什么 C++14 起几乎都该用 lambda？
3. lambda 可内联为什么对 HFT 重要？
4. 下面代码有什么问题？
```cpp
auto pw = std::make_unique<Widget>();
auto f = std::bind(&Widget::process, pw, _1);
```

<details>
<summary>参考答案</summary>

1. 主要缺陷：①**可读性差**：`_1`、`_2` 占位符与实际参数没有名字关联，读代码要先在脑子里做一遍"占位符 ↔ 形参"映射，远不如 `[config](const Tick& t){ check(t, config); }` 直白。②**难以内联**：bind 返回一个类型擦除的、通过 `operator()` 间接调用的对象，编译器通常无法看到最终调用目标，从而阻止内联与后续优化。③**捕获语义反直觉**：默认把实参**按值拷贝**进 bind 对象（想要引用必须显式 `std::ref`/`std::cref`，想要移动必须 `std::move`），而且这些拷贝在 bind 时就发生。④**转发不完美**：调用时把存储的实参一律按左值传出，原始值类别丢失（C++11 的 bind 不保留右值性）。⑤嵌套 bind 会被"立即求值"，需要额外包装才能延迟求值，行为容易搞错；重载函数名/成员函数取地址还要 `static_cast` 消歧义。

2. 因为 C++14 补齐了 lambda 相对 bind 的最后两个短板：**初始化捕获**（可以移动捕获，bind 也能做但要 `std::move` 且写法更绕）和 **`auto` 形参的泛型 lambda**（bind 的"延迟绑定/泛型"能力被覆盖）。在此之后 lambda 在可读性、可内联性、编译错误友好度、捕获控制上全面优于 bind，bind 剩下的适用场景基本只有 C++11 代码库、以及需要"部分应用 + 重排参数"的极少数场合。

3. 因为 HFT 热路径（每 tick 的过滤、聚合、回调分发）对每一次函数调用开销都敏感：lambda 的闭包类型在编译期完全已知，编译器能把它**内联**进调用点，消除函数调用与间接跳转，并进一步做常量传播、循环展开、寄存器分配优化，指令与分支预测都更友好；而 `bind`/`std::function` 通常是不可内联的间接调用（甚至有一次类型擦除的动态分派），在每 tick 执行成千上万次时会累积成可观的延迟。所以在回测/撮合循环里传 lambda 而不是 bind 或函数指针。

4. 编译失败。`std::bind` 会把实参**按值**存进 bind 对象，而 `pw` 是左值 `std::unique_ptr<Widget>`——不可拷贝，bind 内部对 `unique_ptr` 的 decay-copy 无法通过编译。必须显式移动：
```cpp
auto f = std::bind(&Widget::process, std::move(pw), _1);  // OK，但要意识到 pw 外部已空
```
不过更推荐的现代写法是用初始化捕获的 lambda，语义清晰、可内联、还能按需 `mutable`：
```cpp
auto f = [pw = std::move(pw)](auto&& x){ pw->process(std::forward<decltype(x)>(x)); };
```
另外要注意：无论哪种写法，`std::move` 之后外层的 `pw` 就变为空，再使用它是未定义行为。

</details>

---

## 参考与延伸

- 下一章：[第 7 章 并发 API](../ch07-concurrency-api/README.md)
- 回到：[第 6 章](README.md)
