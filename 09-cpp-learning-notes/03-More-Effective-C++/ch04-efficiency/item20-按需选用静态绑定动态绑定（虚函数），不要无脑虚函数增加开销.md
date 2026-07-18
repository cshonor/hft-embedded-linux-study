# 条款 20：按需选用静态绑定/动态绑定（虚函数），不要无脑虚函数增加开销

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
class Base { public: virtual void f(); };
class Derived : public Base {};
void use(Base &b) { b.f(); }  // 需要多态才用 virtual
```
