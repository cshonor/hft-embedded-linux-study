# Item 11：优先 = default 声明默认构造

> 第 3 章 移步现代 C++ · Item 11 · 上一节：[Item 10 scoped enum](item10-scoped-enum.md)

## 这节讲什么

`= default` 让编译器生成默认实现，比手写空函数体更可靠——手写 `C() {}` 会变成"用户定义"，影响是否为 trivial 类型（影响 `memcpy` 合法性与 ABI）。

---

## 核心区别

```cpp
struct A { A() {} };           // 用户定义：A 不是 trivial
struct B { B() = default; };   // 编译器生成：B 是 trivial（如果成员都是 trivial）
```

trivial 类型的优势：
- 可以安全 `memcpy` / `memset`
- 可以作为 `union` 成员
- ABI 边界传递更高效

---

## 新手要点（和 C 的区别）

- **C 没有构造函数**：C 的结构体 `struct A { int x; };` 自动获得"零初始化"。C++ 的类有构造函数，手写空的 `A() {}` 会改变类型的 trivial 性质。
- **规则**：想用编译器默认行为就写 `= default`，别手写空函数体。只有当真的需要自定义逻辑时才手写。

---

## HFT 关联

- **POD 类型**：HFT 协议结构体用 `= default` 保持 trivial，可用 `memcpy` 直接操作网络缓冲区。
- **`memcpy` 合法性**：trivially copyable 类型才能安全 `memcpy`——手写空构造会破坏这一性质。

---

## 自测题

1. `A() {}` 和 `A() = default;` 有什么区别？哪个保持 trivial 性质？
2. 为什么 trivial 类型可以用 `memcpy` 而 non-trivial 不行？
3. 什么场景需要手写构造函数而非 `= default`？

---

## 参考与延伸

- 下一节：[Item 12 override](item12-override.md)
- 回到：[第 3 章 移步现代 C++](README.md)
