# 条款 14：理解异常规范 throw() 的运行时机制、局限性与坑

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
void old_api() throw();       // C++17 前异常规范，已不推荐
void modern_api() noexcept;   // 现代写法
```

---

## 代码自测

**题目 1：** C++11 的 `noexcept` 和 C++03 的 `throw()` 有什么区别？
```cpp
void f() throw();         // C++03
void g() noexcept;        // C++11
void h() noexcept(true);  // 等价
```

<details>
<summary>参考答案</summary>

`throw()`（C++03，C++17 已移除）：如果函数抛异常，会意外调用 `std::unexpected`，默认行为是 `std::terminate`——但运行时开销较大（需要跟踪异常规范）。
`noexcept`（C++11）：如果函数抛异常，直接 `std::terminate`——编译器可以做更多优化（不需要展开栈）。`noexcept` 是性能契约：承诺不抛异常，编译器据此优化。标准库容器的 `move` 操作如果声明 `noexcept`，在扩容时会用 move 而非 copy。

</details>
