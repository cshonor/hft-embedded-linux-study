# 4.4 指向成员函数的指针

> 第 4 章 · 上一节：[4.3 虚基类下的虚函数](03-virtual-base-vfunc.md) · 下一节：[4.5 inline](05-inline.md)

## 这节讲什么

`void (Shape::*fp)() = &Shape::draw;` 不是普通函数指针——对虚函数它是 vtable 偏移，对非虚函数它是函数地址 + this 调整信息。大小通常是 2 个指针。

---

## 为什么要学这个（先建立直觉）

C 程序员用函数指针模拟"成员函数"：

```c
// C 的"成员函数指针"
struct Shape_C {
    int x;
    void (*draw)(struct Shape_C*);  // 函数指针
};
// 调用：shape->draw(shape) 或 shape->draw(&shape)
// 函数指针大小 = sizeof(void*) = 8 字节
```

C++ 的指向成员函数的指针更复杂——它需要同时记录"调哪个函数"和"怎么调整 this"：

```cpp
class Shape {
public:
    virtual void draw();   // 虚函数
    void resize(int w);    // 非虚函数
};
void (Shape::*fp)() = &Shape::draw;   // 指向虚函数
void (Shape::*fr)(int) = &Shape::resize;  // 指向非虚函数
// sizeof(fp) 可能 = 16（2 个指针），不是 8！
// 因为要存：函数地址/vtable 偏移 + this 调整量
```

---

## 核心概念详解

### 指向非虚成员函数的指针

```cpp
class Widget {
public:
    void process(int x);  // 非虚函数
};
void (Widget::*fp)(int) = &Widget::process;
// fp 存的是：函数地址 + this 调整量（单继承时调整量为 0）
// sizeof(fp) 通常 = 8（单继承）或 16（多继承）

Widget w;
(w.*fp)(42);      // 调 w.process(42)
Widget* wp = &w;
(wp->*fp)(42);    // 同上
```

### 指向虚成员函数的指针

```cpp
class Shape {
public:
    virtual void draw();
};
class Circle : public Shape {
public:
    void draw() override { /* ... */ }
};
void (Shape::*fp)() = &Shape::draw;  // 指向虚函数
// fp 存的是：vtable 偏移（不是函数地址！）

Shape* s = new Circle;
(s->*fp)();  // 1. 取 s 的 vptr
             // 2. 用 fp 里的偏移查 vtable[slot]
             // 3. 间接 call
             // 调的是 Circle::draw！
```

### 多继承下的复杂性

```cpp
class A { public: virtual void fa(); };
class B { public: virtual void fb(); };
class C : public A, public B {};
void (B::*fp)() = &B::fb;
// fp 需要：vtable 偏移 + this 调整量（从 B* 到 C*）
// sizeof(fp) = 16（2 个指针：偏移 + 调整量）
```

---

## 常见错误（新手踩坑）

### 错误 1：混淆函数指针和成员函数指针

```cpp
class Widget { public: void f(); };
void (*fp)() = &Widget::f;           // 编译错误！需要对象
void (Widget::*fpm)() = &Widget::f;  // 正确
Widget w;
w.*fpm;  // 正确调用
// fp 是普通函数指针，fpm 是成员函数指针——类型不同
```

### 错误 2：以为 sizeof = 8

```cpp
class A { virtual void f(); };
class B { virtual void g(); };
class C : public A, public B {};
// sizeof(void (C::*)()) 可能 = 16（多继承需要 this 调整量）
// 不是 8！
```

### 错误 3：对空成员函数指针的判断

```cpp
void (Widget::*fp)() = nullptr;
if (fp) { /* 非空 */ }
// fp == nullptr 的判断不是简单的指针比较
// 需要检查内部所有字段
```

---

## 和 C 的区别

| 特性 | C 函数指针 | C++ 成员函数指针 |
|------|-----------|-----------------|
| 大小 | sizeof(void*) = 8 | 8-16 字节（取决于继承关系） |
| 调用 | `func(args)` | `(obj.*fp)(args)` 或 `(ptr->*fp)(args)` |
| this 绑定 | 手动传 `self` | 编译器自动绑定 |
| 虚函数 | N/A | 存 vtable 偏移，运行时分派 |
| 多继承 | N/A | 需存 this 调整量 |

