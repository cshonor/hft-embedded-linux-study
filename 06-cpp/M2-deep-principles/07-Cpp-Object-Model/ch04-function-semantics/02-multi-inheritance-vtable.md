# 4.2 多重继承的 vtable 与 this 调整

> 第 4 章 · 上一节：[4.1 虚函数分派](01-vtable-dispatch.md) · 下一节：[4.3 虚基类下的虚函数](03-virtual-base-vfunc.md)

## 这节讲什么

多重继承下派生类有多个 vptr。调非首基类的虚函数时，`this` 要调整到对应基类子对象——thunk 技术在 vtable 里插入调整代码。

---

## this 调整与 thunk

```
class D : public A, public B { ... };
// D 的内存布局：[A vptr | A data | B vptr | B data | D data]

D* d = new D;
B* b = d;  // b 指向 D 中 B 子对象的开头（this 调整）
b->f();    // 调 B 的虚函数 f，但实际对象是 D
// vtable 里的 thunk 先把 this 调整回 D 的起始，再调 D::f
```

**thunk** 是 vtable 里的一段小代码：先调整 `this` 指针，再跳转到实际函数。这让多继承的虚函数调用比单继承多一次 this 调整。

---

## 新手要点

- **多继承的隐藏代价**：不是"不能多继承"，而是每次调非首基类的虚函数都有 this 调整开销。
- **C 程序员的理解**：相当于 C 里 `struct D { A a; B b; }`，访问 `b` 的方法时要把指针从 `D*` 偏移到 `B*`。

---

## HFT 关联

- **避免多继承热路径**：多继承的 this 调整 + 多个 vptr 让对象膨胀 + 调用变慢。用组合替代。

---

## 自测题

1. 多重继承下调非首基类虚函数，`this` 如何调整？
2. thunk 是什么？它在 vtable 里做什么？
3. 为什么多继承比单继承有额外的运行时代价？

---

## 参考与延伸

- 下一节：[4.3 虚基类下的虚函数](03-virtual-base-vfunc.md)
- 回到：[第 4 章 函数语义](README.md)
