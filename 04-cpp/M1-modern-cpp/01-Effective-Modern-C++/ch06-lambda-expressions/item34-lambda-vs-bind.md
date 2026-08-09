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

---

## 参考与延伸

- 下一章：[第 7 章 并发 API](../ch07-concurrency-api/README.md)
- 回到：[第 6 章](README.md)