---

## HFT 关联

1. **事件系统**：`struct Handler { void (Engine::*cb)(Order&); Engine* obj; }` —— 成员函数指针 + 对象指针实现回调。
2. **避免在热路径用**：成员函数指针比普通函数指针慢（多继承需 this 调整 + 可能的虚分派）。热路径用 `std::function` 或直接函数指针 + void* 上下文。
3. **sizeof 不可忽略**：多继承下成员函数指针 16 字节——比普通指针大一倍，影响事件表的大小。

---

## 代码自测

### Q1: 基本用法

```cpp
class Calculator {
public:
    int add(int a, int b) { return a + b; }
    int mul(int a, int b) { return a * b; }
};
int (Calculator::*op)(int, int) = &Calculator::add;
Calculator calc;
// (calc.*op)(3, 4) 的值？
op = &Calculator::mul;
// (calc.*op)(3, 4) 的值？
```

<details>
<summary>答案与复习指引</summary>

`(calc.*op)(3, 4)` = 7（add）。切换后 `(calc.*op)(3, 4)` = 12（mul）。成员函数指针通过 `.*` 或 `->*` 调用，编译器自动绑定 `this`。

**复习：** → [4.4 指向成员函数的指针](./04-pointer-to-member-func.md)
</details>

### Q2: 虚函数指针

```cpp
class Shape { public: virtual void draw() { cout << "Shape"; } };
class Circle : public Shape {
public: void draw() override { cout << "Circle"; }
};
void (Shape::*fp)() = &Shape::draw;
Shape* s = new Circle;
(s->*fp)();  // 输出什么？
```

<details>
<summary>答案与复习指引</summary>

输出 `Circle`。`fp` 存的是 vtable 偏移（不是函数地址）。`(s->*fp)()` 经 s 的 vptr 查 vtable 找到 `Circle::draw`——表现多态。即使通过成员函数指针调用，虚函数仍然分派。

**复习：** → [4.4 指向成员函数的指针](./04-pointer-to-member-func.md)
</details>

### Q3: sizeof

```cpp
class SingleBase { virtual void f(); };
class MultiBase1 { virtual void f1(); };
class MultiBase2 { virtual void f2(); };
class Multi : public MultiBase1, public MultiBase2 {};
// sizeof(void (SingleBase::*)()) = ?
// sizeof(void (Multi::*)()) = ?
```

<details>
<summary>答案与复习指引</summary>

`sizeof(void (SingleBase::*)())` 通常 = 8（单继承，只需函数地址/vtable 偏移）。`sizeof(void (Multi::*)())` 可能 = 16（多继承，需要函数地址 + this 调整量）。成员函数指针的大小取决于继承关系。

**复习：** → [4.4 指向成员函数的指针](./04-pointer-to-member-func.md)
</details>

### Q4: 事件回调

```cpp
class Engine {
public:
    void onOrder(Order& o) { /* 处理订单 */ }
    void onTick(Tick& t) { /* 处理行情 */ }
};
struct Callback {
    Engine* obj;
    void (Engine::*func)(Order&);
};
// 如何用 Callback 调用 onOrder？
```

<details>
<summary>答案与复习指引</summary>

```cpp
Engine eng;
Callback cb{&eng, &Engine::onOrder};
Order o;
(cb.obj->*cb.func)(o);  // 调用 eng.onOrder(o)
```
这是 C++ 事件系统的常见模式——成员函数指针 + 对象指针实现回调。但在 HFT 热路径上，`std::function` 或直接函数指针 + void* 可能更快（避免成员函数指针的 this 调整开销）。

**复习：** → [4.4 指向成员函数的指针](./04-pointer-to-member-func.md)
</details>

---

## 参考与延伸

- 下一节：[4.5 inline](05-inline.md)
- 回到：[第 4 章 函数语义](README.md)
