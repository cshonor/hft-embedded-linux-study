# 3.2 继承布局

> 第 3 章 · 上一节：[3.1 成员布局](01-member-layout.md) · 下一节：[3.3 sizeof 的真相](03-sizeof-truth.md)

## 这节讲什么

继承下成员布局的细节——单继承、多继承、虚继承各有不同的数据排列方式。布局直接影响 sizeof 和 cache 行为。

---

## 为什么要学这个（先建立直觉）

C 程序员用嵌套结构体模拟"继承"，布局很直观：

```c
// C 的"继承"：嵌套结构体
struct Base_C { int x; };
struct Derived_C { struct Base_C base; int y; };
// sizeof(Derived_C) = sizeof(Base_C) + sizeof(int) = 8
// 访问：derived.base.x 或 derived.y
```

C++ 单继承的布局和 C 嵌套**完全一样**——但如果基类有虚函数，就多了 vptr：

```cpp
class Base { virtual void f(); int x; };   // [vptr|x] = 16B
class Derived : public Base { int y; };     // [vptr|x|y|pad] = 16B 或 24B
// 比嵌套多了 vptr（8B），但复用了同一个 vptr
```

多继承和虚继承则引入了 C 完全没有的布局复杂度。

---

## 三种继承的数据布局详解

### 单继承

```cpp
class Base { int x; };
class Derived : public Base { int y; };
// [Base::x | Derived::y] = 8B
// 如果 Base 有虚函数：[vptr | x | y | pad] = 16B+，vptr 在最前面
```

### 多继承

```cpp
class A { int a; };         // 4B
class B { int b; };         // 4B
class C : public A, public B { int c; };
// [A::a | B::b | c] = 12B（可能加 padding = 16B）
// 如果 A/B 有虚函数：各含自己的 vptr
```

### 虚继承

```cpp
class A { int a; };
class B : virtual public A { int b; };
class C : virtual public A { int c; };
class D : public B, public C { int d; };
// D 中 A 只存一份（在对象末尾）
// B/C 各有虚基类指针指向 A 的位置
// [B vptr | B::vbptr | b | C vptr | C::vbptr | c | d | A::a]
// sizeof 可能 = 40+
```

---

## 常见错误（新手踩坑）

### 错误 1：多继承指针偏移

```cpp
class A { int a; };
class B { int b; };
class C : public A, public B {};
C* c = new C;
printf("%p %p\n", (void*)c, (void*)static_cast<B*>(c));
// 两个地址不同！B* 偏移到了 B 子对象的位置
```

### 错误 2：虚继承的 sizeof 暴涨

```cpp
class Base { int x; };           // 4B
class Left : virtual public Base { int y; };   // 16B+（vbptr + y + padding）
class Right : virtual public Base { int z; };  // 16B+
class D : public Left, public Right { int w; }; // 40B+
// 原本只需 16B（x+y+z+w），虚继承让 sizeof 暴涨
```

### 错误 3：菱形继承数据重复

```cpp
class A { int data; };
class B : public A {};  // 非虚继承
class C : public A {};  // 非虚继承
class D : public B, public C {};
// D 有两份 data（B::data 和 C::data）
// d.data;  // 歧义错误
```

---

## 和 C 的区别

| 特性 | C 嵌套结构体 | C++ 继承 |
|------|-------------|---------|
| 单继承 | `struct D { Base b; int x; }` | `class D : public Base { int x; }`（相同布局） |
| 多继承 | 多层嵌套，手动管理 | 编译器自动布局 + this 调整 |
| 虚继承 | 无等价物 | 虚基类指针/偏移表，共享一份 |
| vptr | 无 | 有虚函数时在对象头部（8B） |

---

## HFT 关联

1. **避免虚继承**：虚继承让对象膨胀（偏移表指针）+ 访问需间接，热路径数据结构避免虚继承。
2. **单继承无虚函数 = 零开销**：`class Order : public PodHeader { int qty; }` 布局和 C 嵌套一样，无额外开销。
3. **组合优于继承**：`struct Engine { MarketData md; OrderBook ob; }` 比 `class Engine : public MarketData, public OrderBook` 更可控——sizeof 可预测，无 this 调整。

---

## 代码自测

### Q1: 单继承 sizeof

```cpp
class Base { int x; };
class Derived : public Base { int y; };
class BaseV { virtual void f(); int x; };
class DerivedV : public BaseV { int y; };
// sizeof(Derived) = ?  sizeof(DerivedV) = ?
```

<details>
<summary>答案与复习指引</summary>

`sizeof(Derived) = 8`（x 4 + y 4，无虚函数）。`sizeof(DerivedV) = 16`（vptr 8 + x 4 + y 4，复用基类 vptr）。有虚函数时多 8B vptr。

**复习：** → [3.2 继承布局](./02-inheritance-layout.md)
</details>

### Q2: 多继承地址

```cpp
class A { int a; };
class B { int b; };
class C : public A, public B { int c; };
C* c = new C;
A* a = c;
B* b = c;
// a == c 吗？b == c 吗？
```

<details>
<summary>答案与复习指引</summary>

`a == c`（A 是第一个基类，地址相同）。`b != c`（B 是第二个基类，b 偏移到 B 子对象位置）。编译器自动做 this 调整。

**复习：** → [3.2 继承布局](./02-inheritance-layout.md)
</details>

### Q3: 虚继承 vs 普通继承

```cpp
class A { int data; };
// 方案1：普通继承
class B1 : public A { int b; };
class C1 : public A { int c; };
class D1 : public B1, public C1 { int d; };
// 方案2：虚继承
class B2 : virtual public A { int b; };
class C2 : virtual public A { int c; };
class D2 : public B2, public C2 { int d; };
// sizeof(D1) = ?  sizeof(D2) = ?
```

<details>
<summary>答案与复习指引</summary>

`sizeof(D1) ≈ 16`（B1::A::data + B1::b + C1::A::data + C1::c + d，A 存两份）。`sizeof(D2) ≈ 40+`（B2 vbptr + b + C2 vbptr + c + d + A::data，虚基类指针 + padding）。虚继承让 A 只存一份但增加了 vbptr 开销。HFT 避免两种方式——用组合。

**复习：** → [3.2 继承布局](./02-inheritance-layout.md)
</details>

### Q4: 组合 vs 继承

```cpp
// 方案 A：继承
class Trader : public MarketData, public RiskCheck, public Logger {};

// 方案 B：组合
struct Trader {
    MarketData md;
    RiskCheck rc;
    Logger log;
};
// 哪个更适合 HFT？
```

<details>
<summary>答案与复习指引</summary>

方案 B（组合）。sizeof 可预测，无 this 调整，无多 vptr（如果基类有虚函数）。组合是 HFT 的首选——只有需要多态时才用继承。

**复习：** → [3.2 继承布局](./02-inheritance-layout.md)
</details>

---

## 参考与延伸

- 下一节：[3.3 sizeof 的真相](03-sizeof-truth.md)
- 回到：[第 3 章 数据语义](README.md)
