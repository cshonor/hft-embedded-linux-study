# 6.2 RTTI（运行时类型识别）

> 第 6 章 · 上一节：[6.1 new/delete 链路](01-new-delete-chain.md) · 下一节：[6.3 异常处理开销](03-exception-cost.md)

## 这节讲什么

`dynamic_cast` 和 `typeid` 的运行时代价——查类型信息表 + 字符串比较。RTTI 只对多态类型（有虚函数）有效。HFT 热路径禁用 RTTI。

---

## 为什么要学这个（先建立直觉）

C 程序员用 enum 标签手动跟踪类型：

```c
// C：手动类型识别
enum Type { TYPE_CIRCLE, TYPE_SQUARE };
struct Shape {
    enum Type type;
    // ...
};
if (shape->type == TYPE_CIRCLE) {
    Circle* c = (Circle*)shape;  // 手动转型
}
// 编译期完成，零运行时代价
```

C++ 的 RTTI 是运行时类型识别——但不是免费的：

```cpp
class Shape { public: virtual ~Shape() {} };
class Circle : public Shape {};
Shape* s = new Circle;
// dynamic_cast 检查 s 的真实类型
Circle* c = dynamic_cast<Circle*>(s);  // 运行时查 type_info 表
// 代价：查 vtable → type_info → 类名比较 → 可能遍历继承链
// 比 C 的 enum + static_cast 慢得多
```

---

## 两种 RTTI 详解

### dynamic_cast

```cpp
class Base { public: virtual ~Base() {} };
class Derived : public Base { public: void special() {} };
Base* b = new Derived;

// 向下转型
Derived* d = dynamic_cast<Derived*>(b);
if (d) d->special();  // 成功：d 非空

// 代价：查 type_info 表 + 遍历继承链 + 类名比较
// 失败：指针返回 nullptr，引用抛 bad_cast

// 对非多态类型无效
struct POD { int x; };
POD* p = ...;
dynamic_cast<POD*>(p);  // 编译错误：POD 不是多态类型
```

### typeid

```cpp
#include <typeinfo>
Base* b = new Derived;
cout << typeid(*b).name();  // 输出 mangled name（如 "7Derived"）
// 代价：查 type_info 表

// 比较
if (typeid(*b) == typeid(Derived)) {
    // 类型匹配
}
```

### RTTI 的实现

```
vtable:
[type_info* | &Base::f | &Base::g | ...]
  ↑ type_info 存类名、父类信息

dynamic_cast 流程：
1. 取 b 的 vptr
2. 查 vtable[0] 获取 type_info
3. 比较 type_info 的类名（字符串比较）
4. 如果不匹配，遍历继承链查找
5. 找到 → 返回调整后的指针；没找到 → nullptr
```

---

## 常见错误（新手踩坑）

### 错误 1：热路径用 dynamic_cast

```cpp
void process(Base* b) {
    Derived* d = dynamic_cast<Derived*>(b);
    if (d) d->hotPath();
    // dynamic_cast 的查表 + 字符串比较在每 tick 路径上
    // 可能浪费 20-100ns
}
// 修正：用 enum 标签 + static_cast
```

### 错误 2：对非多态类型用 RTTI

```cpp
struct POD { int x; };
POD p;
dynamic_cast<POD*>(&p);  // 编译错误：POD 没有虚函数
typeid(POD).name();      // OK：typeid 对类型名可以用（静态 RTTI）
typeid(p).name();        // OK：但返回静态类型信息（POD），不是动态
```

### 错误 3：忘了 -fno-rtti 的影响

```cpp
// 编译时 -fno-rtti
dynamic_cast<Derived*>(b);  // 编译错误！RTTI 被禁用
typeid(*b).name();          // 编译错误！
// 但 typeid(TypeName) 静态使用可能仍可用
```

---

## 和 C 的区别

