# Item 2：理解 auto 类型推导

> 第 1 章 类型推导 · Item 2 · 上一节：[Item 1 模板类型推导](item01-template-type-deduction.md)

## 这节讲什么

`auto` 的推导规则**几乎和模板推导完全一致**，但有一个关键例外：`auto` 推导大括号初始化 `{}` 时会推导出 `std::initializer_list<T>`，而模板推导不会。这个例外是 `auto` 最容易踩的坑。

---

## 核心规则

`auto` 推导的三种形态（对应模板推导的三种 ParamType）：

| 形态 | 写法 | 推导规则 |
|------|------|----------|
| 指针/引用 | `auto& x = expr` | 同模板规则 1：忽略右值性，保留 const |
| 通用引用 | `auto&& x = expr` | 同模板规则 2：左值→`auto&`，右值→`auto&&` |
| 按值 | `auto x = expr` | 同模板规则 3：忽略 const 和引用 |

```cpp
auto x = 27;          // int（按值）
const auto cx = x;    // const int
const auto& rx = x;   // const int&

auto&& uref1 = x;     // int&（左值→左值引用）
auto&& uref2 = 27;    // int&&（右值→右值引用）
```

### 大括号例外（auto 独有）

```cpp
auto x = {11};        // std::initializer_list<int>！
auto y{11};           // C++14 前：initializer_list<int>
                      // C++17 起：int（直接初始化修正）
```

模板推导**不接受** braced-init-list：
```cpp
template<class T> void f(T x);
f({11});              // 编译失败！无法推导 T
```

但 `auto` **接受** braced-init-list 并推导出 `initializer_list`——这是 `auto` 与模板推导唯一的本质差异。

---

## 新手要点（和 C 的区别）

- **C 没有 auto 推导**（C 的 `auto` 只是默认存储期，C++11 起完全废弃了这个旧含义）。C 里变量类型必须手写，C++ 用 `auto` 让编译器帮你推。
- **`auto x = {1, 2, 3}` 的陷阱**：C 程序员直觉认为 `x` 是 `int` 或数组，实际是 `initializer_list<int>`。用 `auto` 接大括号要特别小心。
- **什么时候用 auto**：类型名长（迭代器、lambda）、怕写错类型、重构时类型会变。简单类型（`int`、`double`）手写更清晰。

---

## HFT 关联

- **避免隐式窄化**：`auto sz = vec.size()` 推出 `size_t`，不会截断；`unsigned sz = vec.size()` 在大容器时会截断。HFT 里订单序号用 `auto` 最安全。
- **`auto` 与迭代器**：`for (auto it = m.begin(); it != m.end(); ++it)` 比手写 `std::unordered_map<std::string, Order, HashFn>::const_iterator` 简洁得多。

---

## 自测题

1. `auto x = 27;` `auto& rx = x;` `auto&& uref = x;` 各推出什么类型？
2. `auto x = {1, 2, 3};` 推出什么类型？为什么模板推导不能接受 `{1,2,3}` 而 `auto` 可以？
3. C++17 的 `auto x{11};` 和 C++14 的 `auto x{11};` 有何不同？
4. 为什么说 `auto` 推导和模板推导"几乎一致"？唯一的例外是什么？

<details>
<summary>参考答案</summary>

1. `auto x = 27;` → `x` 是 `int`（按值形态，忽略 const 与引用）。`auto& rx = x;` → `rx` 是 `int&`（`auto` 推为 `int`，加上声明的 `&`）。`auto&& uref = x;` → `uref` 是 `int&`（`x` 是左值，`auto` 被推为 `int&`，`int& &&` 引用折叠成 `int&`）。若写成 `auto&& uref2 = 27;`，右值使 `auto` 推为 `int`，结果是 `int&&`。

2. `x` 的类型是 `std::initializer_list<int>`。原因是标准专门为 `auto` 规定了一条推导规则：当初始化表达式是 braced-init-list 时，`auto` 被推导为该列表的 `std::initializer_list<T>`。模板推导没有这条规则，`{1,2,3}` 不属于可由模板实参推导的表达式形式（`T` 无从下手），所以 `f({1,2,3})` 直接编译失败——除非形参显式写成 `std::initializer_list<T>`。这是 `auto` 与模板推导唯一的语义差异。

3. C++14（`auto x{11};`）：`x` 是 `std::initializer_list<int>`，沿用与 `auto x = {11}` 相同的规则。C++17 起：标准改为单元素花括号按"直接初始化"处理，`x` 推导为 `int`。也就是说 C++17 让 `auto x{11}` 与 `auto x = 11` 行为一致，消除了最常见的歧义。

4. 因为 `auto` 推导就是把 `auto` 当成模板参数 `T`、把声明的修饰符（`&`、`&&`、`const`）当成 `ParamType`，然后套用 Item 1 的三条模板推导规则：引用形态忽略引用性保留 const；`T&&` 形态左值推成 `T&` 再引用折叠；按值形态剥掉引用和顶层 const。唯一例外是 braced-init-list：`auto` 会推导出 `std::initializer_list<T>`，而等价的模板调用无法推导、编译失败。

</details>

---

## 参考与延伸

- 下一节：[Item 3 decltype](item03-decltype.md)
- 回到：[第 1 章 类型推导](README.md)
