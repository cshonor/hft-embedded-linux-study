# 4.5 inline

> 第 4 章 · 上一节：[4.4 指向成员函数的指针](04-pointer-to-member-func.md) · 下一章：[第 5 章 构造、析构与拷贝](../ch05-construction-destruction-copy/README.md)

## 这节讲什么

`inline` 是对编译器的建议。内联后函数体展开，省 call/ret 开销 + 开启跨函数优化。但过度内联增大代码段（I-cache 压力）。虚函数通常不内联（间接调用）。CRTP + inline 实现零开销多态。

---

## 为什么要学这个（先建立直觉）

C 程序员用宏或手动展开消除函数调用开销：

```c
// C：用宏"内联"
#define MAX(a, b) ((a) > (b) ? (a) : (b))
// 缺点：无类型检查、参数多次求值、作用域问题

// 或手动展开（代码重复）
int x = a > b ? a : b;  // 手写
```

C++ 的 inline 是类型安全的"宏展开"：

```cpp
inline int max(int a, int b) { return a > b ? a : b; }
// 类型安全 + 编译器决定是否展开
int x = max(3, 4);  // 可能被内联为：int x = 3 > 4 ? 3 : 4;
```

关键区别：**inline 只是建议，编译器自己决定。** 现代编译器在 `-O2` 下会自动内联小函数，即使没写 `inline`。

---

## 收益与代价详解

### 收益

```cpp
inline int square(int x) { return x * x; }
int y = square(5) + square(3);
// 内联后：int y = 5*5 + 3*3 = 34;
// 收益：
// 1. 省 call/ret 指令（约 2-5 周期）
// 2. 开启跨函数优化（常量传播：5*5 → 25）
// 3. 参数已知 → 更激进的优化
```

### 代价

```cpp
// 过度内联 → 代码段膨胀
inline void bigFunction() {
    // 100 行代码
}
// 如果在 10 处调用 → 代码段增加 10 倍
// I-cache miss 增多 → 整体变慢
```

### 编译器决策

```cpp
// 编译器基于成本模型决定：
// - 函数体大小（太小 → 内联，太大 → 不内联）
// - 调用频率（热路径 → 优先内联）
// - 是否递归（递归通常不内联）
// - 是否通过虚函数调用（虚函数不内联）
```

---

## 常见错误（新手踩坑）

### 错误 1：以为写了 inline 就一定内联

```cpp
inline void bigFunc() {
    // 200 行代码
    // 编译器通常不内联（太大）
}
// inline 只是建议，不是命令
```

### 错误 2：虚函数不内联

```cpp
class Shape {
public:
    virtual double area() { return 0; }  // 虚函数
};
// 即使写了 inline virtual double area()，
// 通过 Shape* 调用也不内联（间接 call）
```

### 错误 3：头文件定义导致多重定义

```cpp
// header.h
void func() { /* 定义在头文件 */ }
// 多个 .cpp 包含 → 链接错误（多重定义）
// 修正：inline void func() { } 或在 .cpp 中定义
```

---

## 和 C 的区别

| 特性 | C 宏 | C++ inline |
|------|------|-----------|
| 类型安全 | 无 | 有 |
| 参数求值 | 可能多次 | 一次 |
| 作用域 | 全局 | 类作用域 |
| 调试 | 难（展开后看不到） | 可调试（编译器可保留调用信息） |
| 编译器决定 | N/A（宏总是展开） | 编译器基于成本模型决定 |

---

## HFT 关联

1. **热路径小函数内联**：`inline int64_t now_ns()` 省 call 开销（2-5ns），在每 tick 路径上显著。
2. **避免过度内联**：大函数内联撑爆 I-cache。用 `__attribute__((noinline))` 阻止特定函数内联。
3. **CRTP + inline = 零开销多态**：`template<class D> struct Base { inline void f() { static_cast<D*>(this)->impl(); } };` 编译期内联，无虚函数开销。
4. **PGO 引导内联**：用 Profile-Guided Optimization 让编译器基于实际调用频率决定内联——比手动 inline 更精准。

---

## 代码自测

### Q1: 内联效果

```cpp
inline int square(int x) { return x * x; }
int result = square(5) + square(3);
// 内联后等价于什么？有什么额外优化？
```

<details>
<summary>答案与复习指引</summary>

内联后：`int result = 5*5 + 3*3;` 编译器可进一步常量传播：`int result = 34;`——完全在编译期计算，运行时零开销。这就是内联的最大价值——开启跨函数优化。

**复习：** → [4.5 inline](./05-inline.md)
</details>

### Q2: 虚函数不内联

```cpp
class Base {
public:
    virtual void process() { /* 默认实现 */ }  // 虚函数
};
Base* b = getBase();  // 运行时才知道类型
b->process();  // 能内联吗？为什么？
```

<details>
<summary>答案与复习指引</summary>

不能内联。`b` 指向的类型在运行时才确定，编译器在编译期不知道调 `Base::process` 还是 `Derived::process`。虚函数经 vtable 间接 call，无法内联。用 CRTP 可实现编译期分派 + 内联。

**复习：** → [4.5 inline](./05-inline.md)
</details>

### Q3: 头文件定义

```cpp
// math_utils.h
int add(int a, int b) { return a + b; }  // 没有 inline
// 如果两个 .cpp 文件都包含此头文件，会发生什么？
```

<details>
<summary>答案与复习指引</summary>

链接错误（多重定义）。两个 .cpp 各有一份 `add` 的定义，链接器不知道用哪个。修正：`inline int add(int a, int b) { return a + b; }`——`inline` 允许多重定义，链接器合并为一份。或把定义放在 .cpp 文件中。

**复习：** → [4.5 inline](./05-inline.md)
</details>

### Q4: CRTP 零开销

```cpp
template<class Derived>
struct Strategy {
    inline int execute() {
        return static_cast<Derived*>(this)->compute();
    }
};
struct FastStrategy : Strategy<FastStrategy> {
    inline int compute() { return 42; }
};
FastStrategy s;
s.execute();  // 有虚函数开销吗？能内联吗？
```

<details>
<summary>答案与复习指引</summary>

无虚函数开销，可以内联。CRTP 在编译期确定类型——`static_cast<FastStrategy*>(this)->compute()` 是直接 call，编译器可以完全内联为 `return 42;`。零开销多态——比虚函数快，但牺牲了运行时多态（类型在编译期固定）。

**复习：** → [4.5 inline](./05-inline.md)
</details>

---

## 参考与延伸

- 下一章：[第 5 章 构造、析构与拷贝](../ch05-construction-destruction-copy/README.md)
- 回到：[第 4 章 函数语义](README.md)
