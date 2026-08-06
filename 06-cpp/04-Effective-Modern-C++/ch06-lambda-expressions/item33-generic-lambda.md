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

---

## 参考与延伸

- 下一节：[Item 34 lambda vs bind](item34-lambda-vs-bind.md)
- 回到：[第 6 章](README.md)
