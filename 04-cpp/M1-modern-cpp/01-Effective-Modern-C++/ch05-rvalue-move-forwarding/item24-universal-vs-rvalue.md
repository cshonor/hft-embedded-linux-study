# Item 24：区分万能引用和右值引用

> 第 5 章 · Item 24 · 上一节：[Item 23 move/forward](item23-move-and-forward.md)

## 这节讲什么

`T&&` 在**类型推导发生的语境**（模板 + `auto`）里是万能引用，能绑左值也能绑右值；在无推导的语境里是纯右值引用，只绑右值。这是 C++ 最容易混淆的语法之一。

---

## 核心区别

```cpp
void f(Widget&& w);               // 右值引用（无推导，只绑右值）
template<class T> void g(T&& x);  // 万能引用（有推导，左右值都绑）
auto&& x = expr;                  // 万能引用（auto 推导）
```

**万能引用的推导**：
- 左值实参 → `T` 推为 `T&`（引用折叠）
- 右值实参 → `T` 推为 `T`

这就是"万能"的根源——左值和右值都能绑。

### 判断标准

`T&&` 是万能引用当且仅当：
1. 发生类型推导（模板参数 `T` 或 `auto`）
2. 形式恰好是 `T&&`（不是 `const T&&`，不是 `std::vector<T>&&`）

```cpp
template<class T> void f(const T&& x);  // 不是万能引用（有 const）
template<class T> void g(std::vector<T>&& v);  // 不是万能引用（&& 不直接在 T 上）
```

---

## 新手要点（和 C 的区别）

- **C 没有引用**（C 只有指针）：C 程序员学 C++ 引用时要分清"引用是别名，不是指针"。
- **`T&&` 不总是右值引用**：看到 `T&&` 先问"这里有没有类型推导"——有 = 万能引用，没有 = 右值引用。
- **万能引用 ≠ 万能药**：它只是"能绑左右值的引用"，正确使用仍需配合 `std::forward`。

---

## HFT 关联

- **泛型回调注册**：`template<class F> void setCallback(F&& f)` 的 `F&&` 是万能引用，能接 lambda/函数指针/仿函数，配合 `forward` 原样转发。

---

## 自测题

1. `void f(Widget&&)` 和 `template<class T> void g(T&&)` 的 `&&` 有何不同？
2. 万能引用的判断标准是什么？
3. `const T&&` 是万能引用吗？为什么？
4. 万能引用接左值时 `T` 推成什么？接右值时呢？

<details>
<summary>参考答案</summary>

1. `void f(Widget&&)` 里的 `&&` 是普通**右值引用**：类型 `Widget` 是确定的、不涉及推导，因此形参只能绑定右值（不能接左值），它表达"我要接管一个即将消亡的对象"。`template<class T> void g(T&&)` 里的 `&&` 是**万能引用**（universal reference / forwarding reference）：`T` 需要推导，因此它既能接左值也能接右值，接左值时经引用折叠变成 `T&`，接右值时是 `T&&`，配合 `std::forward<T>` 才能保持原值类别转发。

2. 两条同时满足：①形参形式恰好是 `T&&`（不含 `const`、不含其他修饰，如 `std::vector<T>&&` 就不是）；②`T` 是**在该函数模板中被推导**的类型参数（不是 `Widget&&`、不是类模板参数如 `vector<T>::iterator&&`、不是 `const T&&`）。典型形式：`template<class T> void f(T&& x)`、`auto&& x = expr;`（`auto&&` 同样是万能引用，因为 `auto` 等价于推导）。

3. 不是。`const T&&` 虽然带 `T&&` 形式，但多了 `const` 限定——它不符合"恰好是 `T&&`"这一形式要求，因此是**右值引用**，只能绑定 const 右值，不能接左值。这也说明判断万能引用必须严格看形式：加了任何修饰（`const`、`volatile`）就退回普通右值引用。

4. 接左值：`T` 被推导为 `T&`（左值引用类型），形参 `T&&` 变成 `T& &&`，经引用折叠得到 `T&`。接右值：`T` 被推导为 `T`（值类型），形参类型为 `T&&`。正是这条"左值把 `T` 推成引用类型"的特殊规则，加上引用折叠，才让同一个函数模板既能接受左值又能接受右值，并让 `std::forward<T>` 能还原原始值类别——这就是完美转发的根基。

</details>

---

## 参考与延伸

- 下一节：[Item 25 move vs forward 使用](item25-move-vs-forward-usage.md)
- 回到：[第 5 章](README.md)
