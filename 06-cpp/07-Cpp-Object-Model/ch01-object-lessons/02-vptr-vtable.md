# 1.2 虚函数与 vptr

> 第 1 章 · 上一节：[1.1 对象模型三规则](01-object-model-rules.md) · 下一节：[1.3 继承布局](03-inheritance-layout.md)

## 这节讲什么

有虚函数的类，对象内多一个 `vptr`（虚表指针）指向 `vtable`（虚函数表）。虚函数调用经 vptr 间接取地址——比普通函数多一次访存。

---

## 核心机制

```cpp
class Shape { public: virtual void draw(); virtual double area(); };
// sizeof(Shape) = 8（vptr），draw/area 在 vtable 里
```

虚函数调用流程：
```
shape->draw()
  → 取 shape 的 vptr
  → 查 vtable[draw 的 slot]
  → 间接 call 该地址
```

比直接 call 多：①一次间接访存（vtable 可能 cache miss）②一次间接跳转（分支预测代价）③**无法内联**（编译期不知实际调哪个）。

---

## 新手要点（和 C 的区别）

- **C 没有虚函数**：C 用函数指针 + `switch` 手动实现"多态"。C++ 的虚函数是编译器自动生成 vtable——等价于 C 的函数指针数组，但自动化。
- **vptr 是隐式的**：你不会在代码里看到 `vptr`，它是编译器偷偷加到对象头部的。`sizeof` 会多 8 字节（64 位系统）。
- **vptr 在构造时设置**：构造函数执行时设置 vptr 指向当前类的 vtable。这就是为什么构造函数调虚函数不表现多态（调的是当前类的版本）。

---

## HFT 关联

- **热路径禁虚函数**：vtable 间接在每 tick 路径上引入 cache miss + 分支预测代价 + 不可内联。策略分派用 `enum` + `switch`/函数指针数组或 CRTP 静态分派替代。

---

## 自测题

1. vptr/vtable 的工作机制是什么？虚函数调用比普通函数多什么代价？
2. 为什么虚函数不能内联？
3. 构造函数里调虚函数会表现多态吗？为什么？
4. `sizeof(Shape)`（只有两个虚函数，无数据成员）是多少？

---

## 参考与延伸

- 下一节：[1.3 继承布局](03-inheritance-layout.md)
- 回到：[第 1 章 关于对象](README.md)
