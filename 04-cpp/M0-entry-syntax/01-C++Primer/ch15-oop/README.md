# 第 15 章 面向对象程序设计（OOP）

本章讲解面向对象编程的核心思想：**数据抽象**、**继承**和**动态绑定**。

## 小节

- [基类与派生类](./15.1-基类与派生类.md)
- [虚函数与动态绑定](./15.2-虚函数与动态绑定.md)
- [抽象基类与访问控制](./15.3-抽象基类与访问控制.md)
- [继承中的拷贝控制与类作用域](./15.4-继承中的拷贝控制与类作用域.md)
- [容器与继承](./15.5-容器与继承.md)


## 章节摘要

面向对象编程核心：数据抽象、继承和动态绑定。包括基类与派生类、虚函数与 `virtual`、抽象基类与纯虚函数、访问控制与继承、继承中的拷贝控制、容器与继承（对象切片）。

### 和 C 的区别

| C | C++ |
|---|-----|
| 无继承 | 单/多/虚继承 |
| 函数指针模拟多态 | `virtual` 虚函数（vtable 自动分派） |
| 无抽象类型 | 纯虚函数 = 抽象基类 |
| 无对象切片 | 值拷贝派生类到基类会切片 |

## 章节自测

### Q1: 虚函数分派

```cpp
class Shape {
public:
    virtual void draw() { std::cout << "Shape "; }
    void print() { std::cout << "Print "; }
};
class Circle : public Shape {
public:
    void draw() override { std::cout << "Circle "; }
    void print() { std::cout << "CirclePrint "; }
};
Shape *s = new Circle;
s->draw();   // A
s->print();  // B
```

> A 和 B 分别输出什么？`virtual` 的作用是什么？

<details>
<summary>答案与复习指引</summary>

**A: `Circle `** — `draw()` 是虚函数，运行时根据对象实际类型（`Circle`）调用
**B: `Print `** — `print()` 不是虚函数，编译时根据指针类型（`Shape*`）调用

**`virtual` 的作用：** 启用动态绑定（运行时分派）。虚函数通过 vptr→vtable 间接调用，代价是多一次间接访存 + 不可内联。

**`override` 的作用：** 让编译器检查是否真的重写了基类虚函数（签名必须匹配），防止"以为重写了其实没有"的 bug。

**复习：** → [虚函数与动态绑定](./15.2-虚函数与动态绑定.md)
</details>

### Q2: 虚析构函数

```cpp
class Base {
public:
    ~Base() {}  // 非虚析构
};
class Derived : public Base {
    int *data;
public:
    Derived() : data(new int[100]) {}
    ~Derived() { delete[] data; }
};
Base *p = new Derived;
delete p;  // 会发生什么？
```

> `delete p` 会发生什么？如何修复？

<details>
<summary>答案与复习指引</summary>

**内存泄漏。** `delete p` 调用 `Base` 的析构（非虚），不调用 `Derived` 的析构 → `data` 泄漏。

**修复：** 基类析构声明为 `virtual`：
```cpp
virtual ~Base() {}
```
这样 `delete p` 先调 `Derived::~Derived()`（释放 `data`），再调 `Base::~Base()`。

**规则：** 如果类可能被继承且通过基类指针删除，析构函数必须是 `virtual`。这是 OOP 的铁律。

**复习：** → [基类与派生类](./15.1-基类与派生类.md)
</details>

### Q3: 对象切片

```cpp
class Animal {
public:
    virtual std::string sound() { return "..."; }
};
class Dog : public Animal {
public:
    std::string sound() override { return "Woof"; }
};
Dog d;
Animal a = d;  // 值拷贝
std::cout << a.sound();
```

> 输出是什么？为什么？如何避免切片？

<details>
<summary>答案与复习指引</summary>

**输出：** `...` — `a.sound()` 调用 `Animal::sound()`，不是 `Dog::sound()`

**原因：** `Animal a = d` 把 `Dog` 对象拷贝到 `Animal` 对象——派生部分被"切掉"了。`a` 是一个纯正的 `Animal` 对象，它的 vptr 指向 `Animal` 的 vtable。

**避免切片：**
1. 通过指针/引用操作多态对象：`Animal &a = d;`
2. 容器存指针：`vector<unique_ptr<Animal>>`
3. 拷贝构造设为 `protected` 或 `=delete`

**复习：** → [容器与继承](./15.5-容器与继承.md)
</details>

### Q4: 抽象基类

```cpp
class Shape {
public:
    virtual double area() const = 0;  // 纯虚函数
    virtual void describe() const { std::cout << "Shape with area " << area(); }
};
// Shape s;  // A: 合法吗？
class Circle : public Shape {
    double r;
public:
    Circle(double radius) : r(radius) {}
    double area() const override { return 3.14159 * r * r; }
};
Circle c(2);
c.describe();  // B: 输出什么？
```

> A 合法吗？B 输出什么？

<details>
<summary>答案与复习指引</summary>

**A: 编译错误。** 含纯虚函数（`= 0`）的类是抽象类，不能实例化。

**B: 输出 `Shape with area 12.5664`**

**解析：** `Circle` 实现了 `area()`，成为具体类可实例化。`describe()` 在 `Shape` 中定义，调用 `area()` 时由于虚函数分派，实际调用 `Circle::area()`。这是"模板方法模式"——基类定义算法骨架，派生类填充细节。

**复习：** → [抽象基类与访问控制](./15.3-抽象基类与访问控制.md)
</details>
