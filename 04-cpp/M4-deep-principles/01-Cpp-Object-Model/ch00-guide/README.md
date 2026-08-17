# 导读

**Guide**

## 本书讲什么

《深度探索 C++ 对象模型》不讲 C++ 语法，讲的是**编译器如何在底层实现 C++ 的面向对象特性**——对象在内存中长什么样、虚函数怎么分派、构造/析构何时被编译器偷偷合成、继承与虚基类的布局代价。这是从"会用 C++"到"理解 C++ 每个特性的代价"的桥梁，也是 HFT 用 C++ 时做性能取舍的依据。

## 为什么对 HFT 重要

C++ 的"零开销抽象"承诺不是无条件的——虚函数有 vtable 间接、异常有表开销、虚基类有 this 调整。理解对象模型，才能在热路径上避开有代价的特性、选零开销的等价写法。本书与 Effective Modern C++（04）配合：04 讲"怎么写现代 C++"，07 讲"这些写法底层发生了什么"。

## 阅读建议

重点章节：ch1（对象模型全景）、ch3（数据语义/布局）、ch4（函数语义/vtable）。ch2（构造语义）和 ch5（生命周期）次之。ch6/ch7（运行时/模板异常）按需。配合编译器实际行为验证（`sizeof`、`offsetof`、`-fdump-lang-class`）。

## 自测题

1. 本书讲的是 C++ 语法还是编译器实现？两者区别是什么？
2. 为什么 HFT 工程师需要理解对象模型？
3. 哪几章是重点？

## 代码自测

### Q1: 对象模型全景判断
```cpp
class Empty {};
class WithMember { int x; };
class WithVirtual { virtual void f() {} };
class WithStatic { static int s; int x; };

int main() {
    std::cout << sizeof(Empty) << ' '
              << sizeof(WithMember) << ' '
              << sizeof(WithVirtual) << ' '
              << sizeof(WithStatic);
}
```
> 四个 sizeof 分别输出什么？（假设 64 位）为什么 Empty 不等于 0？

<details>
<summary>答案与复习指引</summary>

- `sizeof(Empty)` = **1**（空类也至少 1 字节，保证不同对象地址唯一）
- `sizeof(WithMember)` = **4**（一个 int）
- `sizeof(WithVirtual)` = **8**（一个 vptr 指针，64 位）
- `sizeof(WithStatic)` = **4**（静态成员不在对象内，只有 int x）

**关键**：静态成员存于全局/静态区，成员函数存于代码段，都不占对象空间。虚函数引入 vptr 才让对象变大。

**复习：** → [导读：对象模型全景](./README.md)
</details>

### Q2: 为什么 HFT 要学对象模型
```cpp
// 方案 A：虚函数分派
class Strategy { public: virtual void onTick() = 0; };
class FastStrat : public Strategy { public: void onTick() override { /*...*/ } };

// 方案 B：switch 分派
enum class StratType { FAST, SLOW };
void dispatch(StratType t) {
    switch (t) {
        case StratType::FAST: fastOnTick(); break;
        case StratType::SLOW: slowOnTick(); break;
    }
}
```
> 方案 A 的虚函数调用比方案 B 的 switch 多什么运行时代价？HFT 热路径为什么选 B？

<details>
<summary>答案与复习指引</summary>

虚函数调用 = 经 `vptr` → `vtable[slot]` 间接取函数地址 → call。比 switch 多一次间接访存（vtable 查找），可能 cache miss + 分支预测失败。switch 是编译期地址表（jump table），连续内存、cache 友好、可内联。

HFT 热路径每 tick 执行数百万次，虚函数的间接访存引入延迟抖动（cache miss 延迟 ~100 cycles）。switch 静态分派无间接、可内联优化。

**复习：** → [导读：为什么对 HFT 重要](./README.md)
</details>
