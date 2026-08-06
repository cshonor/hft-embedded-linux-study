# Item 29：认识移动操作不存在或廉价的情形

> 第 5 章 · Item 29 · 上一节：[Item 28 引用折叠](item28-reference-collapsing.md)

## 这节讲什么

移动不是万能的——有些类型没有移动操作，有些移动不比拷贝快。无脑 `std::move` 不总是有效。

---

## 三种"移动无效"的情形

1. **没有移动构造**：旧类、C 兼容结构体没有移动操作，`std::move` 退回拷贝。
2. **移动不比拷贝快**：
   - `std::array<T, N>` 的移动是逐元素移动（O(N)）——因为它内联存储元素，没有指针可交接
   - 小类型（`int`、指针）移动 = 拷贝
3. **常量对象不能移动**：`const T&&` 的移动构造无法修改对象，退回拷贝。`const shared_ptr` 拷贝而非移动。

```cpp
std::vector<std::array<int, 1000>> v;
auto v2 = std::move(v);  // O(N)！array 的移动是逐元素的
```

---

## 新手要点

- **别无脑 move**：`std::move` 不是免费的——对 `array`、小类型、const 对象没用甚至有害。
- **别 move 返回值**：RVO/NRVO 已经做了更好的优化，`return std::move(local)` 反而**阻止** NRVO（因为 `move` 把它变成了右值引用，编译器不能再 RVO）。

---

## HFT 关联

- **`vector<vector<Tick>>` 移动**：桶间用 `move` 转移是 O(1) 真收益；但桶内 `array<Tick, N>` 的移动是 O(N)——选 `vector` 而非 `array` 才能享受移动红利。

---

## 自测题

1. `std::vector<std::array<int, 1000>>` 的移动是 O(1) 还是 O(N)？为什么？
2. 为什么 `return std::move(local)` 反而有害？
3. `const shared_ptr` 能移动吗？为什么？
4. 什么类型的移动和拷贝代价相同？

---

## 参考与延伸

- 下一节：[Item 30 完美转发失败](item30-forwarding-failures.md)
- 回到：[第 5 章](README.md)
