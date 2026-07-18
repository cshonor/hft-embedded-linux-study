# 条款 46：在模板内定义非成员函数，使用友元模板

## 本节讲什么

运算符重载嵌入类内友元，自动实例化，支持参数推导。

## 示例

```cpp
template<typename T>
class Widget {
    friend void doStuff(const Widget<T> &w) { /* 可访问私有 */ }
};
```
