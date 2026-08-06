# Item 13：优先 const_iterator 而非 iterator

> 第 3 章 移步现代 C++ · Item 13 · 上一节：[Item 12 override](item12-override.md)

## 这节讲什么

C++11 引入 `cbegin()`/`cend()`，配合 `auto` 拿到 `const_iterator`，防止意外修改。STL 算法在 C++14 起支持 `const_iterator`。

---

## 核心用法

```cpp
std::vector<int> v = {1, 2, 3};
auto it = std::find(v.cbegin(), v.cend(), 2);  // const_iterator，不能通过 it 修改元素
```

C++14 起算法接受 `const_iterator` 并返回同类型，C++11 的部分算法只接受 `iterator`。

---

## 新手要点（和 C 的区别）

- **C 指针的 const 约定**：`const int* p` 指向 const 数据。C++ 迭代器的 `const_iterator` 类似——`const_iterator` 相当于"指向 const 元素的指针"。
- **规则**：不修改元素的迭代用 `cbegin()`/`cend()`，需要修改时才用 `begin()`/`end()`。

---

## HFT 关联

- **只读遍历行情**：`for (auto it = ticks.cbegin(); it != ticks.cend(); ++it)` 表达"只读"意图，编译器帮你检查。

---

## 自测题

1. `const_iterator` 和 `const iterator` 有什么区别？
2. C++11 和 C++14 对 `const_iterator` 在 STL 算法中的支持有何不同？
3. 为什么推荐用 `cbegin()`/`cend()` 而非 `begin()`/`end()`？

---

## 参考与延伸

- 下一节：[Item 14 noexcept](item14-noexcept.md)
- 回到：[第 3 章 移步现代 C++](README.md)
