# 条款 29：实现引用计数智能指针（shared_ptr 前身），循环引用问题来源就在这一条

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
class RefCounted {
    int *count;
    int *data;
public:
    RefCounted() : count(new int(1)), data(new int(0)) {}
    RefCounted(const RefCounted &o) : count(o.count), data(o.data) { ++*count; }
    ~RefCounted() { if (--*count == 0) { delete count; delete data; } }
};
```
