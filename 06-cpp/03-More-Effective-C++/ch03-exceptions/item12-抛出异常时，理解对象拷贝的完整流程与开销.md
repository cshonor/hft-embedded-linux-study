# 条款 12：抛出异常时，理解对象拷贝的完整流程与开销

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
class MyError : public std::exception {};
void f() {
    try { throw MyError(); }
    catch (MyError e) { /* 按值捕获会切片/拷贝 */ }
}
```
