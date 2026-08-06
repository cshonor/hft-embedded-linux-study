# 条款 25：虚拟继承（virtual public）的底层实现、巨大开销，能不用就不用

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
class Base {};
class A : virtual public Base {};
class B : virtual public Base {};
class C : public A, public B {};  // 虚拟继承解决菱形，但有开销
```
