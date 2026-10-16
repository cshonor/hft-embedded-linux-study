# 1.2 虚函数与 vptr

> 第 1 章 · 上一节：[1.1 对象模型三规则](01-object-model-rules.md) · 下一节：[1.3 继承布局](03-inheritance-layout.md)

## 这节讲什么

有虚函数的类，对象内多一个 `vptr`（虚表指针）指向 `vtable`（虚函数表）。虚函数调用经 vptr 间接取地址——比普通函数多一次访存、不可内联。这是 C++ 多态的底层代价。

---

## 为什么要学这个（先建立直觉）

C 程序员手动实现"多态"的方式是函数指针：

```c
// C 的"多态"：手动维护函数指针表
struct Shape_C {
    enum Type type;
    void (*draw)(struct Shape_C*);
    double (*area)(struct Shape_C*);
};
// 每个对象都存了函数指针 → 对象膨胀
```

C++ 的虚函数是**编译器自动生成**这张表，对象里只存一个 vptr（指向表），而不是每个函数指针都存：

```cpp
class Shape {
public:
    virtual void draw();
    virtual double area();
};
// sizeof(Shape) = 8（只有 vptr，不是 16）
// vtable 是全局唯一的函数指针数组，对象共享
```

关键区别：C 的方式每个对象存 N 个函数指针（N×8 字节）；C++ 只存 1 个 vptr（8 字节），不管有多少虚函数。

---

## 核心机制详解

### 对象内存布局

```cpp
class Shape {
    virtual void draw();
    virtual double area();
    int color;
};
// 内存布局（64 位）：
// [vptr (8B)] [color (4B)] [padding (4B)]
// sizeof = 16
```

### 虚函数调用流程

```cpp
Shape* s = new Circle;
s->draw();
// 编译器展开为：
// 1. 取 s 的 vptr          → 一次访存（vptr 可能在 cache）
// 2. 查 vtable[0]（draw 的 slot）→ 一次访存（vtable 可能 cache miss）
// 3. 间接 call 该地址       → 分支预测代价 + 不可内联
```

### vptr 在构造时设置

```cpp
class Base {
public:
    Base() {
        // 此时 vptr 指向 Base 的 vtable
        // 如果这里调虚函数 virtual_func()，调的是 Base::virtual_func()
        // 不是 Derived::virtual_func()！
    }
    virtual void virtual_func() {}
};

class Derived : public Base {
public:
    void virtual_func() override {}
    Derived() {
        // Base 构造完后，vptr 被改为指向 Derived 的 vtable
        // 此时调 virtual_func() 调的是 Derived::virtual_func()
    }
};
```

构造时 vptr 的变化：`Base::Base()` 执行时 vptr → Base vtable → 执行完改为 → Derived vtable → `Derived::Derived()` 执行。

---

## 常见错误（新手踩坑）

### 错误 1：构造函数里调虚函数期望多态

```cpp
class Base {
public:
    Base() { init(); }  // 期望调 Derived::init()
    virtual void init() { cout << "Base"; }
};
class Derived : public Base {
public:
    void init() override { cout << "Derived"; }
};
Derived d;  // 输出 "Base"，不是 "Derived"！
// 原因：Base() 执行时 vptr 指向 Base vtable
```

### 错误 2：忘了虚析构函数

```cpp
class Base { public: ~Base() {} };  // 非虚析构
class Derived : public Base { int* data; ~Derived() { delete[] data; } };
Base* p = new Derived;
delete p;  // UB！只调 ~Base()，~Derived() 没调 → 内存泄漏
// 修正：virtual ~Base() {}
```

### 错误 3：以为虚函数和普通函数一样快

```cpp
class Strategy {
public:
    virtual int execute() { return 0; }  // 虚函数
};
// s->execute() 比 if (cond) { return 0; } 慢——
// 多一次间接访存 + 不可内联 + 分支预测代价
```

---

## 和 C 的区别

