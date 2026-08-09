# 第 2 章 函数参数的占位符类型

**Placeholder Types for Function Parameters**

## 本章讲什么

C++20 允许函数参数用 `auto` 作为占位符类型（像泛型 lambda 那样），让普通函数也能"简写模板"。

## 要点

### 基本用法

```cpp
// C++20：auto 参数
void print(const auto& x) {
    std::cout << x;
}
// 等价于
template <typename T>
void print(const T& x) {
    std::cout << x;
}

// 多个 auto
auto add(auto a, auto b) { return a + b; }
// 等价于 template <typename T, typename U> auto add(T a, U b)
```

### 与模板的关系

`auto` 参数的函数**本质是模板**——编译器为每个不同类型实例化一份。但写法更简洁。

```cpp
// 这两个完全等价
void foo(auto x);
template <typename T> void foo(T x);
```

### 显式约束（配合 Concepts）

```cpp
// C++20：auto 参数 + Concepts 约束
void process(std::integral auto x) {       // 只接受整数
    std::cout << x;
}

void sort(std::ranges::random_access_range auto& r) {  // 只接受随机访问范围
    std::ranges::sort(r);
}
```

`Concept auto` 形式让"简写模板 + 类型约束"结合，比传统 `enable_if` 简洁十倍。

### 与 lambda 的统一

C++14 起 lambda 就有 `auto` 参数（泛型 lambda）。C++20 让普通函数也有，风格统一：

```cpp
auto lambda = [](auto x) { return x; };   // C++14
void func(auto x) { /* ... */ }           // C++20
```

### 限制

- `auto` 参数的函数是模板，不能放在 .cpp 单独编译（要 header-only 或显式实例化）。
- 不能取函数地址当普通函数指针（要模板实例化）。
- 虚函数不能用 `auto` 参数（虚函数不能是模板）。

## HFT 关联

- **泛型打印/日志函数**：`void log(auto&& x)` 简化日志库，接受任意类型。
- **Concept 约束的简写**：`void process(std::integral auto x)` 比 `template<Integral T> void process(T x)` 简洁。
- **header-only 工具**：HFT 工具库常 header-only，`auto` 参数的函数自然适配。
- **与 Ranges 配合**：`std::ranges::sort(r)` 这类算法接受 `range auto` 参数，自定义函数也用同样风格。
- **限制注意**：热路径函数若需放在 .cpp 编译（隐藏实现），不能用 `auto` 参数——用显式模板或 Concept。

## 自测题

1. C++20 的 `auto` 参数函数和模板是什么关系？
2. `void foo(auto x)` 等价于什么模板声明？
3. `Concept auto` 形式如何同时简写和约束？
4. `auto` 参数函数有什么限制？（.cpp 编译、函数指针、虚函数）
5. HFT 泛型工具函数如何用 `auto` 参数 + Concept？

## 代码自测

### Q1: auto 返回类型改进
```cpp
// C++14: auto 返回类型需要编译器推导
auto f(int x) { return x * 2; }  // 推导为 int

// C++20: auto 可用于函数签名（概念约束）
auto g(int x) -> int { return x; }  // 尾置返回类型仍可用

// C++20: decltype(auto) 保留引用
decltype(auto) h(int& x) { return x; }  // 返回 int&（不是 int）
```
> `decltype(auto)` 和 `auto` 在返回类型上的区别？

<details>
<summary>答案与复习指引</summary>

- `auto`：按值返回（退化引用、数组→指针）
- `decltype(auto)`：保留表达式的精确类型（引用、const 修饰）

```cpp
int& ref = some_int;
auto a = ref;              // a 是 int（拷贝）
decltype(auto) b = ref;    // b 是 int&（引用）
```

**用途**：转发函数、完美转发包装器需要保留引用和 const。标准库的 `std::forward`、`std::move` 内部用 `decltype(auto)`。

**注意**：`decltype(auto)` 不能用于 lambda 参数（C++20 concepts 可以），且每个 return 语句必须类型一致。

**复习：** → [占位类型](./README.md)
</details>
