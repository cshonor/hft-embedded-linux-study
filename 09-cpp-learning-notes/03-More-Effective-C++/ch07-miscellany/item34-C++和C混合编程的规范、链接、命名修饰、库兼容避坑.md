# 条款 34：C++ 和 C 混合编程的规范、链接、命名修饰、库兼容避坑

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
extern "C" void c_api(int x);
// C++ 实现
extern "C" void c_api(int x) { /* ... */ }
```
