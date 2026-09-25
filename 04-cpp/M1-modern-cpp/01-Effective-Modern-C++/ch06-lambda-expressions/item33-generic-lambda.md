# Item 33：对 auto&& 形参用 decltype + std::forward（泛型 lambda）

> 第 6 章 · Item 33 · 上一节：[Item 32 初始化捕获](item32-init-capture.md)

## 这节讲什么

C++14 泛型 lambda 用 `auto&&` 参数实现完美转发——让 lambda 能当泛型转发器用。

---

## 核心用法

```cpp
auto f = [](auto&& x){
    func(std::forward<decltype(x)>(x));
};
```

`auto&&` 是万能引用（有 `auto` 推导）。`decltype(x)` 对 `auto&&` 参数保留左右值性：
- 左值实参 → `decltype(x)` = `T&` → `forward<T&>` 保留左值
- 右值实参 → `decltype(x)` = `T&&` → `forward<T&&>` 转为右值

---

## 新手要点

- **C++14 独有**：C++11 的 lambda 参数必须显式写类型，不能 `auto`。C++14 起支持泛型 lambda。
- **什么时候用**：写泛型转发 lambda 时。普通 lambda 不需要。

---

## 自测题

1. 泛型 lambda 的 `auto&&` 参数如何配合 `std::forward` 实现完美转发？
2. `decltype(x)` 在 `auto&&` 参数上保留什么信息？
3. C++11 和 C++14 的 lambda 参数有什么区别？

<details>
<summary>参考答案</summary>

1. 泛型 lambda 的形参写成 `auto&& x`，`auto&&` 因为有推导所以是**万能引用**：传左值时 `auto` 推为 `T&`（形参折叠成 `T&`），传右值时 `auto` 推为 `T`（形参是 `T&&`）。转发时用 `std::forward<decltype(x)>(x)`：`decltype(x)` 拿到形参的**确切类型**（含引用性），交给 `forward` 后，左值实参保持左值、右值实参保持右值，从而实现完美转发：
```cpp
auto f = [](auto&& x){ func(std::forward<decltype(x)>(x)); };
```
注意不能用 `std::forward<T>`——lambda 没有可写的模板参数 `T`，只能靠 `decltype(x)` 取类型；也不能用 `std::move(x)`，那会把左值也变成右值，掏空调用方的对象。

2. `decltype(x)` 返回形参 `x` 的声明类型，**完整保留引用性与 cv 限定**：左值实参时 `auto&&` 被推成 `T&`，`decltype(x)` 就是 `T&`；右值实参时是 `T&&`。也就是说它保留的是"原始实参的值类别信息"，这正是 `std::forward` 判断"要不要转成右值"所需要的输入。注意它不像 `auto` 那样退化。

3. C++11 的 lambda 形参**必须写具体类型**（`[](int x){}`），不支持 `auto`，因此无法写泛型 lambda；C++14 起允许形参用 `auto`（以及 `auto&&`、`auto*` 等），编译器把闭包类型变成带模板 `operator()` 的类型，每次调用按实参推导。此外 C++14 还放宽了 lambda 的 `constexpr`／返回类型推导，并引入初始化捕获（Item 32）；C++20 进一步支持显式模板参数列表（`[]<class T>(T x){}`）。

</details>

---

## 参考与延伸

- 下一节：[Item 34 lambda vs bind](item34-lambda-vs-bind.md)
- 回到：[第 6 章](README.md)
