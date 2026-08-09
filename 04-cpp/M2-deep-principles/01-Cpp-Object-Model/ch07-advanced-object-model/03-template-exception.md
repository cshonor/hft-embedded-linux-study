# 7.3 模板 + 异常的交互

> 第 7 章 · 上一节：[7.2 异常处理实现](02-exception-impl.md) · 回到：[本书完结](../README.md)

## 这节讲什么

模板函数的异常规范影响实例化行为。C++17 起 `noexcept` 是类型系统的一部分，影响函数指针类型。`-fno-exceptions` 对模板的影响。

---

## 为什么要学这个（先建立直觉）

C 程序员不关心异常规范——C 没有异常。但 C++ 中 `noexcept` 从"建议"变成了"类型系统的一部分"：

```cpp
// C++17 前：noexcept 不是类型的一部分
void f() noexcept;
void g();
void (*p)() = f;  // OK
void (*p)() = g;  // OK（noexcept 可隐式退化为非 noexcept）

// C++17 起：noexcept 是类型的一部分
void (*p)() noexcept = f;  // OK
void (*p)() noexcept = g;  // 编译错误！g 不是 noexcept
```

这影响模板实例化和函数指针赋值——新手容易踩坑。

---

## 核心要点详解

### noexcept 作为类型修饰符（C++17）

```cpp
void may_throw();
void no_throw() noexcept;

// 函数指针类型不同
void (*p1)() = may_throw;         // OK
void (*p2)() noexcept = no_throw; // OK
void (*p3)() = no_throw;          // OK（noexcept 可退化为非 noexcept）
void (*p4)() noexcept = may_throw;// 编译错误！非 noexcept 不能赋给 noexcept
```

### 模板中的 noexcept

```cpp
template<class F>
void caller(F f) {
    static_assert(noexcept(f()), "f should be noexcept");
    f();
}

void safe() noexcept { }
void risky() { }

caller(safe);   // OK：safe 是 noexcept
caller(risky);  // 编译错误：risky 不是 noexcept
```

### noexcept(expr) 检查

```cpp
template<class T>
void process(T& obj) {
    if constexpr (noexcept(obj.process())) {
        // T::process() 是 noexcept → 可以安全调用
        obj.process();
    } else {
        // T::process() 可能抛异常 → 需要异常处理
        try { obj.process(); } catch (...) { /* handle */ }
    }
}
```

### conditional noexcept

```cpp
template<class T>
void move_or_copy(T& src, T& dst) noexcept(noexcept(T(std::move(src)))) {
    dst = std::move(src);
}
// noexcept(noexcept(...)) 让函数的 noexcept 性取决于 T 的移动构造是否 noexcept
```

---

## 常见错误（新手踩坑）

### 错误 1：noexcept 函数指针赋值

```cpp
void f() noexcept;
void g();
void (*p)() noexcept = g;  // 编译错误！
// g 不是 noexcept，不能赋给 noexcept 函数指针
// 修正：void (*p)() = g;
```

### 错误 2：在 noexcept 函数里调非 noexcept 函数

```cpp
void risky() { throw std::runtime_error("oops"); }
void safe() noexcept {
    risky();  // 如果 risky 抛异常 → std::terminate！
    // noexcept 承诺不抛 → 抛了就 terminate
}
```

### 错误 3：-fno-exceptions 下模板行为

```cpp
// -fno-exceptions 下：
// throw 语句被移除（编译为 unreachable）
// noexcept 变得无意义（没有异常可抛）
// STL 的某些行为变化（如 std::vector 的强异常保证弱化）
```

---

## 和 C 的区别

| 特性 | C | C++ |
|------|---|-----|
| 异常规范 | N/A | `noexcept`（C++11 替代 `throw()`） |
| 类型系统 | N/A | C++17 起 noexcept 是类型的一部分 |
| 模板实例化 | N/A | noexcept 影响模板行为 |
| 可关闭 | N/A | `-fno-exceptions` |

---

## HFT 关联

1. **`noexcept` 标注热路径**：HFT 热路径函数标 `noexcept`——让编译器省略异常处理代码 + 文档化"绝不抛异常"。
2. **移动构造 noexcept**：`T(T&&) noexcept` 让 vector 在扩容时用移动而非拷贝——性能关键。
3. **`-fno-exceptions` + 模板**：关异常时模板的 `throw` 被移除，但 STL 部分行为变化（如 vector 扩容策略）。

---

## 代码自测

### Q1: noexcept 类型

```cpp
void f() noexcept;
void g();
// 以下哪些能编译？
void (*p1)() = f;             // A
void (*p2)() noexcept = f;    // B
void (*p3)() = g;             // C
void (*p4)() noexcept = g;    // D
```

<details>
<summary>答案与复习指引</summary>

A、B、C 可以编译。D 编译错误——`g` 不是 `noexcept`，不能赋给 `noexcept` 函数指针。规则：`noexcept` 可退化为非 `noexcept`（B→A），但反之不行（D）。C++17 起 `noexcept` 是类型的一部分。

**复习：** → [7.3 模板+异常交互](./03-template-exception.md)
</details>

### Q2: noexcept 检查

```cpp
template<class T>
void safe_call(T&& f) {
    static_assert(noexcept(f()), "f must be noexcept");
    f();
}
void a() noexcept {}
void b() {}
safe_call(a);  // 编译吗？
safe_call(b);  // 编译吗？
```

<details>
<summary>答案与复习指引</summary>

`safe_call(a)` 可以编译（`a` 是 noexcept）。`safe_call(b)` 编译错误（`b` 不是 noexcept，`static_assert` 失败）。`noexcept(expr)` 在编译期检查表达式是否声明为 noexcept。

**复习：** → [7.3 模板+异常交互](./03-template-exception.md)
</details>

### Q3: noexcept 中抛异常

```cpp
void risky() { throw std::runtime_error("oops"); }
void safe() noexcept {
    risky();  // 会发生什么？
}
```

<details>
<summary>答案与复习指引</summary>

`std::terminate` → 程序崩溃。`safe()` 声明 `noexcept`（承诺不抛异常），但调用的 `risky()` 抛了异常。异常传播出 `safe()` 时，因为 `safe` 是 noexcept → 调 `std::terminate`。**不要在 noexcept 函数里调可能抛异常的函数。**

**复习：** → [7.3 模板+异常交互](./03-template-exception.md)
</details>

### Q4: vector 扩容

```cpp
class Widget {
public:
    Widget() {}
    Widget(const Widget&) { /* 拷贝 */ }
    Widget(Widget&&) noexcept { /* 移动 */ }  // noexcept 移动构造
};
std::vector<Widget> v;
v.push_back(Widget());  // vector 扩容时用移动还是拷贝？
```

<details>
<summary>答案与复习指引</summary>

用**移动**。`Widget(Widget&&) noexcept` 声明了 noexcept 移动构造——vector 在扩容时检查移动构造是否 noexcept，如果是则用移动（快），否则用拷贝（安全）。这就是为什么移动构造应该标 noexcept——它直接影响 vector 的性能。

**复习：** → [7.3 模板+异常交互](./03-template-exception.md)
</details>

---

## 参考与延伸

- 本书完结，回到：[《深度探索 C++ 对象模型》索引](../README.md)
