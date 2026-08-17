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
