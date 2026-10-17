# 泛型编程改进

## Concepts 替代 SFINAE

```cpp
// 详见第 3-5 章
// C++20 Concepts 是泛型编程的核心改进

// C++17
template <typename T,
          std::enable_if_t<std::is_integral_v<T>, int> = 0>
void process(T x) { /* ... */ }

// C++20
void process(std::integral auto x) { /* ... */ }
```

## 简化模板语法

```cpp
// C++20：auto 模板参数（见第 2 章）
void foo(auto x) { /* ... */ }

// C++20：模板 lambda（见第 17 章）
auto f = []<typename T>(T x) { /* ... */ };
```

## 更好的类型推导

```cpp
// C++20：CTAD 改进
std::vector v = {1, 2, 3};  // vector<int>（C++17）
std::pair p{1, 2.0};        // pair<int, double>（C++17）

// C++20：聚合体 CTAD
struct Point { int x, y; };
Point p{1, 2};  // C++20：聚合体 CTAD

// C++20：别名模板 CTAD
template <typename T>
using Vec = std::vector<T>;
Vec v = {1, 2, 3};  // C++20：推导 Vec<int>
```

## noexcept 类型化

```cpp
// C++20：noexcept 是函数类型的一部分
void (*fp1)() noexcept = []() noexcept {};
// void (*fp2)() = fp1;  // C++17 可以，C++20 严格

// 影响：函数指针匹配
void maybe_throw();
void no_throw() noexcept;

void (*fp)() = no_throw;  // ✅ noexcept 转 non-noexcept
// void (*fp_n)() noexcept = maybe_throw;  // ❌ non-noexcept 不能转 noexcept
```

## 三路比较默认生成

```cpp
// 详见第 1 章
struct Point {
    int x, y;
    auto operator<=>(const Point&) const = default;
};
// 一步生成全部比较运算符
```

## 自测题

1. C++20 Concepts 如何替代 SFINAE？
2. 聚合体 CTAD 是什么？
3. `noexcept` 在 C++20 中作为类型一部分有什么影响？
4. C++20 的别名模板 CTAD 怎么用？
5. C++20 泛型编程的三大改进是什么？

<details>
<summary>参考答案</summary>

1. 替代方式体现在三处：
   1. **约束写在签名上**：`template <std::integral T>` 或 `template <typename T> requires ...`，不必再用 `enable_if_t<...>` 塞进返回类型/额外模板参数/参数类型。
   2. **失败即清晰报错**：约束不满足时在**调用点**报"哪个 concept 的哪一项不满足"，而不是进入模板内部报几十行实例化错误；也不必再写 `void_t` 探测惯用法。
   3. **约束参与重载消解**：更严格的约束通过 **subsumption** 自动胜出，不必像 `enable_if` 那样手工把条件写成互斥（写错就二义）。
另外函数体内部的分派由 `if constexpr` 承担（C++17），两者合起来就取代了 SFINAE + tag dispatch 的整套技巧。
2. **聚合体 CTAD** 指 C++20 起**聚合类模板**也能从初始化器推导模板参数（通过新增的"聚合推导候选"）：
```cpp
template <class T> struct Point { T x, y; };
Point p{1, 2};       // C++20：推导为 Point<int>
Point q{.x = 1, .y = 2};   // 配合指定初始化也可以
```
注意：笔记里写的 `struct Point { int x, y; };` 是**非模板**聚合体，`Point p{1,2};` 只是普通的聚合初始化，C++17 及更早也一直合法——真正的新特性是上面这种**类模板**的聚合推导（不必再手写 `make_point` 推导指引）。
3. 影响是"异常规范成了类型匹配的一部分"：
   - `void()` 与 `void() noexcept` 是**两种不同的类型**；函数指针、函数引用、成员指针、`std::function`、模板实参的类型都必须匹配。
   - 转换是**单向**的：指向"不抛"函数的指针可以**隐式转换**为指向"可能抛"函数的指针；反过来**不允许**（否则等于谎报不抛异常）。
```cpp
void (*fp)() = no_throw;                 // ✅ noexcept → 可能抛
// void (*fn)() noexcept = maybe_throw;  // ❌ 反向不允许
```
   - 不能仅靠异常规范重载函数；虚函数覆写者的异常规范不能比基类更严格。
（备注：`noexcept` 进入函数类型是 **C++17**（P0012R1）的变化，并非 C++20 才引入；C++20 相关的是 `throw()` 被移除等收尾工作。）
4. C++20 起别名模板也参与 CTAD，可以直接用别名名字推导：
```cpp
template <typename T> using Vec = std::vector<T>;
Vec v = {1, 2, 3};        // 推导为 Vec<int>，即 std::vector<int>

template <typename K, typename V> using Map = std::map<K, V>;
Map m{{1, "a"}, {2, "b"}};   // 推导为 Map<int, const char*>
```
推导过程是：先按别名模板的推导指引/实参推导出别名的模板实参，再换算回原模板。这让 `Vec`、`Map` 这类"简写别名"用起来和真实容器一样自然，不必显式写 `<int>`。
5. 可以概括为三条主线：
   1. **Concepts 与约束（constraints）**：`concept` 定义、`requires` 子句与 `requires` 表达式、约束 `auto`、以及基于 **subsumption** 的重载消解——取代 SFINAE/`enable_if`，错误信息与重载分派都质变。
   2. **模板书写简化**：简写函数模板（`void f(auto x)`）、模板 lambda（`[]<typename T>`），泛型代码不必再写冗长的 `template<...>` 头。
   3. **CTAD 增强**：聚合体 CTAD 与别名模板 CTAD，让 `Point p{1,2}`、`Vec v = {1,2,3}` 这类写法成立。
配套的还有 C++17 已引入、C++20 继续发扬的 `if constexpr`（函数体内编译期分派）和 `<=>`（一行生成全部比较运算符）。

</details>
