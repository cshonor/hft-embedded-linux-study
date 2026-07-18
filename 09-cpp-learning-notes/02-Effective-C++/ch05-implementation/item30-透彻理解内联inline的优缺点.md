# 条款 30：透彻理解内联 inline 的优缺点

## 本节讲什么

小高频函数适合 inline；大函数、递归、虚函数不要 inline，代码膨胀得不偿失。

## 示例

```cpp
// Widget.h 只前置声明，实现放 Widget.cpp
class WidgetImpl;
class Widget {
    std::unique_ptr<WidgetImpl> pImpl;
};
```
