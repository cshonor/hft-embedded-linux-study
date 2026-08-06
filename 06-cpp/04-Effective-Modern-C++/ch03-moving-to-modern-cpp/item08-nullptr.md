# Item 8：优先 nullptr 而非 0 和 NULL

> 第 3 章 移步现代 C++ · Item 8 · 上一节：[Item 7 () vs {}](item07-parens-vs-braces.md)

## 这节讲什么

`0` 和 `NULL` 都是整型字面量，在指针与整型重载时会误选整型。`nullptr` 的类型是 `std::nullptr_t`，能隐式转任意指针但不能转整型——彻底消除歧义。

---

## 核心示例

```cpp
void f(int);
void f(Widget*);
f(0);          // 调 f(int)！不是 f(Widget*)
f(NULL);       // 仍可能调 f(int)（NULL 的类型依赖实现）
f(nullptr);    // 调 f(Widget*)，正确
```

`nullptr` 的优势：
1. **不会误选整型重载**
2. **模板推导正确**：`template<class T> void g(T x); g(nullptr);` 推出 `T = nullptr_t`，而非 `int`
3. **代码意图清晰**：`nullptr` 一眼看出是空指针，`0` 需要看上下文

---

## 新手要点（和 C 的区别）

- **C 用 `NULL` 或 `0`**：C 里 `NULL` 通常定义为 `((void*)0)`，指针和整数区分较清晰。C++ 里 `NULL` 定义为 `0` 或 `0L`（整型），导致重载歧义。
- **一律用 nullptr**：C++ 代码里空指针全写 `nullptr`，不写 `0` 或 `NULL`。这是零成本的改进。
- **`nullptr_t` 类型**：`nullptr` 不是指针，它有自己的类型 `std::nullptr_t`，可以隐式转任意指针类型。

---

## HFT 关联

- **模板推导正确性**：策略工厂 `template<class T> T* create(Args...)` 里传 `nullptr` 做默认参数时，类型推导正确；传 `0` 会被推成 `int`。

---

## 自测题

1. `f(0)` 在 `void f(int); void f(Widget*);` 重载集里调用哪个？`f(nullptr)` 呢？
2. `nullptr` 的类型是什么？它能隐式转成 `int` 吗？
3. 为什么 `NULL` 在 C++ 里不安全？
4. `template<class T> void g(T x); g(nullptr);` 推出 `T` 是什么？

---

## 参考与延伸

- 下一节：[Item 9 using 别名](item09-using.md)
- 回到：[第 3 章 移步现代 C++](README.md)
