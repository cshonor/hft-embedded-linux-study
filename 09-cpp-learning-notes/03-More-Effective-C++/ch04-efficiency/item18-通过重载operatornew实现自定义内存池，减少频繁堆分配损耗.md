# 条款 18：通过重载 operator new 实现自定义内存池，减少频繁堆分配损耗

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
class Pool {
    char slab[4096];
public:
    void *allocate(std::size_t n);
    void deallocate(void *p);
};
```
