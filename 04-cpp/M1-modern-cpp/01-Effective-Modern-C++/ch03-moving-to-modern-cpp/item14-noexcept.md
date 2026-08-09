# Item 14：声明 noexcept 如果函数保证不抛

> 第 3 章 移步现代 C++ · Item 14 · 上一节：[Item 13 const_iterator](item13-const-iterator.md)

## 为什么要学这个（先建立直觉）

C 没有异常机制。C 程序员习惯用返回值表示错误：

```c
int process(const char* data) {
    if (data == NULL) return -1;    // 错误码
    // ... 处理 ...
    return 0;                        // 成功
}
```

C++ 有异常。函数可以抛异常而非返回错误码。但异常有代价——编译器需要生成异常处理表，运行时栈展开有开销。`noexcept` 是 C++11 引入的关键字，告诉编译器"这个函数保证不抛异常"，编译器据此做优化。

**最关键的应用场景：** STL 容器在 `push_back` 扩容时会检查元素的移动构造是否 `noexcept`。如果 `noexcept`，用移动（O(1) per element，快）；否则退回拷贝（安全但慢）。这个检查是**编译期**的——你标不标 `noexcept` 直接决定了容器的运行时性能。

```cpp
class Widget {
public:
    // 标了 noexcept → vector 扩容用移动 → 快
    Widget(Widget&& other) noexcept;
    // 没标 noexcept → vector 扩容退回拷贝 → 慢
    Widget(const Widget& other);
};
```

---

## 这节讲什么

`noexcept` 是函数接口契约的一部分。对**移动构造**、**swap**、**析构**标 `noexcept` 尤其关键——STL 容器在 `push_back` 扩容时会检查元素移动构造是否 `noexcept`：是则用移动（快），否则退回拷贝（安全）。

---

## 核心机制

### noexcept 影响容器性能

```cpp
class Widget {
public:
    Widget(Widget&& other) noexcept;  // noexcept 移动构造
    Widget& operator=(Widget&& other) noexcept;
};

// STL 容器扩容时的分派逻辑（简化）：
// vector::push_back 扩容时
if (is_nothrow_move_constructible_v<T>)
    move old elements;    // O(1) per element，快
else
    copy old elements;    // O(n) per element，安全

// 标了 noexcept → is_nothrow_move_constructible 为 true → 走移动
// 没标 noexcept → 可能走拷贝——哪怕你写了移动构造！
```

### noexcept 的两种形式

```cpp
// 1. 直接标注
void process() noexcept { /* 保证不抛 */ }

// 2. 条件 noexcept——根据表达式是否 noexcept 决定
template<class T>
void swap(T& a, T& b) noexcept(noexcept(T(std::declval<T>()))) {
    T tmp = std::move(a);
    a = std::move(b);
    b = std::move(tmp);
}
// 如果 T 的移动构造是 noexcept，swap 也是 noexcept
```

### 标错 noexcept 的后果

```cpp
void risky() noexcept {
    throw std::runtime_error("oops");  // 抛了异常！
}
// 调用 risky() 时会 std::terminate()——程序直接挂
// noexcept 是承诺，不是建议
```

**标错 `noexcept` 但抛异常会 `std::terminate`**——所以 `noexcept` 是承诺，不是建议。

---

## 常见错误（新手踩坑）

**错误 1：移动构造忘了标 noexcept**
```cpp
class Order {
public:
    Order(Order&& o);  // 没标 noexcept
};
std::vector<Order> v;
v.push_back(Order{...});  // 扩容时退回拷贝！哪怕你有移动构造
```
**修正：** `Order(Order&& o) noexcept;`

**错误 2：标了 noexcept 但函数内部调了可能抛异常的函数**
```cpp
void process() noexcept {
    auto ptr = std::make_shared<Widget>();  // make_shared 可能抛 bad_alloc
    // 如果真抛了 → std::terminate
}
```
**修正：** 不确定就别标 `noexcept`。或者 catch 所有异常：
```cpp
void process() noexcept {
    try { /* 可能抛异常的代码 */ }
    catch (...) { /* 吞掉 */ }
}
```

**错误 3：析构函数抛异常**
```cpp
class Bad {
public:
    ~Bad() { throw std::runtime_error("oops"); }  // 析构抛异常！
};
// 析构函数默认是 noexcept（C++11 起）
// 如果真的抛了 → std::terminate
```
**修正：** 析构函数永远不抛异常。如果析构里调了可能抛的代码，用 try-catch 包住。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 错误处理 | 返回值（`return -1`） | 异常（`throw`）或返回值 | C++ 有异常机制 |
| 不抛保证 | 不适用 | `noexcept` | 编译器优化 + 接口契约 |
| 析构抛异常 | 不适用 | 禁止（`terminate`） | 异常安全保证 |
| 影响容器 | 不适用 | `noexcept` 移动 → 走移动 | 编译期分派 |

**一句话总结：** C 程序员记住——C++ 有异常，`noexcept` 是"我保证不抛异常"的契约。最该标的是移动构造/赋值和 swap——它们直接影响 STL 容器性能。

---

## HFT 关联

- **扩容延迟尖峰**：`vector<Order> push_back` 扩容时，`Order` 的移动构造必须 `noexcept` 才走移动语义——否则扩容退回拷贝，订单簿重建延迟尖峰。这是 HFT C++ 性能的**隐形开关**。
- **move 语义 vs copy**：HFT 热路径中大量使用 `std::move` 转移数据所有权，`noexcept` 保证移动操作不抛异常，避免异常处理开销。
- **编译器优化**：`noexcept` 让编译器省略异常处理代码（unwind table），减少代码体积和分支预测压力。

---

## 自测题

1. STL 容器 `push_back` 扩容时如何决定用移动还是拷贝？`noexcept` 在其中起什么作用？
2. 标了 `noexcept` 但函数抛了异常会发生什么？
3. 最应该标 `noexcept` 的四个函数是什么？
4. 为什么说"标错 noexcept 比不标更危险"？
5. 下面代码有什么问题？
```cpp
class Widget {
public:
    Widget(Widget&& o) { data_ = o.data_; o.data_ = nullptr; }
};
std::vector<Widget> v;
v.push_back(std::move(w));
```

---

## 参考与延伸

- 下一节：[Item 15 constexpr](item15-constexpr.md)
- 回到：[第 3 章 移步现代 C++](README.md)
