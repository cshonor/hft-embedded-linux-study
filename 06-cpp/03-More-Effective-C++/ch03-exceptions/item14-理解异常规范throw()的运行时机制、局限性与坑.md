# 条款 14：理解异常规范 throw() 的运行时机制、局限性与坑

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
void old_api() throw();       // C++17 前异常规范，已不推荐
void modern_api() noexcept;   // 现代写法
```
