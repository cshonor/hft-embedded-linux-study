# 4.5 inline

> 第 4 章 · 上一节：[4.4 指向成员函数的指针](04-pointer-to-member-func.md) · 下一章：[第 5 章 构造、析构与拷贝](../ch05-construction-destruction-copy/README.md)

## 这节讲什么

`inline` 是对编译器的建议。内联后函数体展开，省 call/ret 开销 + 开启跨函数优化。但过度内联增大代码段（I-cache 压力）。

---

## 收益与代价

**收益**：
- 省 call/ret 指令开销
- 开启跨函数优化（常量传播、死代码消除）
- 参数已知 → 更激进的优化

**代价**：
- 代码段增大 → I-cache 压力
- 编译时间增加

编译器自行权衡（基于成本模型）。`virtual` 函数通常不内联（间接调用）。

---

## 新手要点

- **`inline` 只是建议**：写了 `inline` 编译器不一定内联，不写也可能内联。现代编译器自己决定。
- **`constexpr` 隐含 inline**（C++17）：`constexpr` 函数默认可内联。
- **头文件里的函数**：要在头文件定义（而非只声明）才能内联——`inline` 允许多重定义不报链接错误。

---

## HFT 关联

- **热路径小函数内联**：省 call 开销，但过度内联撑爆 I-cache。用 `__attribute__((flatten))` / PGO 引导内联决策。
- **CRTP + inline = 零开销多态**：`template<class D> struct Base { inline void f() { static_cast<D*>(this)->impl(); } };` 编译期内联，无虚函数开销。

---

## 自测题

1. `inline` 的收益和代价分别是什么？
2. 为什么 `virtual` 函数通常不内联？
3. 过度 inline 有什么代价？HFT 如何权衡？
4. CRTP 如何实现零开销的多态？

---

## 参考与延伸

- 下一章：[第 5 章 构造、析构与拷贝](../ch05-construction-destruction-copy/README.md)
- 回到：[第 4 章 函数语义](README.md)
