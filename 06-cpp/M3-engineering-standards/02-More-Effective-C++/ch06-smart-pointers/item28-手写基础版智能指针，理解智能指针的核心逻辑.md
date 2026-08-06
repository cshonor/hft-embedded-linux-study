# 条款 28：手写基础版智能指针，理解智能指针的核心逻辑

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
template<typename T>
class SmartPtr {
    T *ptr;
public:
    explicit SmartPtr(T *p) : ptr(p) {}
    ~SmartPtr() { delete ptr; }
    T &operator*() { return *ptr; }
};
```
