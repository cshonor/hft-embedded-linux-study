# 2.1 编译器合成默认构造的四种情况

> 第 2 章 构造函数语义 · 上一节：[本章导读](README.md) · 下一节：[2.2 按位拷贝 vs 逐成员拷贝](02-bitwise-vs-memberwise.md)

## 这节讲什么

你写的类没定义默认构造函数，编译器何时"偷偷"合成一个？答案是四种情况——POD 类不合成。理解这一点能预测哪些类有隐式构造开销、哪些类可以安全 memset。

---

## 为什么要学这个（先建立直觉）

C 程序员习惯了"定义即分配，不管初始化"：

```c
struct PodData { int x; double y; };
struct PodData d;   // 分配了，但 x 和 y 是未初始化的垃圾值
// C 程序员知道要手动初始化：d.x = 0; d.y = 0.0;
```

C++ 的类可以有构造函数——但如果你没写，编译器什么时候"帮你"合成一个？这关系到：

1. **性能**：合成的构造函数有运行时代价（调用成员/基类的构造）
2. **安全性**：POD 不合成构造 = 值未初始化 = 读到垃圾值
3. **可预测性**：知道何时合成，才能控制对象行为

```cpp
struct Pod { int x; double y; };         // 不合成，x/y 是垃圾值
class WithString { std::string s; };      // 合成！调 string 默认构造
class WithVirtual { virtual void f(); };  // 合成！设 vptr
```

---

## 四种情况详解

### 情况 1：类含有默认构造的成员对象

```cpp
class Widget {
    std::string name;   // string 有默认构造 → 编译器合成 Widget 默认构造
                        // 合成的构造调 string::string() 初始化 name
};
Widget w;  // name 自动初始化为空字符串 ""
```

### 情况 2：基类有默认构造

```cpp
class Base { public: Base() { cout << "Base"; } };
class Derived : public Base {
    // 没写默认构造，但 Base 有 → 编译器合成
    // 合成的构造先调 Base::Base()
};
Derived d;  // 输出 "Base"
```

### 情况 3：类有虚函数

```cpp
class Shape {
    virtual void draw();  // 有虚函数 → 编译器合成默认构造
                          // 合成的构造设 vptr 指向 Shape 的 vtable
};
Shape s;  // s.vptr 已设置，可以安全调 draw()
```

### 情况 4：类有虚基类

```cpp
class Base {};
class Derived : virtual public Base {
    // 虚基类 → 编译器合成默认构造
    // 合成的构造设虚基类指针/偏移表
};
```

### 不合成的情况：POD

```cpp
struct POD { int x; double y; };
// 不含成员对象（int/double 没有构造函数）
// 不含虚函数、不继承、无虚基类
// → 编译器不合成默认构造！
POD p;  // p.x 和 p.y 是垃圾值！
```

---

## 常见错误（新手踩坑）

### 错误 1：以为所有类都有默认构造

```cpp
struct Config {
    int port;        // 没写构造 → POD → 不合成
    double timeout;
};
Config cfg;          // port 和 timeout 是垃圾值！
// 修正：struct Config { int port = 8080; double timeout = 1.0; };
//       或 struct Config { Config() : port(8080), timeout(1.0) {} ... };
```

### 错误 2：memset 非 POD 类

```cpp
class Logger {
    std::string name;  // 非 POD
    int level;
};
Logger log;
memset(&log, 0, sizeof(log));  // UB！破坏 string 内部指针
// string 的析构会 double free 或崩溃
```

### 错误 3：在构造函数里依赖虚函数分派

```cpp
class Base {
public:
    Base() { init(); }  // 编译器合成的构造会调 Base::init()
    virtual void init() { cout << "Base init"; }
};
class Derived : public Base {
public:
    void init() override { cout << "Derived init"; }
};
Derived d;  // 输出 "Base init"（不是 "Derived init"）
// 因为 Base() 执行时 vptr 还指向 Base 的 vtable
```

---

## 和 C 的区别

| 特性 | C struct | C++ class（无显式构造） |
|------|----------|----------------------|
| 纯数据（POD） | 不初始化（垃圾值） | 不合成构造（和 C 一样） |
| 含成员对象 | N/A | 合成构造调成员的构造 |
| 有虚函数 | N/A | 合成构造设 vptr |
| 有基类 | N/A | 合成构造调基类构造 |
| 默认初始化 | 无（需手动 memset/赋值） | 非 POD 自动初始化 |

---

## HFT 关联

1. **POD 零构造开销**：POD 类不合成构造，`vector<Tick>` 扩容用 `memcpy` 移动——HFT 数据结构尽量 POD。
2. **POD 可预测 sizeof**：POD 没有 vptr、没有隐式成员，sizeof 完全可预测——cache 行为可优化。
3. **成员默认初始化（C++11）**：`struct Tick { int price = 0; int qty = 0; };` 给 POD 成员默认值，兼顾安全和性能。

---

## 代码自测

### Q1: 判断是否合成构造

```cpp
struct A { int x; };                        // 1
struct B { std::string s; };                 // 2
struct C { virtual void f(); };              // 3
struct D : public A {};                      // 4（A 是 POD）
// 哪些会合成默认构造？
```

<details>
<summary>答案与复习指引</summary>

- A（POD）：不合成
- B（有 string 成员）：合成（调 string 默认构造）
- C（有虚函数）：合成（设 vptr）
- D（基类 A 是 POD，不合成）：不合成

**复习：** → [2.1 合成默认构造](./01-synthesized-default-ctor.md)
</details>

### Q2: POD 初始化

```cpp
struct Tick {
    int price;
    int qty;
};
Tick t;  // t.price 和 t.qty 的值是什么？
```

<details>
<summary>答案与复习指引</summary>

垃圾值（未初始化）。Tick 是 POD，编译器不合成构造。修正：`struct Tick { int price = 0; int qty = 0; };` 或 `Tick t{};`（值初始化）或 `Tick t = {};`。

**复习：** → [2.1 合成默认构造](./01-synthesized-default-ctor.md)
</details>

### Q3: memset 安全性

```cpp
struct A { int x; double y; };              // POD
struct B { std::vector<int> v; int n; };    // 非 POD
// 哪个可以 memset(&obj, 0, sizeof(obj))？
```

<details>
<summary>答案与复习指引</summary>

A 可以（POD，纯数据，memset 安全）。B 不行（vector 有内部指针，memset 破坏后析构崩溃）。判断标准：是否是平凡类型（trivially constructible）。

**复习：** → [2.1 合成默认构造](./01-synthesized-default-ctor.md)
</details>

### Q4: 构造函数调虚函数

```cpp
class Base {
public:
    Base() { setup(); }
    virtual void setup() { printf("Base"); }
};
class Derived : public Base {
public:
    void setup() override { printf("Derived"); }
};
Derived d;  // 输出什么？
```

<details>
<summary>答案与复习指引</summary>

输出 `Base`。`Base()` 执行时 vptr 指向 Base 的 vtable，`setup()` 调的是 `Base::setup()`。构造函数里调虚函数不表现多态。

**复习：** → [2.1 合成默认构造](./01-synthesized-default-ctor.md)
</details>

---

## 参考与延伸

- 下一节：[2.2 按位拷贝 vs 逐成员拷贝](02-bitwise-vs-memberwise.md)
- 回到：[第 2 章 构造函数语义](README.md)
