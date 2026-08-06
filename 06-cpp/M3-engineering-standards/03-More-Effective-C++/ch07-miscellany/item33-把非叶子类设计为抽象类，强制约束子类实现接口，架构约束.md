# 条款 33：把非叶子类设计为抽象类，强制约束子类实现接口，架构约束

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
class NonLeaf {
public:
    virtual void mustImplement() = 0;
protected:
    NonLeaf() = default;  // 抽象类，不能实例化
};
```
