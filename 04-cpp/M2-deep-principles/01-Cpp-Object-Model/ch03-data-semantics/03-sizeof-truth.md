# 3.3 sizeof 的真相

> 第 3 章 · 上一节：[3.2 继承布局](02-inheritance-layout.md) · 下一节：[3.4 指向数据成员的指针](04-pointer-to-member.md)

## 这节讲什么

`sizeof(Derived)` 到底由什么组成？vptr、padding、基类子对象都贡献了什么？理解 sizeof 的组成能精确控制对象大小，优化 cache 利用率。

---

## 为什么要学这个（先建立直觉）

C 程序员的 sizeof 心智模型很简单：

```c
struct Foo_C {
    int x;     // 4
    char c;    // 1 + 3 padding
};
// sizeof = 8，就是成员 + padding
```

C++ 的 sizeof 多了几个隐藏贡献者：

```cpp
class Foo {
    virtual void f();  // 贡献 vptr（8B）
    int x;             // 4
    char c;            // 1 + 3 padding
};
// sizeof = 16（vptr 8 + x 4 + c 1 + pad 3），不是 8！
```

继承还引入基类子对象：

```cpp
class Derived : public Base {
    int y;
};
// sizeof(Derived) = sizeof(Base子对象) + y + padding
// 如果 Base 有虚函数，Base子对象包含 vptr
```

---

## sizeof 的组成详解

### 公式

```
sizeof(类) = vptr(如有) + 各基类子对象 + 自身成员 + padding
```

### 逐项分解

```cpp
class Base {
    virtual void f();   // → vptr 8B
    int x;              // → 4B
};
// sizeof(Base) = 16（vptr 8 + x 4 + padding 4）

class Derived : public Base {
    int y;              // → 4B
    char z;             // → 1B
};
// sizeof(Derived) = 24?
// [vptr(8) | x(4) | y(4) | z(1) | padding(7)] = 24
// 或 [vptr(8) | x(4) | pad(4) | y(4) | z(1) | pad(3)] = 24
// 具体 padding 取决于编译器
```

### 多继承的 sizeof

```cpp
class A { virtual void fa(); int a; };  // 16B
class B { virtual void fb(); int b; };  // 16B
class C : public A, public B { int c; };
// [A:vptr|a | B:vptr|b | c | pad]
// sizeof(C) = 40（两个 vptr + 三个 int + padding）
```

### 虚继承的 sizeof

```cpp
class A { int a; };                    // 4B
class B : virtual public A { int b; }; // vbptr(8) + b(4) + pad + A(4) = 24B
// 虚继承额外贡献虚基类指针
```

### padding 可能被复用

```cpp
class Base { virtual void f(); int x; };
// [vptr(8) | x(4) | pad(4)] = 16
class Derived : public Base { int y; };
// [vptr(8) | x(4) | y(4)] = 16  ← y 复用了 Base 的 padding！
// sizeof 不增加！
```

但 padding 复用不可依赖——不同编译器行为不同。

---

## 常见错误（新手踩坑）

### 错误 1：忘了 vptr

```cpp
class Shape { virtual void draw(); int color; };
// 新手以为 sizeof = 4（只有 color）
// 实际 sizeof = 16（vptr 8 + color 4 + padding 4）
```

### 错误 2：多继承多个 vptr

```cpp
class A { virtual void f(); };
class B { virtual void g(); };
class C : public A, public B {};
// 新手以为 sizeof = 8（一个 vptr）
// 实际 sizeof = 16（两个 vptr，每个基类一个）
```

### 错误 3：空基类优化

```cpp
class Empty {};
class A : public Empty { int x; };
// sizeof(A) = 4（空基类优化，Empty 不占空间）
class B { Empty e; int x; };
// sizeof(B) = 8（Empty 作为成员占 1B + padding）
// 继承空类 vs 组合空类：继承有 EBO 优化
```

---

## 和 C 的区别

| 特性 | C sizeof | C++ sizeof |
|------|---------|-----------|
| 组成 | 成员 + padding | 成员 + padding + **vptr** + **基类子对象** |
| 空结构体 | 0（GCC 扩展）或 1 | **保证 ≥ 1** |
| 继承 | N/A | 基类子对象 + 派生成员 |
| 隐藏开销 | 无 | vptr（虚函数）、vbptr（虚继承） |
| 可预测性 | 高 | 低（编译器可调 padding 策略） |

---

## HFT 关联

1. **sizeof 直接影响 cache**：`sizeof(Order)` 从 56 变 64（加 vptr），每 cache 行从 1 个变 1 个——白白浪费 8B。从 48 变 56，每行从 1 个变 1 个——更亏。设计时要让 sizeof 尽量是 64 的因数。
2. **避免多个 vptr**：多继承多个 vptr 让对象膨胀。用组合替代多继承——只存数据，不存 vptr。
3. **EBO 优化**：用空基类继承（EBO）而不是组合空类——省 1 字节 + padding。

---

## 代码自测

### Q1: sizeof 推断

```cpp
class A { int x; };
class B { virtual void f(); int x; };
class C : public B { int y; };
// sizeof(A) = ?  sizeof(B) = ?  sizeof(C) = ?
```

<details>
<summary>答案与复习指引</summary>

`sizeof(A) = 4`。`sizeof(B) = 16`（vptr 8 + x 4 + padding 4）。`sizeof(C) = 16`（vptr 8 + x 4 + y 4，y 复用了 B 的 padding）。如果 y 不能复用 padding 则 sizeof = 24。

**复习：** → [3.3 sizeof 的真相](./03-sizeof-truth.md)
</details>

### Q2: 多继承 vptr

```cpp
class A { virtual void fa(); };
class B { virtual void fb(); };
class C : public A, public B {};
// sizeof(C) = ?
```

<details>
<summary>答案与复习指引</summary>

`sizeof(C) = 16`（A 的 vptr 8 + B 的 vptr 8）。多继承每个有虚函数的基类贡献一个 vptr。这就是多继承让对象膨胀的原因之一。

**复习：** → [3.3 sizeof 的真相](./03-sizeof-truth.md)
</details>

### Q3: 空基类优化

```cpp
class Empty {};
class A : public Empty { int x; };   // 继承
class B { Empty e; int x; };          // 组合
// sizeof(A) = ?  sizeof(B) = ?
```

<details>
<summary>答案与复习指引</summary>

`sizeof(A) = 4`（空基类优化 EBO，Empty 不占空间）。`sizeof(B) = 8`（Empty 成员占 1B + padding 3B + x 4B）。继承空类比组合空类更省空间。

**复习：** → [3.3 sizeof 的真相](./03-sizeof-truth.md)
</details>

### Q4: cache 行设计

```cpp
struct Order {
    int symbol;       // 4
    int qty;          // 4
    double price;     // 8
    long timestamp;   // 8
    char side;        // 1 + 7 padding
};
// sizeof = ?  一个 64B cache 行装几个？
// 如何优化让每行装更多？
```

<details>
<summary>答案与复习指引</summary>

sizeof = 32（symbol 4 + qty 4 + price 8 + timestamp 8 + side 1 + padding 7）。cache 行装 64/32 = 2 个。优化：去掉 side 或用位域，让 sizeof = 24 → 装不了 3 个（64/24=2.67）。或者精简到 sizeof = 16（symbol 4 + qty 4 + price 8，去掉 timestamp 和 side）→ 装 4 个。

**复习：** → [3.3 sizeof 的真相](./03-sizeof-truth.md)
</details>

---

## 参考与延伸

- 下一节：[3.4 指向数据成员的指针](04-pointer-to-member.md)
- 回到：[第 3 章 数据语义](README.md)
