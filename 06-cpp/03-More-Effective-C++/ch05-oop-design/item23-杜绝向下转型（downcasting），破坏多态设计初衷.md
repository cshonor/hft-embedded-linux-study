# 条款 23：杜绝向下转型（downcasting），破坏多态设计初衷

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
void process(Base &b) {
    // Derived *d = static_cast<Derived*>(&b);  // 避免向下转型
    b.virtualMethod();
}
```
