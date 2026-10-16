# 第 4 章 函数语义

**The Semantics of Functions**

## 本章讲什么

虚函数如何分派？多重继承下虚函数的 `this` 如何调整？指向成员函数的指针是什么？`inline` 的真实代价与收益？本章讲函数调用在对象模型层面的实现——这是理解 C++ 运行时开销的核心。

## 要点

### 虚函数分派（vtable）

```cpp
shape->draw();   // 经 vptr→vtable[slot]→call，间接调用
```
虚函数调用 = 取 `vptr` → 查 `vtable[slot]` → 间接 call。比直接 call 多一次访存（vtable 可能 cache miss）+ 一次间接跳转（分支预测代价）。**无法内联**（编译期不知实际调哪个）。

### 多重继承的 vtable 与 this 调整

多重继承下，派生类有多个 vptr（每个基类一个）。调非首基类的虚函数时，`this` 要调整到对应基类子对象的开头——`thunk` 技术在 vtable 里插入调整代码。

### 虚基类下的虚函数

虚基类让 `this` 调整更复杂（经虚基类偏移表），虚函数调用代价更高。

### 指向成员函数的指针

```cpp
void (Shape::*fp)() = &Shape::draw;
(shape->*fp)();
```
指向成员函数的指针不是普通函数指针——对虚函数它是 vtable 偏移，对非虚函数它是函数地址 + `this` 调整信息。大小通常是 2 个指针（vs 普通函数指针 1 个）。

### inline

`inline` 是对编译器的**建议**。内联后函数体展开，省 call/ret 开销 + 开启跨函数优化。但过度内联增大代码段（I-cache 压力）。编译器自行权衡（基于成本模型）。`virtual` 函数通常不内联（间接调用）。

## HFT 关联

- **热路径禁虚函数**：虚函数的 vtable 间接 + 不可内联是 HFT 性能大忌。策略分派用 `switch`/模板/CRTP（静态多态）替代。
- **CRTP 静态多态**：`template<class D> struct Base { void f() { static_cast<D*>(this)->impl(); } };` 编译期分派，零虚函数开销 + 可内联——HFT 策略基类常用。
- **内联与 I-cache**：热路径小函数内联省 call 开销，但过度内联撑爆 I-cache。用 `__attribute__((flatten))` / PGO 引导内联决策。

## 自测题

1. 虚函数调用比普通函数多哪些代价？为什么不能内联？
2. 多重继承下调非首基类虚函数，`this` 如何调整？thunk 是什么？
3. 指向成员函数的指针为什么比普通函数指针大？
4. CRTP 如何实现零开销的多态？与虚函数相比有什么优势？
5. 过度 inline 有什么代价？HFT 如何权衡？

## 代码自测

### Q1: 虚函数调用的间接性
```cpp
class Base { public: virtual void f() { std::puts("Base"); } };
class Derived : public Base { public: void f() override { std::puts("Derived"); } };

void callDirect(Base b) { b.f(); }    // 按值传递
void callVirtual(Base& b) { b.f(); }  // 按引用传递
```
> `Derived d; callDirect(d);` 和 `callVirtual(d);` 分别输出什么？为什么？

<details>
<summary>答案与复习指引</summary>

- `callDirect(d)` 输出 **"Base"**——对象切片：按值传递拷贝到 `Base b`，vptr 被切回 Base 的 vtable。
- `callVirtual(d)` 输出 **"Derived"**——引用不拷贝对象，vptr 仍是 Derived 的，虚函数正确分派。

**关键**：虚函数通过 vptr→vtable 间接调用，但按值传参会发生对象切片，vptr 被覆盖为基类的。多态必须用指针或引用。

**复习：** → [虚函数调用机制](./README.md)
</details>

### Q2: 静态绑定 vs 动态绑定
```cpp
class Base {
public:
    void show() { std::puts("Base::show"); }         // 非虚
    virtual void display() { std::puts("Base::display"); }  // 虚
};
class Derived : public Base {
public:
    void show() { std::puts("Derived::show"); }
    void display() override { std::puts("Derived::display"); }
};

Base* p = new Derived;
p->show();
p->display();
```
> 两行分别输出什么？非虚函数和虚函数的绑定时机有何不同？

<details>
<summary>答案与复习指引</summary>

- `p->show()` 输出 **"Base::show"**——非虚函数静态绑定，编译期根据指针类型（Base*）决定。
- `p->display()` 输出 **"Derived::display"**——虚函数动态绑定，运行期通过 vptr→vtable 查找实际函数。

**静态绑定**：编译期确定，直接 call（无间接，可内联）。**动态绑定**：运行期经 vptr 间接查找（有额外开销，不可内联）。

**复习：** → [静态绑定 vs 动态绑定](./README.md)
</details>
