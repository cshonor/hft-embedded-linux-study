# 4.1 虚函数分派（vtable）

> 第 4 章 函数语义 · 上一节：[本章导读](README.md) · 下一节：[4.2 多继承的 vtable 与 this 调整](02-multi-inheritance-vtable.md)

## 这节讲什么

虚函数调用的底层流程：经 vptr 查 vtable 再间接 call。比直接 call 多一次访存 + 不可内联。这是理解 C++ 多态性能代价的核心。

---

## 为什么要学这个（先建立直觉）

C 程序员手动实现"多态"——用函数指针 + switch：

```c
// C 的"多态"
enum ShapeType { CIRCLE, SQUARE };
struct Shape {
    enum ShapeType type;
};
double area(struct Shape* s) {
    switch (s->type) {
        case CIRCLE: return circle_area(s);
        case SQUARE: return square_area(s);
    }
}
// 编译器可内联 circle_area/square_area → 直接 call
```

C++ 的虚函数用 vtable 自动分派，但引入间接性：

```cpp
class Shape {
public:
    virtual double area() = 0;
};
class Circle : public Shape {
    double area() override { return 3.14 * r * r; }
};
Shape* s = new Circle;
s->area();  // 经 vptr → vtable → 间接 call Circle::area
// 编译器无法内联（不知 s 实际指向 Circle 还是 Square）
```

关键代价：**间接 call 不可内联**——编译器在编译期不知道 `s` 指向什么类型，不能把 `Circle::area` 的函数体内联到调用点。

---

## 调用流程详解

### 虚函数调用展开

```cpp
shape->draw();
// 编译器展开为：
// 1. vptr = *(void**)(shape)           // 取 vptr（一次访存）
// 2. func = *(void**)(vptr + slot)     // 查 vtable[slot]（一次访存）
// 3. func(shape)                       // 间接 call（不可内联）
```

### 对比直接 call

```cpp
class Widget {
public:
    void process() { /* ... */ }  // 非虚函数
};
Widget* w = new Widget;
w->process();
// 编译器展开为：
// 1. Widget::process(w)               // 直接 call（可内联）
// 没有间接访存，没有分支预测代价
```

### vtable 的内存位置

```
vtable 存在 .rodata 段（只读数据段）
每个有虚函数的类一张 vtable

对象内存：
[vptr → 指向类的 vtable | 数据成员...]

vtable:
[ &Base::f | &Base::g | &Base::h | ... ]
  slot 0     slot 1     slot 2
```

---

## 常见错误（新手踩坑）

### 错误 1：热路径用虚函数

```cpp
class OrderHandler {
public:
    virtual void process(Order& o) = 0;  // 虚函数
};
// 每笔订单调 process() → 间接 call → cache miss + 不可内联
// 在 HFT 每 tick 路径上可能浪费 10-50ns
```

### 错误 2：忘了虚析构

```cpp
class Base { public: virtual void f(); /* 没写虚析构 */ };
class Derived : public Base { int* data; ~Derived() { delete[] data; } };
Base* p = new Derived;
delete p;  // 只调 ~Base()，data 泄漏
```

### 错误 3：在构造函数里调虚函数

```cpp
class Base {
public:
    Base() { init(); }  // 期望调 Derived::init()
    virtual void init() {}
};
class Derived : public Base {
    void init() override { /* 初始化 */ }
};
Derived d;  // Base() 里调的是 Base::init()，不是 Derived::init()
```

---

## 和 C 的区别

| 特性 | C 函数指针 | C++ 虚函数 |
|------|-----------|-----------|
| 实现方式 | 手动维护函数指针 | 编译器自动生成 vtable |
| 对象开销 | 每个函数指针 8B | 固定 1 个 vptr = 8B |
| 内联 | 不可能 | 不可能 |
| 类型安全 | 无（void* 传 this） | 有 |
| 调用代价 | 间接 call | 间接 call（经 vptr→vtable） |

---

## HFT 关联

1. **热路径禁虚函数**：vtable 间接在每 tick 路径上引入 cache miss + 分支预测代价 + 不可内联。用 `enum` + `switch` 或 CRTP 替代。
2. **CRTP 零开销多态**：`template<class D> struct Base { void f() { static_cast<D*>(this)->impl(); } };` 编译期分派，无 vptr、可内联。
3. **vtable 的 cache 行为**：vtable 在 .rodata 段，可能不在热路径数据的 cache 里——首次虚调用可能 cache miss。

---

## 代码自测

### Q1: 调用代价

```cpp
class A { public: void f() {} };                // 非虚
class B { public: virtual void f() {} };        // 虚
A* a = new A; B* b = new B;
// a->f() 和 b->f() 的调用流程有何不同？
```

<details>
<summary>答案与复习指引</summary>

`a->f()`：直接 `call A::f`（可内联，零间接）。`b->f()`：①取 `b` 的 vptr → ②查 `vtable[0]` → ③间接 call。多两次间接访存 + 不可内联 + 分支预测代价。

**复习：** → [4.1 虚函数分派](./01-vtable-dispatch.md)
</details>

### Q2: 为什么不能内联

```cpp
class Shape { public: virtual double area() = 0; };
Shape* s = getShape();  // 运行时才知道类型
s->area();
// 为什么编译器不能内联 area()？
```

<details>
<summary>答案与复习指引</summary>

编译器在编译期不知道 `s` 指向 `Circle` 还是 `Square`——`getShape()` 返回 `Shape*`，实际类型在运行时才确定。虚函数经 vtable 间接 call，编译器无法确定调哪个函数，无法内联。

**复习：** → [4.1 虚函数分派](./01-vtable-dispatch.md)
</details>

### Q3: CRTP 替代

```cpp
template<class Derived>
struct Base {
    void process() { static_cast<Derived*>(this)->doProcess(); }
};
struct Concrete : Base<Concrete> {
    void doProcess() { /* 实现 */ }
};
// Concrete::process() 有虚函数开销吗？
```

<details>
<summary>答案与复习指引</summary>

没有。CRTP 在编译期确定类型——`static_cast<Derived*>(this)->doProcess()` 是直接 call，可内联。无 vptr、无 vtable、无间接访存。HFT 用 CRTP 实现零开销多态。

**复习：** → [4.1 虚函数分派](./01-vtable-dispatch.md)
</details>

### Q4: 构造函数调虚函数

```cpp
class Base {
public:
    Base() { log(); }
    virtual void log() { printf("Base\n"); }
};
class Derived : public Base {
public:
    void log() override { printf("Derived\n"); }
};
// Derived d; 输出什么？
```

<details>
<summary>答案与复习指引</summary>

输出 `Base`。`Base()` 执行时 vptr 指向 Base 的 vtable，`log()` 调的是 `Base::log()`。构造函数里调虚函数不表现多态——vptr 在基类构造期间指向基类的 vtable。

**复习：** → [4.1 虚函数分派](./01-vtable-dispatch.md)
</details>

---

## 参考与延伸

- 下一节：[4.2 多继承的 vtable 与 this 调整](02-multi-inheritance-vtable.md)
- 回到：[第 4 章 函数语义](README.md)
