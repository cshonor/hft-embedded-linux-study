# 条款 24：理解虚函数、多重继承带来的内存布局、开销、歧义问题

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
class A { virtual void f(); };
class B : public A { void f() override; };
// 多重继承时注意虚表与对象布局
```
