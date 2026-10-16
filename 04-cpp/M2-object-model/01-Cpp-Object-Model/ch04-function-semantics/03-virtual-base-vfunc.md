# 4.3 虚基类下的虚函数

> 第 4 章 · 上一节：[4.2 多继承的 vtable](02-multi-inheritance-vtable.md) · 下一节：[4.4 指向成员函数的指针](04-pointer-to-member-func.md)

## 这节讲什么

虚基类让 `this` 调整更复杂——需经虚基类偏移表定位。虚函数调用代价更高。这是 C++ 对象模型中最昂贵的分派方式。

---

## 为什么要学这个（先建立直觉）

C 程序员没有"虚继承"的概念——要么嵌套结构体（数据可能重复），要么用指针共享：

```c
// C 的"菱形继承"方案：用指针共享
struct A { int data; };
struct B { struct A* a_ptr; int b; };  // 指向共享的 A
struct C { struct A* a_ptr; int c; };
struct D { struct B b; struct C c; struct A shared_a; };
// 手动让 b.a_ptr 和 c.a_ptr 都指向 d.shared_a
```

C++ 的虚继承让编译器自动管理共享，但代价是运行时间接定位：

```cpp
class A { public: virtual void f(); int data; };
class B : virtual public A { int b; };
class C : virtual public A { int c; };
class D : public B, public C { int d; };
// D 中 A 只存一份，但 B/C 访问 A 需经虚基类偏移表定位
// 比 C 指针方案多了编译器自动管理的开销
```

---

## 核心问题详解

### 虚基类的位置不固定

```cpp
class A { public: int data; };
class B : virtual public A { int b; };
// B 的布局可能是：[vptr | vbptr | b | A::data]
// 也可能是：[vptr | vbptr | b | ... | A::data（在末尾）]
// A 的位置取决于对象的具体类型（B 还是 D）
```

### 虚基类偏移表

```
访问虚基类成员：
obj.data
  → 取 obj 的 vbptr（虚基类指针）
  → 查偏移表获取 A 子对象的偏移量
  → this + offset → A 子对象的地址
  → 访问 data
```

### 虚函数调用的额外代价

```cpp
class A { public: virtual void f(); };
class B : virtual public A {
public:
    void f() override { /* C::f */ }
};
A* a = new B;
a->f();
// 1. 取 a 的 vptr
// 2. 查 vtable[f 的 slot]
// 3. 如果 f 被 override，thunk 需要把 this 从 A* 调整到 B*
//    但 A 是虚基类，位置不固定 → 需经虚基类偏移表定位
// 4. 比 非 虚继承的多继承还多一次间接
```

---

## 常见错误（新手踩坑）

### 错误 1：虚继承的性能陷阱

```cpp
class Base { virtual void f(); int x; };
class Left : virtual public Base { int y; };
class Right : virtual public Base { int z; };
class Derived : public Left, public Right { int w; };
// sizeof(Derived) 可能 = 48+
// 访问 Base::x 需两次间接（vbptr → 偏移表 → x）
```

### 错误 2：default constructor 的隐式开销

```cpp
class A { public: A() {} };
class B : virtual public A {
    // 编译器合成默认构造——设置 vbptr
    // 比非虚继承多一步：设置虚基类偏移表
};
```

### 错误 3：虚基类指针的 sizeof

```cpp
class A { int x; };
class B : virtual public A { int y; };
// sizeof(B) 可能 = 24（vptr + vbptr + y + A + padding）
// 而非虚继承的 sizeof(B) = 8（x + y）
```

---

## 和 C 的区别

| 特性 | C 指针共享 | C++ 虚继承 |
|------|-----------|-----------|
| 共享方式 | 手动指针 | 编译器自动（vbptr + 偏移表） |
| 访问代价 | 一次间接（指针解引用） | 两次间接（vbptr → 偏移表 → 成员） |
| sizeof | 指针 8B | vbptr 8B + 可能的 vptr + padding |
| 虚函数 | N/A | thunk + 虚基类偏移 → 最昂贵的分派 |

---

## HFT 关联

1. **绝不用于热路径**：虚继承的运行时代价（偏移表间接 + 对象膨胀）是 HFT 性能大忌。
2. **用组合替代虚继承**：`struct D { B b; C c; shared_data; }` 手动管理共享，无虚继承开销。
3. **sizeof 不可控**：虚继承让 sizeof 暴涨（vbptr + padding），破坏 cache 行为。

---

## 代码自测

### Q1: 访问代价

```cpp
class A { public: int data; };
class B : virtual public A { int b; };
class C : virtual public A { int c; };
class D : public B, public C { int d; };
D d;
d.data = 42;  // 比 d.b = 1 多几次间接？
```

<details>
<summary>答案与复习指引</summary>

`d.data` 需经虚基类偏移表定位 A 子对象——两次间接（取 vbptr → 查偏移表 → 定位 A）。`d.b` 是直接访问（B 子对象在固定位置）。虚基类成员的访问比普通成员慢。

**复习：** → [4.3 虚基类下的虚函数](./03-virtual-base-vfunc.md)
</details>

### Q2: sizeof 膨胀

```cpp
class A { int x; };                       // 4B
class B_normal : public A { int y; };     // 非虚继承
class B_virtual : virtual public A { int y; }; // 虚继承
// sizeof(B_normal) = ?  sizeof(B_virtual) = ?
```

<details>
<summary>答案与复习指引</summary>

`sizeof(B_normal) = 8`（x 4 + y 4）。`sizeof(B_virtual) = 24`（vbptr 8 + y 4 + padding + A::x 4 + padding）。虚继承让 sizeof 暴涨——多了 vbptr 和 padding。

**复习：** → [4.3 虚基类下的虚函数](./03-virtual-base-vfunc.md)
</details>

### Q3: 何时需要虚继承

```cpp
class A { public: virtual void f(); int data; };
class B : public A {};
class C : public A {};
class D : public B, public C {};
// D d; d.data;  // 会发生什么？
```

<details>
<summary>答案与复习指引</summary>

歧义错误——D 有两份 A::data（B::data 和 C::data）。如果只要一份，需虚继承：`class B : virtual public A {}; class C : virtual public A {};`。但虚继承引入 vbptr 开销。HFT 的建议：避免菱形继承，用组合替代。

**复习：** → [4.3 虚基类下的虚函数](./03-virtual-base-vfunc.md)
</details>

### Q4: 替代方案

```cpp
// 虚继承方案
class SharedBase { int data; };
class Left : virtual public SharedBase {};
class Right : virtual public SharedBase {};
class Derived : public Left, public Right {};

// 组合方案
struct Derived2 {
    SharedBase shared;
    Left left;
    Right right;
};
// 哪个更适合 HFT？
```

<details>
<summary>答案与复习指引</summary>

组合方案。虚继承引入 vbptr + 偏移表间接 + sizeof 膨胀。组合方案 sizeof 可预测，访问直接（无间接），cache 友好。代价是失去了多态——但 HFT 热路径通常不需要多态。

**复习：** → [4.3 虚基类下的虚函数](./03-virtual-base-vfunc.md)
</details>

---

## 参考与延伸

- 下一节：[4.4 指向成员函数的指针](04-pointer-to-member-func.md)
- 回到：[第 4 章 函数语义](README.md)
