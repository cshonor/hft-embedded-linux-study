# 1.1 对象模型的三个规则

> 第 1 章 关于对象 · 上一节：[本章导读](README.md) · 下一节：[1.2 虚函数与 vptr](02-vptr-vtable.md)

## 这节讲什么

C++ 对象在内存里到底长什么样？三条规则决定了 `sizeof` 的结果——哪些在对象内，哪些不在。这是理解后续所有对象布局（vptr、继承、对齐）的基础。

---

## 为什么要学这个（先建立直觉）

C 程序员对 `struct` 的心智模型很简单：**结构体就是数据的拼接，sizeof 等于各成员加 padding**。C++ 的 class 打破了这个模型——它可以有函数、有静态成员、有虚函数——但对象里到底存了什么？

```c
// C 的心智模型：struct = 纯数据
struct Point_C {
    int x, y;          // sizeof = 8，就这
};
```

```cpp
// C++ 的 class：有数据、有函数、有静态成员
class Point {
    int x, y;          // 在对象内
    static int count;  // 在对象外（全局/静态区）
    void draw();       // 在对象外（代码段）
};
// sizeof(Point) = 8 —— 和 C 的 struct 一样！
```

关键洞察：**C++ 的对象只存数据，不存函数。** 函数编译后放在代码段，所有对象共享一份。这和 C 的心智模型其实一致——区别在于 C++ 编译器偷偷加东西（vptr）。

---

## 三条规则详解

### 规则 1：非静态数据成员 → 在对象内部

```cpp
class Point {
    int x, y;          // 每个对象各有一份
};
// sizeof(Point) = 8（两个 int）
```

### 规则 2：静态数据成员 → 在对象外部

```cpp
class Point {
    int x, y;
    static int count;  // 全局唯一一份，不在任何对象里
};
// sizeof(Point) = 8（不含 count！）
// Point::count 存在 .bss/.data 段
```

### 规则 3：成员函数（静态/非静态）→ 在对象外部

```cpp
class Point {
    int x, y;
    void draw();       // 编译后放在代码段
    static int total(); // 同上
};
// sizeof(Point) = 8（不含函数！）
```

函数的"this"是怎么绑定的？编译器把 `void Point::draw()` 编译成类似 `void Point_draw(Point* this)`——this 是隐式参数，不是存在对象里的。

---

## 常见错误（新手踩坑）

### 错误 1：以为静态成员占对象空间

```cpp
class Counter {
    int id;
    static int total;  // 新手以为 sizeof 包含 total
};
// 实际 sizeof(Counter) = 4（只有 id）
// total 在全局区，所有 Counter 对象共享一份
```

### 错误 2：以为成员函数指针存在对象里

```cpp
class Widget {
    int data;
    void process();   // 新手以为每个对象存了 process 的地址
};
// 实际 sizeof(Widget) = 4（只有 data）
// process 的地址在编译期就确定了，不需要存对象里
```

### 错误 3：memset 一个非 POD 类

```cpp
class Logger {
    std::string name;  // 非 POD 成员
    int level;
};
Logger log;
memset(&log, 0, sizeof(log));  // UB！破坏 string 内部状态
// POD（纯数据）可以 memset，非 POD 不行
```

---

## 和 C 的区别

| 特性 | C struct | C++ class |
|------|----------|-----------|
| 数据成员 | 在对象内 | 在对象内（相同） |
| 静态成员 | 无（用全局变量模拟） | 在对象外，全局唯一 |
| 成员函数 | 无（用函数指针模拟） | 在对象外（代码段），对象不存 |
| 空结构体 sizeof | C 允许 0（GCC 扩展） | **C++ 保证 ≥ 1**（空类 sizeof=1） |
| 隐式开销 | 无 | vptr（仅有虚函数时） |

**空类 sizeof=1 的原因**：`class Empty {};` 的两个对象 `a` 和 `b` 必须地址不同（`&a != &b`），所以编译器给空对象分配 1 字节占位。

---

## HFT 关联

1. **sizeof 影响 cache 行利用率**：64 字节 cache 行能装 `64 / sizeof(Order)` 个对象。如果 Order 有 vptr（8 字节），可能让每行对象数减半。
2. **POD 优先**：POD 类型 sizeof 可预测、可 memset、可 memcpy——HFT 热路径数据结构尽量用 POD。
3. **static 成员做全局计数器**：`static uint64_t g_orderCount` 不占对象空间，所有线程可见（需 atomic）。

---

## 代码自测

### Q1: sizeof 推断

```cpp
class Widget {
    int a, b;               // 8 字节
    static int counter;     // ?
    void process();         // ?
    static int getCount();  // ?
};
// sizeof(Widget) = ?
```

<details>
<summary>答案与复习指引</summary>

`sizeof(Widget) = 8`。只有非静态数据成员 `a` 和 `b` 在对象内。`counter` 是静态成员（全局区），`process` 和 `getCount` 是函数（代码段），都不占对象空间。

**复习：** → [1.1 三条规则](./01-object-model-rules.md)
</details>

### Q2: 空类 sizeof

```cpp
class Empty {};
class Holder {
    Empty e;
    int x;
};
// sizeof(Empty) = ?
// sizeof(Holder) = ?
```

<details>
<summary>答案与复习指引</summary>

`sizeof(Empty) = 1`（空类保证 ≥1，让不同对象地址不同）。
`sizeof(Holder) = 8`（Empty 占 1 字节 + 3 字节 padding + int 4 字节 = 8）。空成员也会引起 padding。

**复习：** → [1.1 三条规则](./01-object-model-rules.md)
</details>

### Q3: memset 安全性

```cpp
struct PodData { int x; double y; };        // A
struct NonPod { std::string name; int x; }; // B
// 哪个可以安全 memset(&obj, 0, sizeof(obj))？
```

<details>
<summary>答案与复习指引</summary>

A（POD）可以安全 memset——纯数据，无内部状态。B（非 POD）不行——`std::string` 内部有指针，memset 会破坏其状态导致 UB（双重释放或崩溃）。

**复习：** → [1.1 三条规则](./01-object-model-rules.md)
</details>

### Q4: C 对比

```c
// C 版本
struct Counter_C {
    int id;
    /* 没有 static，用全局变量 */
};
int g_counter;  // 模拟 static 成员
```
```cpp
// C++ 版本
class Counter {
    int id;
    static int counter;  // 类作用域内，更安全
};
```
> 两种方式的 sizeof 分别是多少？C++ static 成员比 C 全局变量好在哪里？

<details>
<summary>答案与复习指引</summary>

两种 sizeof 都 = 4（只有 `id`）。C++ static 成员的优势：作用域受限（private static 只类内可见），不会污染全局命名空间；且可以控制访问权限。

**复习：** → [1.1 三条规则](./01-object-model-rules.md)
</details>

---

## 参考与延伸

- 下一节：[1.2 虚函数与 vptr](02-vptr-vtable.md)
- 回到：[第 1 章 关于对象](README.md)
