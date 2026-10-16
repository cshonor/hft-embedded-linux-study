# 3.4 指向数据成员的指针

> 第 3 章 · 上一节：[3.3 sizeof 的真相](03-sizeof-truth.md) · 下一章：[第 4 章 函数语义](../ch04-function-semantics/README.md)

## 这节讲什么

`int Point::* p = &Point::x;` 是什么？它不是真实地址，而是**偏移量**——在具体对象上定位成员。这是 C++ 独有的类型安全偏移量机制。

---

## 为什么要学这个（先建立直觉）

C 程序员用 `offsetof` 宏获取成员偏移量：

```c
#include <stddef.h>
struct Point_C { int x; int y; };
size_t offset = offsetof(struct Point_C, x);  // offset = 0
// 通过偏移量访问：*(int*)((char*)&obj + offset) = 42;
```

C++ 的指向成员的指针是类型安全的偏移量：

```cpp
class Point { public: int x; int y; };
int Point::* px = &Point::x;  // 类型安全：编译器检查类型
Point p{10, 20};
p.*px = 42;  // p.x = 42
// 等价于 *(int*)((char*)&p + offset) = 42，但类型安全
```

区别：C 的 `offsetof` 返回 `size_t`（无类型信息），C++ 的 `Point::*` 携带类型信息——不能把 `int Point::*` 赋给 `double Point::*`。

---

## 核心概念详解

### 基本用法

```cpp
class Widget {
public:
    int x;
    int y;
    double score;
};

int Widget::* pi = &Widget::x;           // 指向 int 成员的指针
double Widget::* pd = &Widget::score;    // 指向 double 成员的指针

Widget w;
w.*pi = 42;       // w.x = 42
w.*pd = 3.14;     // w.score = 3.14

Widget* wp = &w;
wp->*pi = 100;    // wp->x = 100
```

### 本质是偏移量

```cpp
// Point::* 的值是偏移量，不是地址
int Point::* px = &Point::x;  // px 的值 = 0（x 在对象起始）
int Point::* py = &Point::y;  // py 的值 = 4（y 在 offset 4）
// obj.*px 等价于 *(int*)((char*)&obj + 偏移量)
```

### 虚继承下的复杂偏移

```cpp
class Base { public: int data; };
class Derived : virtual public Base { int y; };
int Base::* pd = &Base::data;
// 虚继承下偏移量不是固定值——需经虚基类偏移表定位
// sizeof(Base::*) 可能 > sizeof(void*)（需存额外信息）
```

---

## 常见错误（新手踩坑）

### 错误 1：混淆 Point::* 和普通指针

```cpp
class Point { public: int x; };
int Point::* pm = &Point::x;  // 指向成员的指针（偏移量）
int* p = &Point::x;           // 编译错误！&Point::x 不是普通地址
// pm 不是地址，是偏移量——必须绑定到对象才能用
```

### 错误 2：空指针检查

```cpp
int Point::* pm = nullptr;
Point p{42};
if (pm) {
    p.*pm = 10;  // 检查 pm 非空
}
// pm == nullptr 时值通常是 -1（表示无效偏移量）
```

### 错误 3：虚继承下的 sizeof

```cpp
class Base { int x; };
class Normal : public Base { int y; };
class Virtual : virtual public Base { int y; };
// sizeof(int Base::*) 在 Normal 下 = sizeof(size_t)
// 在 Virtual 下可能更大（需存虚基类偏移信息）
```

---

## 和 C 的区别

| 特性 | C offsetof | C++ Point::* |
|------|-----------|-------------|
| 类型 | size_t（无类型） | `T Class::*`（类型安全） |
| 用法 | `*(T*)((char*)&obj + offset)` | `obj.*pm` 或 `obj->*pm` |
| 虚继承 | N/A | 需额外信息，sizeof 可能更大 |
| 空值 | offset = (size_t)-1 | nullptr |
| 安全性 | 无（可任意 cast） | 编译期类型检查 |

---

## HFT 关联

1. **序列化/反射**：`offsetof` 或指向成员的指针用于按偏移量批量访问字段——协议解析器常用。
2. **批量字段映射**：`struct FieldMap { const char* name; int Tick::* offset; }` —— 用名字查找偏移量，批量解析行情字段。
3. **避免虚继承**：虚继承下 `Point::*` 的 sizeof 增大，访问变慢——HFT 不用虚继承。

---

## 代码自测

### Q1: 基本用法

```cpp
class Point { public: int x; int y; };
int Point::* px = &Point::x;
int Point::* py = &Point::y;
Point p{10, 20};
p.*px = 100;
// p.x 和 p.y 的值？
```

<details>
<summary>答案与复习指引</summary>

`p.x = 100`（被 `p.*px = 100` 修改），`p.y = 20`（未修改）。`px` 是指向 `Point::x` 的指针（偏移量 0），`py` 是指向 `Point::y` 的指针（偏移量 4）。

**复习：** → [3.4 指向数据成员的指针](./04-pointer-to-member.md)
</details>

### Q2: 本质

```cpp
class Widget { char c; int x; };
int Widget::* pw = &Widget::x;
// pw 的值是多少？它是什么？
```

<details>
<summary>答案与复习指引</summary>

`pw` 的值 = 4（x 的偏移量，c 1B + padding 3B）。它不是地址，是偏移量——`obj.*pw` 等价于 `*(int*)((char*)&obj + 4)`。

**复习：** → [3.4 指向数据成员的指针](./04-pointer-to-member.md)
</details>

### Q3: 类型安全

```cpp
class Point { public: int x; double y; };
int Point::* pi = &Point::x;
double Point::* pd = &Point::y;
// pi = pd;  // 能编译吗？
```

<details>
<summary>答案与复习指引</summary>

不能编译。`pi` 是 `int Point::*`，`pd` 是 `double Point::*`——类型不同。C++ 的指向成员的指针是类型安全的，不像 C 的 `offsetof` 返回无类型 `size_t`。

**复习：** → [3.4 指向数据成员的指针](./04-pointer-to-member.md)
</details>

### Q4: 批量字段映射

```cpp
struct Tick { int price; int qty; long ts; };
struct FieldMap { const char* name; int Tick::* ptr; };
FieldMap maps[] = {
    {"price", &Tick::price},
    {"qty", &Tick::qty},
};
Tick t{100, 5, 12345};
// 如何用 maps 批量访问 t 的字段？
```

<details>
<summary>答案与复习指引</summary>

```cpp
for (auto& m : maps) {
    printf("%s = %d\n", m.name, t.*m.ptr);
}
```
输出 `price = 100` 和 `qty = 5`。这就是协议解析器的常见模式——用偏移量表批量映射字段名到值。

**复习：** → [3.4 指向数据成员的指针](./04-pointer-to-member.md)
</details>

---

## 参考与延伸

- 下一章：[第 4 章 函数语义](../ch04-function-semantics/README.md)
- 回到：[第 3 章 数据语义](README.md)
