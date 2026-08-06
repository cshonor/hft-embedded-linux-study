# Item 14：声明 noexcept 如果函数保证不抛

> 第 3 章 移步现代 C++ · Item 14 · 上一节：[Item 13 const_iterator](item13-const-iterator.md)

## 这节讲什么

`noexcept` 是函数接口契约的一部分。对**移动构造**、**swap**、**析构**标 `noexcept` 尤其关键——STL 容器在 `push_back` 扩容时会检查元素移动构造是否 `noexcept`：是则用移动（快），否则退回拷贝（安全）。

---

## 核心机制

```cpp
class Widget {
public:
    Widget(Widget&& other) noexcept;  // noexcept 移动构造
    Widget& operator=(Widget&& other) noexcept;
};
```

STL 容器扩容时的分派逻辑：
```cpp
// vector::push_back 扩容时
if (is_nothrow_move_constructible_v<T>)
    move old elements;    // O(1) per element，快
else
    copy old elements;    // O(n) per element，安全
```

**标错 `noexcept` 但抛异常会 `std::terminate`**——所以 `noexcept` 是承诺，不是建议。

---

## 新手要点（和 C 的区别）

- **C 没有异常**：C 程序员不关心 `noexcept`。C++ 有异常机制，`noexcept` 是"我保证不抛异常"的契约。
- **最该标 noexcept 的**：移动构造、移动赋值、swap、析构。这四个影响 STL 容器性能。
- **别乱标**：不确定会不会抛异常就别标 `noexcept`。标错了比不标更危险。

---

## HFT 关联

- **扩容延迟尖峰**：`vector<Order> push_back` 扩容时，`Order` 的移动构造必须 `noexcept` 才走移动语义——否则扩容退回拷贝，订单簿重建延迟尖峰。这是 HFT C++ 性能的**隐形开关**。

---

## 自测题

1. STL 容器 `push_back` 扩容时如何决定用移动还是拷贝？`noexcept` 在其中起什么作用？
2. 标了 `noexcept` 但函数抛了异常会发生什么？
3. 最应该标 `noexcept` 的四个函数是什么？
4. 为什么说"标错 noexcept 比不标更危险"？

---

## 参考与延伸

- 下一节：[Item 15 constexpr](item15-constexpr.md)
- 回到：[第 3 章 移步现代 C++](README.md)
