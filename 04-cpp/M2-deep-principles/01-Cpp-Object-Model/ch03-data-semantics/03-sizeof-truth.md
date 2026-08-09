# 3.3 sizeof 的真相

> 第 3 章 · 上一节：[3.2 继承布局](02-inheritance-layout.md) · 下一节：[3.4 指向数据成员的指针](04-pointer-to-member.md)

## 这节讲什么

`sizeof(Derived)` 到底由什么组成？vptr、padding、基类子对象都贡献了什么？

---

## 组成

```
sizeof(Derived) = 各基类子对象 + 自身成员 + padding + vptr 数量
```

- 每个有虚函数的基类子对象贡献一个 vptr（8 字节）
- 虚继承额外贡献虚基类指针（8 字节）
- padding 按对齐规则插入

```cpp
class Base { virtual void f(); int x; };  // sizeof = 16 (vptr 8 + int 4 + padding 4)
class Derived : public Base { int y; };     // sizeof = 16 (Base 16, y 复用 padding) 或 24
```

---

## 新手要点

- **vptr 是大头**：有虚函数的类至少多 8 字节。多个有虚函数的基类 → 多个 vptr → 对象膨胀。
- **padding 可能被复用**：派生类成员可能填入基类的 padding 空间——但不可依赖，因编译器而异。

---

## HFT 关联

- **sizeof 直接影响 cache**：64 字节 cache 行能装 `64 / sizeof(Order)` 个对象。`vptr` 让 sizeof 从 56 变 64，每行对象数不变；从 48 变 64，每行对象数从 1 变 1（还是亏了）。

---

## 自测题

1. `sizeof(Derived)` 由哪些部分组成？
2. 有虚函数的基类贡献了什么额外开销？
3. 虚继承对 sizeof 有什么影响？

---

## 参考与延伸

- 下一节：[3.4 指向数据成员的指针](04-pointer-to-member.md)
- 回到：[第 3 章 数据语义](README.md)
