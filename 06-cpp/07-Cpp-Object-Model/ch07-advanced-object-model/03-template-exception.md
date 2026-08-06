# 7.3 模板 + 异常的交互

> 第 7 章 · 上一节：[7.2 异常处理实现](02-exception-impl.md) · 回到：[本书完结](../README.md)

## 这节讲什么

模板函数的异常规范影响实例化行为。`noexcept` 是类型系统的一部分（C++17 起类型相关），影响函数指针类型。

---

## 核心要点

- 模板实例化时，异常规范随类型参数变化
- C++17 起 `noexcept` 是函数类型的一部分——`void(*)()` 和 `void(*)() noexcept` 是不同类型
- `noexcept` 影响函数指针赋值：非 `noexcept` 函数指针不能赋给 `noexcept` 函数指针（反之可以）

```cpp
void f() noexcept;
void g();

void (*p1)() noexcept = f;  // OK
void (*p2)() noexcept = g;  // 编译错误！g 不是 noexcept
void (*p3)() = g;           // OK
void (*p4)() = f;           // OK（noexcept 可退化为非 noexcept）
```

---

## 新手要点

- **C++17 的变化**：C++17 前 `noexcept` 不是类型的一部分；C++17 起是。这影响了函数指针的类型兼容性。
- **实际影响**：主要在泛型编程和函数指针类型推导时遇到。新手了解概念即可。

---

## HFT 关联

- **`-fno-exceptions` + 模板**：关异常时模板的 `throw` 被移除，但 STL 部分模板行为变化。

---

## 自测题

1. C++17 起 `noexcept` 有什么类型系统变化？
2. 非 `noexcept` 函数指针能赋给 `noexcept` 函数指针吗？反过来呢？
3. `-fno-exceptions` 对模板有什么影响？

---

## 参考与延伸

- 本书完结，回到：[《深度探索 C++ 对象模型》索引](../README.md)
