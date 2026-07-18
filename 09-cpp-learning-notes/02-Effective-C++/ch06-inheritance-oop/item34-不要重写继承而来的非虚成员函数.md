# 条款 34：不要重写继承而来的非虚成员函数

## 本节讲什么

破坏 is-a 语义，基类指针和派生类指针调用结果不一样，行为割裂。

## 示例

```cpp
class Base { public: void mf() { /* 非虚 */ } };
class Derived : public Base {};
Derived d;
d.mf();  // 不要指望多态地改写非虚 mf
```
