# 条款 26：限制某个类只能在堆上创建 / 只能在栈上创建的设计技巧

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
class HeapOnly {
public:
    static HeapOnly *create() { return new HeapOnly; }
    void destroy() { delete this; }
private:
    HeapOnly() = default;
    ~HeapOnly() = default;
};
```
