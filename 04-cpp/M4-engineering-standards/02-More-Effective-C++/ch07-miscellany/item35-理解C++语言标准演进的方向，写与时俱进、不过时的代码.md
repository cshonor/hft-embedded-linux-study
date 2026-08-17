# 条款 35：理解 C++ 语言标准演进的方向，写与时俱进、不过时的代码

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
#if __cplusplus >= 202002L
    // C++20 特性
#else
    // 兼容旧标准
#endif
```

---

## 代码自测

**题目 1：** C++ 标准从 C++98 到 C++20 有哪些重大变化？简要列举每代的关键特性。

<details>
<summary>参考答案</summary>

- C++98/03：模板、STL、异常、RAII
- C++11：auto、lambda、move 语义、智能指针、range-for、constexpr、variadic templates、thread
- C++14：泛型 lambda、返回类型推导、`make_unique`
- C++17：structured bindings、`optional`/`variant`/`any`、`string_view`、`filesystem`、`if constexpr`
- C++20：Concepts、Ranges、Coroutines、Modules、`<=>` 飞船运算符
学习方向：现代 C++ 更注重类型安全、零开销抽象、编译期计算。写代码时应与时俱进，用新特性替代旧惯用法（如用 `auto` 替代冗长类型名、用 lambda 替代函数对象）。

</details>
