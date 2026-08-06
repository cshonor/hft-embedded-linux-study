# Item 3：理解 decltype

> 第 1 章 类型推导 · Item 3 · 上一节：[Item 2 auto 推导](item02-auto-type-deduction.md)

## 这节讲什么

`decltype` 是"给出名字或表达式，告诉我它的类型"。它和 `auto` 的推导规则不同——`decltype` 大部分时候是**原样保留**类型，但有一个"加括号变引用"的陷阱。`decltype` 最常见的用途是 `auto` 返回类型推导（`auto f() -> decltype(expr)`）。

---

## 核心规则

```cpp
int x = 42;           // decltype(x) = int
const int& cx = x;    // decltype(cx) = const int&
```

`decltype` 基本规则：**变量名 → 返回声明类型；表达式 → 返回该表达式类型的引用（如果是左值）**。

### 加括号陷阱

```cpp
int x = 42;
decltype(x)   a;     // int（变量名）
decltype((x)) b;     // int&！（带括号的表达式是左值 → 引用）
```

`x` 是变量名 → `int`；`(x)` 是表达式（左值）→ `int&`。多一对括号，类型从值变引用——这是 `decltype` 最坑的地方。

### decltype(auto)

C++14 起，`decltype(auto)` 用 `decltype` 规则推导（而非 `auto` 规则）：

```cpp
template<class T>
decltype(auto) wrapper(T&& x) { return std::forward<T>(x); }
// 返回类型保持引用性，不会丢失 const/& 
```

---

## 新手要点（和 C 的区别）

- **C 没有 decltype**（C11 有 `_Generic` 但能力远弱于 `decltype`）。`decltype` 是 C++ 独有的编译期类型查询。
- **`decltype(x)` vs `decltype((x))`**：C 程序员完全不习惯"加括号变引用"。记住口诀：**名字不加括号 = 值类型；名字加括号 = 引用类型**。
- **什么时候用 decltype**：写模板转发函数、需要精确保持返回类型时。普通代码用 `auto` 就够了。

---

## HFT 关联

- **泛型转发**：策略引擎的 `template<class F> decltype(auto) on_tick(F&& f)` 保证回调返回类型不被意外截断（`auto` 会丢引用）。
- **`decltype(auto)` 与 `forward`**：完美转发的包装器用 `decltype(auto)` + `std::forward` 才能正确保持左右值性。

---

## 自测题

1. `int x = 42;` `decltype(x)` 和 `decltype((x))` 分别是什么？为什么不同？
2. `decltype(auto)` 和 `auto` 在推导规则上有何不同？
3. 为什么泛型转发函数推荐用 `decltype(auto)` 而非 `auto` 做返回类型？
4. `const int& cx = x;` `decltype(cx)` 是什么？

---

## 参考与延伸

- 下一节：[Item 4 查看推导结果](item04-viewing-deduced-types.md)
- 回到：[第 1 章 类型推导](README.md)
