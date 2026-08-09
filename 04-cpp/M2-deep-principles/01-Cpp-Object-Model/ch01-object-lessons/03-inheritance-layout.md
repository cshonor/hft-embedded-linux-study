# 1.3 继承布局

> 第 1 章 · 上一节：[1.2 虚函数与 vptr](02-vptr-vtable.md) · 下一节：[1.4 封装的代价](04-encapsulation-cost.md)

## 这节讲什么

单继承、多继承、虚继承的对象布局有何不同？每种继承都有代价——vptr 数量、this 调整、额外指针。理解布局是预测 sizeof 和 cache 行为的前提。

---

## 为什么要学这个（先建立直觉）

C 程序员用结构体嵌套模拟"继承"：

```c
// C 的"继承"：手动嵌套
struct Animal_C { int age; };
struct Dog_C { struct Animal_C base; int barkCount; };
// sizeof(Dog_C) = sizeof(Animal_C) + sizeof(int) = 8
// 访问 base: dog.base.age
```

C++ 的继承让编译器自动处理布局，但引入了 C 没有的复杂度——虚继承、多重继承的 vptr 和 this 调整：

```cpp
// C++ 单继承：编译器自动布局
class Animal { int age; };
class Dog : public Animal { int barkCount; };
// sizeof(Dog) = 8，布局和 C 嵌套一样
// 但如果 Animal 有虚函数，布局就不同了
```

---

## 三种继承的布局详解

### 单继承

```cpp
class Base { virtual void f(); int x; };  // [vptr|x] = 16B
class Derived : public Base { int y; };    // [vptr|x|y|pad] = 16B 或 24B
// 派生成员追加在基类之后，复用基类的 vptr
```

```
内存布局：
[vptr | Base::x | Derived::y | padding]
 ↑ 指向 Derived 的 vtable（覆盖了 Base 的 vtable 条目）
```

### 多继承

```cpp
class A { virtual void fa(); int a; };  // 16B
class B { virtual void fb(); int b; };  // 16B
class C : public A, public B { int c; }; // 40B
// [A:vptr|a | B:vptr|b | c | pad]
```

```
内存布局：
[A vptr | a | B vptr | b | c | padding]
 ↑ C* 指向这里      ↑ B* 指向这里（this 调整！）
```

`C* c = new C; B* b = c;` 时 `b` 不等于 `c`——编译器把指针偏移到 B 子对象的位置。

### 虚继承

```cpp
class A { virtual void f(); int a; };
class B : virtual public A { int b; };
class C : virtual public A { int c; };
class D : public B, public C { int d; };
// D 中 A 只存一份（解决菱形继承）
// 但需要虚基类指针/偏移表定位 A 的位置
```

```
内存布局：
[B vptr | B::vbptr | b | C vptr | C::vbptr | c | d | ... | A(共享) ]
                                    ↑ 经 vbptr 间接定位 A
```

---

## 常见错误（新手踩坑）

### 错误 1：多继承指针转换不是简单的 reinterpret_cast

```cpp
class A { int a; };
class B { int b; };
class C : public A, public B {};
C* c = new C;
B* b = c;       // OK，编译器自动偏移 this
void* v = c;
B* b2 = static_cast<B*>(v);  // UB！v 指向 C 的起始，不是 B 子对象
```

### 错误 2：虚继承的性能陷阱

```cpp
class Base { virtual void f(); int x; };
class Left : virtual public Base { int y; };
class Right : virtual public Base { int z; };
class Derived : public Left, public Right { int w; };
// sizeof(Derived) 可能 = 48+（多个 vbptr + vptr + padding）
// 访问 Base::x 需经虚基类偏移表 → 两次间接
```

### 错误 3：菱形继承不虚继承导致数据重复

```cpp
class A { int data; };
class B : public A {};      // 非虚继承
class C : public A {};      // 非虚继承
class D : public B, public C {};
// D 有两份 A::data！
// d.data;  // 编歧错误：是 B::data 还是 C::data？
// 修正：class B : virtual public A {}
```

