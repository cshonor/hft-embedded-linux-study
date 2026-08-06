# 4.1 虚函数分派（vtable）

> 第 4 章 函数语义 · 上一节：[本章导读](README.md) · 下一节：[4.2 多继承的 vtable 与 this 调整](02-multi-inheritance-vtable.md)

## 这节讲什么

虚函数调用的底层流程：经 vptr 查 vtable 再间接 call。比直接 call 多一次访存 + 不可内联。

---

## 调用流程

```cpp
shape->draw();
// 展开：
// 1. 取 shape 的 vptr
// 2. 查 vtable[draw 的 slot]
// 3. 间接 call 该地址
```

比直接 call 多：
- 一次间接访存（vtable 可能 cache miss）
- 一次间接跳转（分支预测代价）
- **无法内联**（编译期不知实际调哪个）

---

## 新手要点（和 C 的区别）

- **C 用函数指针模拟多态**：`struct Shape { void (*draw)(Shape*); };`——C 程序员手动维护"函数指针表"。C++ 的 vtable 是编译器自动生成的函数指针数组。
- **vtable 不可见**：你在代码里看不到 vtable，但它是真实存在的——每个有虚函数的类一张。

---

## HFT 关联

- **热路径禁虚函数**：vtable 间接 + 不可内联是 HFT 性能大忌。用 `switch`/函数指针数组/CRTP 替代。

---

## 自测题

1. 虚函数调用比普通函数多哪些代价？
2. 为什么虚函数不能内联？
3. vtable 和 C 的函数指针数组有什么关系？

---

## 参考与延伸

- 下一节：[4.2 多继承的 vtable 与 this 调整](02-multi-inheritance-vtable.md)
- 回到：[第 4 章 函数语义](README.md)