| 特性 | C enum 标签 | C++ RTTI |
|------|-----------|----------|
| 实现 | 手动维护 enum | 编译器自动生成 type_info |
| 运行时代价 | 零（编译期比较） | 查表 + 字符串比较 |
| 类型安全 | 无（可任意 cast） | 有（dynamic_cast 检查） |
| 适用范围 | 所有类型 | 仅多态类型（有虚函数） |
| 可关闭 | N/A | `-fno-rtti` |

---

## HFT 关联

1. **禁 dynamic_cast 热路径**：RTTI 的类型表查找有 cache miss + 字符串比较代价。用 `enum` 标签 + `static_cast` 替代（编译期保证安全）。
2. **`-fno-rtti`**：部分 HFT 引擎整体关 RTTI，减小二进制 + 略加速。但会失去 dynamic_cast 和 typeid。
3. **visitor 模式替代**：用 visitor 模式实现类型分派——编译期确定，无运行时代价。

---

## 代码自测

### Q1: dynamic_cast

```cpp
class Base { public: virtual ~Base() {} };
class Derived : public Base { public: void special() {} };
Base* b = new Base;
Derived* d = dynamic_cast<Derived*>(b);
// d 的值是什么？
```

<details>
<summary>答案与复习指引</summary>

`d == nullptr`。`b` 的真实类型是 `Base`，不是 `Derived`——`dynamic_cast` 检查后发现类型不匹配，返回 nullptr。如果 `b` 指向 `Derived` 对象则返回非空指针。

**复习：** → [6.2 RTTI](./02-rtti.md)
</details>

### Q2: RTTI 代价

```cpp
// 方案 A：dynamic_cast
void process(Base* b) {
    Derived* d = dynamic_cast<Derived*>(b);
    if (d) d->fastPath();
}

// 方案 B：enum 标签
void process(Base* b) {
    if (b->type == TYPE_DERIVED) {
        static_cast<Derived*>(b)->fastPath();
    }
}
// 哪个更快？为什么？
```

<details>
<summary>答案与复习指引</summary>

方案 B 更快。方案 A 的 `dynamic_cast` 需查 type_info 表 + 字符串比较 + 可能遍历继承链——可能 cache miss。方案 B 的 enum 比较是整数比较（1 条指令），`static_cast` 零运行时代价。HFT 热路径用方案 B。

**复习：** → [6.2 RTTI](./02-rtti.md)
</details>

### Q3: 非多态类型

```cpp
struct Data { int x; double y; };
Data d;
typeid(d).name();           // A：能编译吗？
dynamic_cast<Data*>(&d);     // B：能编译吗？
```

<details>
<summary>答案与复习指引</summary>

A 可以（typeid 对任何表达式都可用，返回静态类型信息）。B 不能编译——`dynamic_cast` 只对多态类型（有虚函数）有效，Data 没有虚函数。RTTI 的运行时部分依赖 vtable，非多态类型没有 vtable。

**复习：** → [6.2 RTTI](./02-rtti.md)
</details>

### Q4: -fno-rtti

```cpp
// 编译时 -fno-rtti
class Base { public: virtual ~Base() {} };
class Derived : public Base {};
Base* b = new Derived;
// 以下哪些可用？
// A: typeid(*b).name()
// B: dynamic_cast<Derived*>(b)
// C: typeid(Base).name()
```

<details>
<summary>答案与复习指引</summary>

A 和 B 不可用（需要 RTTI 运行时信息，-fno-rtti 移除了）。C 可能可用（typeid 对类型名的静态使用不依赖运行时 RTTI，但行为因编译器而异）。`-fno-rtti` 移除 vtable 中的 type_info 指针，dynamic_cast 和 typeid 对多态对象不可用。HFT 用 `-fno-rtti` 减小二进制 + 略加速。

**复习：** → [6.2 RTTI](./02-rtti.md)
</details>

---

## 参考与延伸

- 下一节：[6.3 异常处理开销](03-exception-cost.md)
- 回到：[第 6 章 运行时语义](README.md)