---

## 和 C 的区别

| 特性 | C 嵌套结构体 | C++ 继承 |
|------|-------------|---------|
| 单继承 | `struct D { Base b; int x; }` | `class D : public Base { int x; }` |
| 布局 | 手动管理 | 编译器自动 |
| 多继承 | 多层嵌套，手动偏移 | 编译器自动布局 + this 调整 |
| 虚继承 | 无等价物 | 虚基类指针/偏移表，共享一份 |
| 函数分派 | 函数指针，手动 | vtable，自动 |
| 隐式开销 | 无 | vptr/vbptr（仅有虚函数/虚继承时） |

---

## HFT 关联

1. **避免多继承/虚继承热路径**：布局膨胀 + this 调整 + 额外指针让对象变大、访问变慢。用组合（has-a）替代。
2. **单继承 + 无虚函数 = 零开销**：`class Order : public PodBase { int qty; };` 如果无虚函数，布局和 C 嵌套结构体完全一样，零额外开销。
3. **虚继承的 cache 灾难**：虚基类指针引入额外间接，破坏 cache 局部性。HFT 数据结构绝不用虚继承。

---

## 代码自测

### Q1: sizeof 推断

```cpp
class A { int a; };                        // 无虚函数
class B { virtual void f(); int b; };      // 有虚函数
class C : public A, public B { int c; };   // 多继承
// sizeof(A) = ?  sizeof(B) = ?  sizeof(C) = ?
```

<details>
<summary>答案与复习指引</summary>

`sizeof(A) = 4`。`sizeof(B) = 16`（vptr 8 + int 4 + padding 4）。`sizeof(C) = 24`（A 子对象 4 + padding 4 + B 子对象 vptr 8 + b 4 + c 4 = 24，具体取决于编译器布局，但至少包含 B 的 vptr）。

**复习：** → [1.3 继承布局](./03-inheritance-layout.md)
</details>

### Q2: 多继承 this 调整

```cpp
class A { public: int a; };
class B { public: int b; };
class C : public A, public B {};
C* c = new C;
B* b = c;
// c == b 吗？为什么？
```

<details>
<summary>答案与复习指引</summary>

`c != b`（通常情况）。`c` 指向 C 对象的起始（A 子对象），`b` 需要偏移到 B 子对象的位置。编译器自动做 this 调整：`b = (char*)c + sizeof(A子对象)`。

**复习：** → [1.3 继承布局](./03-inheritance-layout.md)
</details>

### Q3: 菱形继承

```cpp
class A { public: int data; };
class B : public A {};
class C : public A {};
class D : public B, public C {};
D d;
// d.data = 42;  // 能编译吗？
```

<details>
<summary>答案与复习指引</summary>

不能编译——歧义错误。D 有两份 A::data（B::data 和 C::data），编译器不知道你指哪个。修正：`d.B::data = 42;` 或使用虚继承 `class B : virtual public A {}`。

**复习：** → [1.3 继承布局](./03-inheritance-layout.md)
</details>

### Q4: 布局选择

```cpp
// 方案 A：多继承
class FastOrder : public Header, public Payload, public RiskCheck {};

// 方案 B：组合
struct FastOrder {
    Header header;
    Payload payload;
    RiskCheck risk;
};
```
> 哪个更适合 HFT 热路径？为什么？

<details>
<summary>答案与复习指引</summary>

方案 B（组合）更适合。多继承引入多个 vptr（如果基类有虚函数）+ this 调整开销；组合布局可预测、sizeof 可控、cache 友好。组合是 HFT 的首选——除非基类没有虚函数且不需要多态。

**复习：** → [1.3 继承布局](./03-inheritance-layout.md)
</details>

---

## 参考与延伸

- 下一节：[1.4 封装的代价](04-encapsulation-cost.md)
- 回到：[第 1 章 关于对象](README.md)
