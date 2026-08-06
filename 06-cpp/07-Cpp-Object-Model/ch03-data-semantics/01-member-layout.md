# 3.1 成员布局规则

> 第 3 章 数据语义 · 上一节：[本章导读](README.md) · 下一节：[3.2 继承布局](02-inheritance-layout.md)

## 这节讲什么

类的数据成员在内存中如何排列？padding 如何影响 `sizeof`？这是预测 cache 行为的基础。

---

## 核心规则

- 非 static 成员按**声明顺序**排列
- 中间可能插入 **padding**（对齐要求）
- static 成员不在对象内

```cpp
struct A { char c; int i; };
// sizeof(A) = 8（c 后 3 字节 padding，再 i），不是 5
struct B { int i; char c; };
// sizeof(B) = 8（i 在前，c 后 3 字节 padding）
```

**字段重排减 padding**：大对齐类型放前面，小对齐放后面，减少 padding。

---

## 新手要点（和 C 的区别）

- **C 结构体也有 padding**：这在 C 里完全一样——《C 和指针》ch10 讲过结构体对齐。C++ 的 class 成员布局规则和 C struct 相同。
- **编译器可能重排**：C++ 标准不保证布局顺序跨编译器一致（但实际多数按声明序）。C 标准也是同 样的保证（C11 起有 `_Alignas`）。

---

## HFT 关联

- **字段重排减 padding**：合理排列成员（大对齐在前）减少 padding，`sizeof` 缩小，cache 友好。

---

## 自测题

1. `struct { char c; int i; }` 的 `sizeof` 是多少？padding 在哪里？
2. 如何通过字段重排减少 padding？
3. static 成员在对象的内存里吗？

---

## 参考与延伸

- 下一节：[3.2 继承布局](02-inheritance-layout.md)
- 回到：[第 3 章 数据语义](README.md)
