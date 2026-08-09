# Item 12：把重写函数声明为 override

> 第 3 章 移步现代 C++ · Item 12 · 上一节：[Item 11 =default](item11-default.md)

## 为什么要学这个（先建立直觉）

C 没有继承和虚函数，但 C 程序员理解"函数指针表"（vtable 的 C 模拟）：

```c
// C 的"多态"模拟
struct Strategy {
    void (*on_tick)(void* self, const Tick* t);
    void (*on_order)(void* self, const Order* o);
};
```

C++ 的虚函数就是这个模式的语言级支持。但 C++ 有一个 C 没有的坑：**你以为重写了基类虚函数，实际可能没有**。

```cpp
class Base {
public:
    virtual void on_tick(int x) {}  // 注意参数是 int
};

class Derived : public Base {
public:
    virtual void on_tick(double x) {}  // 参数是 double——签名不匹配！
    // 这不是重写，是创建了一个新虚函数！Base::on_tick 没被覆盖
};
```

C++11 之前，编译器不会告诉你这个错误——代码能编译，但运行时多态不生效。C++11 的 `override` 关键字让编译器**检查**是否真的重写了：

```cpp
class Derived : public Base {
public:
    virtual void on_tick(double x) override;  // 编译报错！签名不匹配
};
```

---

## 这节讲什么

`override` 关键字让编译器**检查**是否真的重写了基类虚函数——签名不匹配会编译报错，而非静默创建一个新虚函数。这是 C++11 最具性价比的防 bug 特性之一。

---

## 核心问题

### 签名不匹配的隐蔽 bug

```cpp
class Base {
public:
    virtual void doWork(int x);       // 基类虚函数
};

class Derived : public Base {
public:
    // 没有 override：编译器不检查，静默创建新虚函数
    virtual void doWork(double x);    // 签名不匹配！Base::doWork 没被重写
    // 运行时：Base* p = &derived; p->doWork(42); 调的是 Base::doWork，不是 Derived！
};

// 加 override：编译器检查，签名不匹配直接报错
class Derived2 : public Base {
public:
    virtual void doWork(double x) override;  // 编译报错！int vs double
};
```

### 签名不匹配的常见原因

```cpp
class Base {
public:
    virtual void f(int x);
    virtual void g() const;           // const 成员函数
    virtual void h() &;               // 左值引用限定
};

class Derived : public Base {
public:
    virtual void f(double x) override;     // 错误：int vs double
    virtual void g() override;             // 错误：缺 const
    virtual void h() && override;          // 错误：& vs &&
};
```

### override + final

```cpp
class Base {
public:
    virtual void f();
};

class Derived : public Base {
public:
    void f() override final;  // 重写了 Base::f，且子类不能再重写
};

class DeepDerived : public Derived {
    // void f() override;  // 编译失败！f 被 final 了
};
```

---

## 常见错误（新手踩坑）

**错误 1：忘了加 const 导致重写失败**
```cpp
class Base {
    virtual void process() const;  // const 版本
};
class Derived : public Base {
    virtual void process() override;  // 编译失败！缺 const
    // 不加 override → 静默创建新虚函数，运行时多态不生效
};
```
**修正：** `void process() const override;`

**错误 2：参数类型写错**
```cpp
class Base {
    virtual void on_event(const Event& e);
};
class Derived : public Base {
    virtual void on_event(Event& e) override;  // 编译失败！缺 const
};
```
**修正：** 参数类型必须完全一致，包括 `const`。

**错误 3：基类函数忘了写 virtual**
```cpp
class Base {
    void f();  // 不是虚函数！
};
class Derived : public Base {
    void f() override;  // 编译失败！Base::f 不是虚函数
};
```
**修正：** 基类函数必须声明为 `virtual`。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 多态 | 函数指针表（手动） | 虚函数（自动） | C++ 语言级支持 |
| 重写检查 | 手动对照（容易出错） | `override` 编译器检查 | 防签名不匹配 |
| 阻止重写 | 不适用 | `final` | 设计封闭类 |
| 重写错误后果 | 编译期不可发现 | 不加 `override` 时运行时静默失败 | C++ 的历史包袱 |

**一句话总结：** C 程序员学 C++ 面向对象时，`override` 是第一条该养成的习惯——每写一个重写虚函数必加 `override`，让编译器帮你查签名。

---

## HFT 关联

- **策略基类**：`class Strategy { virtual void on_tick(const Tick&) = 0; };` 子类 `void on_tick(const Tick&) override;` 加 `override` 后签名写错编译期暴露，避免"策略不生效"的隐蔽 bug。
- **事件处理器**：行情网关的事件回调如果签名写错（如缺 `const`），不加 `override` 会导致回调不被调用，行情丢失。
- **final 优化**：`final` 让编译器去掉虚函数调用的间接跳转（devirtualization），减少一个 vtable 查找——HFT 热路径可以受益。

---

## 自测题

1. `override` 关键字的作用是什么？不加 `override` 会有什么后果？
2. 列举三种导致"签名不匹配"的常见原因。
3. `final` 和 `override` 可以同时用吗？各自表达什么意思？
4. 为什么说 `override` 是"C++11 最具性价比的防 bug 特性"？
5. 下面代码有什么问题？
```cpp
class Base {
public:
    virtual void on_tick(const Tick&) const;
};
class Strategy : public Base {
public:
    void on_tick(const Tick&) override;  // 能编译吗？
};
```

---

## 参考与延伸

- 下一节：[Item 13 const_iterator](item13-const-iterator.md)
- 回到：[第 3 章 移步现代 C++](README.md)
