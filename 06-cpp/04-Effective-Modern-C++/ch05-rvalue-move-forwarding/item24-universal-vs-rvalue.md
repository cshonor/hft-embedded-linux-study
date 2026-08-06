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

---

## 参考与延伸

- 下一节：[Item 25 move vs forward 使用](item25-move-vs-forward-usage.md)
- 回到：[第 5 章](README.md)