| 特性 | C 函数指针 | C++ 虚函数 |
|------|-----------|-----------|
| 实现方式 | 手动维护函数指针 | 编译器自动生成 vtable |
| 对象开销 | 每个函数指针 8B（N 个 = N×8B） | 固定 1 个 vptr = 8B |
| 调用代价 | 间接 call（和虚函数一样） | 间接 call（经 vptr→vtable→func） |
| 内联 | 不可能 | 不可能（编译期不知实际调哪个） |
| 类型安全 | 无（void* 传 this） | 有（编译器检查签名） |

---

## HFT 关联

1. **热路径禁虚函数**：vtable 间接在每 tick 路径上引入 cache miss + 分支预测代价 + 不可内联。策略分派用 `enum` + `switch` 或 CRTP 静态分派替代。
2. **vptr 的 cache 影响**：vptr 8 字节让对象变大，可能让一个 cache 行装的对象数减半。`sizeof(Order)` 从 56 变 64（加 vptr），每行从 1 个变 1 个——但从 48 变 56，每行从 1 个变 1 个，白白浪费。
3. **CRTP 零开销多态**：`template<class D> struct Base { void f() { static_cast<D*>(this)->impl(); } };` 编译期分派，无 vptr、可内联。

---

## 代码自测

### Q1: sizeof 推断

```cpp
class A { int x; };                    // 无虚函数
class B { virtual void f(); int x; };  // 有虚函数
// sizeof(A) = ?  sizeof(B) = ?
```

<details>
<summary>答案与复习指引</summary>

`sizeof(A) = 4`（只有 int x）。`sizeof(B) = 16`（vptr 8B + int 4B + padding 4B）。有虚函数的类多一个 vptr。

**复习：** → [1.2 虚函数与 vptr](./02-vptr-vtable.md)
</details>

### Q2: 构造函数调虚函数

```cpp
class Base {
public:
    Base() { log(); }
    virtual void log() { printf("Base"); }
};
class Derived : public Base {
public:
    void log() override { printf("Derived"); }
};
Derived d;  // 输出什么？
```

<details>
<summary>答案与复习指引</summary>

输出 `Base`。构造 `Base()` 时 vptr 指向 Base 的 vtable，所以 `log()` 调的是 `Base::log()`。构造函数里调虚函数不表现多态——这是 C++ 的设计，不是 bug。

**复习：** → [1.2 虚函数与 vptr](./02-vptr-vtable.md)
</details>

### Q3: 虚析构函数

```cpp
class Base { public: virtual ~Base() = default; };
class Derived : public Base {
    int* data = new int[100];
    ~Derived() { delete[] data; }
};
Base* p = new Derived;
delete p;  // 会调 ~Derived() 吗？
```

<details>
<summary>答案与复习指引</summary>

会。因为 `~Base()` 是 virtual，`delete p` 经 vtable 调用，先调 `~Derived()`（释放 data），再调 `~Base()`。如果 `~Base()` 不是 virtual，则只调 `~Base()`，`data` 泄漏。

**复习：** → [1.2 虚函数与 vptr](./02-vptr-vtable.md)
</details>

### Q4: vtable 调用流程

```cpp
class Shape { public: virtual void draw() = 0; virtual double area() = 0; };
class Circle : public Shape {
    double r;
public:
    void draw() override {}
    double area() override { return 3.14 * r * r; }
};
Shape* s = new Circle{1.0};
s->area();  // 编译器如何分派？
```

<details>
<summary>答案与复习指引</summary>

流程：①取 `s` 的 vptr（指向 Circle 的 vtable）→ ②查 `vtable[area 的 slot]`（得到 `Circle::area` 的地址）→ ③间接 call 该地址。比直接 `call Circle::area` 多两次间接访存。

**复习：** → [1.2 虚函数与 vptr](./02-vptr-vtable.md)
</details>

---

## 参考与延伸

- 下一节：[1.3 继承布局](03-inheritance-layout.md)
- 回到：[第 1 章 关于对象](README.md)
