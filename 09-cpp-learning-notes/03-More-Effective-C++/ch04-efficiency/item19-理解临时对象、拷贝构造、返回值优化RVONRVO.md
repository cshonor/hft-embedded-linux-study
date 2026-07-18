# 条款 19：理解临时对象、拷贝构造、返回值优化 RVO/NRVO

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
BigObject factory() {
    return BigObject();  // 返回值优化 RVO
}
BigObject o = factory();
```
