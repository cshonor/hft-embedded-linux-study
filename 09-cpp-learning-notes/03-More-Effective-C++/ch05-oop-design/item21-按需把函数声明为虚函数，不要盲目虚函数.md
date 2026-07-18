# 条款 21：按需把函数声明为虚函数，不要盲目虚函数

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
class Base {
public:
    virtual void interface() = 0;
    void helper() { /* 非虚即可 */ }
};
```
