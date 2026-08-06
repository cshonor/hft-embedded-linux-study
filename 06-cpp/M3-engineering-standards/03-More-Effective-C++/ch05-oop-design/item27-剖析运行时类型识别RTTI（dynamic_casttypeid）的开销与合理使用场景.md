# 条款 27：剖析运行时类型识别 RTTI（dynamic_cast/typeid）的开销与合理使用场景

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
Base *bp = getObject();
if (auto *dp = dynamic_cast<Derived *>(bp)) {
    dp->derivedOnly();
}
```
