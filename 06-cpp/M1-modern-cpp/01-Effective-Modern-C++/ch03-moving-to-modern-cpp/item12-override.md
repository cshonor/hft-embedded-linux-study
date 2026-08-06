# Item 12：把重写函数声明为 override

> 第 3 章 移步现代 C++ · Item 12 · 上一节：[Item 11 =default](item11-default.md)

## 这节讲什么

`override` 关键字让编译器**检查**是否真的重写了基类虚函数——签名不匹配会编译报错，而非静默创建一个新虚函数。这是 C++11 最具性价比的防 bug 特性之一。

---

## 核心问题

```cpp
class Base {
public:
    virtual void doWork(int x);       // 基类虚函数
};

class Derived : public Base {
public:
    virtual void doWork(double x) override;  // 编译报错！签名不匹配（int vs double）
    // 没有 override → 静默创建新虚函数，Base::doWork 没被重写
};
```

签名不匹配的常见原因：
- 参数类型差异（`int` vs `double`）
- const 差异（`void f()` vs `void f() const`）
- 引用限定符差异（`void f() &` vs `void f() &&`）

---

## 新手要点（和 C 的区别）

- **C 没有虚函数/继承**——这是 C++ 独有的面向对象特性。C 程序员学 C++ 面向对象时，`override` 是第一条该养成的习惯。
- **规则**：每写一个重写虚函数，**必加 `override`**。不加 = 编译器不检查 = 签名写错也不知道。
- **C++11 还加了 `final`**：`void f() override final;` 表示"重写了基类，且子类不能再重写"。

---

## HFT 关联

- **策略基类**：`class Strategy { virtual void on_tick(const Tick&) = 0; };` 子类 `void on_tick(const Tick&) override;` 加 `override` 后签名写错编译期暴露，避免"策略不生效"的隐蔽 bug。

---

## 自测题

1. `override` 关键字的作用是什么？不加 `override` 会有什么后果？
2. 列举三种导致"签名不匹配"的常见原因。
3. `final` 和 `override` 可以同时用吗？各自表达什么意思？
4. 为什么说 `override` 是"C++11 最具性价比的防 bug 特性"？

---

## 参考与延伸

- 下一节：[Item 13 const_iterator](item13-const-iterator.md)
- 回到：[第 3 章 移步现代 C++](README.md)
