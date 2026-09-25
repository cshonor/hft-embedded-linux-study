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

<details>
<summary>参考答案</summary>

1. `decltype(x)` 是 `int`，`decltype((x))` 是 `int&`。`decltype` 有两条分支：如果参数是一个**未加括号的标识符/类成员访问**，就返回该变量的声明类型；否则把它当作表达式，按表达式的值类别处理——`(x)` 是一个左值表达式，左值表达式的 `decltype` 结果就是 `T&`。所以多一对括号就把语义从"查名字的类型"变成了"查表达式的类型"。

2. `auto` 用模板推导规则：会剥掉引用和顶层 const，且对 braced-init-list 推成 `initializer_list`。`decltype(auto)` 用 `decltype` 的规则（C++14 起）：完全按初始化表达式的形式推导，保留引用性和 cv 限定。例如 `int x; auto a = x;` 得 `int`，而 `decltype(auto) b = x;` 也得 `int`；但 `decltype(auto) c = (x);` 得 `int&`。

3. 因为 `auto` 返回类型会按模板规则剥掉引用和顶层 const，把本该返回的引用变成一份拷贝——对返回容器元素（`c[i]`）或成员引用的转发函数，这会多一次拷贝、甚至改变语义（调用方拿不到原对象的修改）。`decltype(auto)` 精确保留返回表达式的类型与值类别，配合 `std::forward<T>` 才能真正做到完美转发。注意风险：`return (x);` 这种加括号写法会返回局部变量的引用，导致悬垂。

4. `const int&`。`cx` 是一个未加括号的变量名，`decltype` 直接返回它的声明类型，引用和 const 都原样保留。

</details>

---

## 参考与延伸

- 下一节：[Item 4 查看推导结果](item04-viewing-deduced-types.md)
- 回到：[第 1 章 类型推导](README.md)
