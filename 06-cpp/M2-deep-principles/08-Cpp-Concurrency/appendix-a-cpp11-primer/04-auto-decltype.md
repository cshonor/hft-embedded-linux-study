# A.4 auto 与 decltype

> 附录 A · 上一节：[A.3 智能指针](03-smart-ptr.md) · 下一节：[A.5 可变参数模板](05-variadic.md)

## 这节讲什么

`auto` 让编译器推导变量类型，`decltype` 查询表达式的类型。本节讲 `auto` 的推导规则、`decltype` 的用法、以及 `auto&&`（万能引用）在泛型编程中的价值。

---

## 核心规则（代码+表格）

### `auto` 基础

```cpp
auto x = 42;        // int
auto y = 3.14;      // double
auto s = "hello";   // const char*
auto v = std::vector<int>{1, 2, 3};  // std::vector<int>
auto p = std::make_shared<Buffer>();  // std::shared_ptr<Buffer>

// auto 会丢弃引用和 const
int& ref = x;
const int& cref = x;
auto a = ref;    // int（不是 int&）
auto b = cref;   // int（不是 const int&）

// 要保留引用/const：手动加
auto& c = ref;       // int&
const auto& d = x;   // const int&
```

### `auto` 在迭代器中的价值

```cpp
std::map<std::string, std::vector<int>> data;

// 不用 auto：类型冗长
for (std::map<std::string, std::vector<int>>::iterator it = data.begin();
     it != data.end(); ++it) { ... }

// 用 auto：简洁
for (auto it = data.begin(); it != data.end(); ++it) { ... }
for (auto& [key, val] : data) { ... }  // C++17 结构化绑定
```

### `decltype` 查询类型

```cpp
int x = 42;
decltype(x) a;        // int
decltype(x + 0) b;    // int
decltype((x)) c = x;  // int&（注意双括号！）

// decltype 的规则：
// decltype(变量名) → 变量的声明类型
// decltype((变量名)) → 变量的引用类型（加括号变左值表达式）
// decltype(表达式) → 表达式的类型

// 用于函数返回类型（C++11）
template <typename T, typename U>
auto add(T t, U u) -> decltype(t + u) {
    return t + u;
}

// C++14 简化：auto 返回类型推导
template <typename T, typename U>
auto add(T t, U u) {
    return t + u;  // 自动推导
}
```

### `decltype((x))` 的陷阱

```cpp
int x = 42;
decltype(x) a = 10;     // int
decltype((x)) b = x;    // int&（不是 int！）
// 加括号 = 左值表达式 → 推导出引用
// 不加括号 = 变量名 → 推导出声明类型
```

### `auto&&` 万能引用

```cpp
// auto&& 在模板中是万能引用（forwarding reference）
// 可以绑定左值或右值

template <typename T>
void wrapper(T&& arg) {
    // T&& 是万能引用（T 是模板参数）
    target(std::forward<T>(arg));  // 完美转发
}

// auto&& 同理
auto&& ref = get_something();
// 如果 get_something() 返回左值 → ref 是左值引用
// 如果返回右值 → ref 是右值引用
```

---

## 新手要点（和 C 的区别）

- **C 没有 `auto` 类型推导**：C 程序员要手写完整类型名——`std::map<std::string, std::vector<int>>::iterator`。C++ 的 `auto` 大幅减少冗余代码。C99 的 `auto` 只是"自动存储期"声明（几乎没人用），C++11 重载了 `auto` 的语义。
- **`decltype` 是 C 程序员陌生的新工具**：C 没有查询类型的机制。C++ 的 `decltype` 让模板编程成为可能——在泛型代码中查询和推导类型。
- **`decltype((x))` vs `decltype(x)` 是 C++ 的著名陷阱**：加不加括号语义不同——这是 C 程序员转型 C++ 时最容易踩的坑之一。记住：双括号 = 引用。
- **`auto&&` 万能引用**：C 程序员可能觉得 `&&` 就是"右值引用"——但在模板和 `auto&&` 中它是"万能引用"，能绑定左值。这是 C++ 的高级特性，配合 `std::forward` 实现完美转发。

---

## HFT 关联

- **`auto` 让 HFT 代码更易读**：HFT 涉大量模板类型（`std::atomic<T>`、`std::chrono::time_point`），`auto` 让代码聚焦逻辑而非类型噪音。
- **`auto` 不影响性能**：`auto` 是编译期推导，运行时和显式类型完全相同——HFT 可以放心用。
- **`decltype` 用于泛型 HFT 库**：HFT 的通用工具（如 SPSC 队列模板）用 `decltype` 推导返回类型——让接口自适应不同元素类型。
- **`auto&&` 在 HFT 转发中**：HFT 的任务包装器用 `auto&&` + `std::forward` 完美转发参数——避免不必要的拷贝。

---

## 自测题

1. `auto` 会丢弃引用和 `const`，如何保留？
2. `decltype(x)` 和 `decltype((x))` 有什么区别？
3. C++14 的 `auto` 返回类型推导和 C++11 的 `-> decltype(...)` 有什么区别？
4. `auto&&` 为什么叫"万能引用"？它能绑定什么？
5. `auto` 会影响运行时性能吗？

---

## 参考与延伸

- 下一节：[A.5 可变参数模板](05-variadic.md)
- 上一节：[A.3 智能指针](03-smart-ptr.md)
- 回到：[附录 A](README.md)
