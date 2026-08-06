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

---

## 参考与延伸

- 下一节：[Item 3 decltype](item03-decltype.md)
- 回到：[第 1 章 类型推导](README.md)
