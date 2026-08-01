# 条款 8：吃透 new、delete 多种重载形式的不同含义（全局/类专属/placement new）

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
void *p1 = ::operator new(100);       // 全局
void *p2 = ::operator new(100, buf);  // placement
Widget *w = new Widget;               // 类专属（若定义）
```
