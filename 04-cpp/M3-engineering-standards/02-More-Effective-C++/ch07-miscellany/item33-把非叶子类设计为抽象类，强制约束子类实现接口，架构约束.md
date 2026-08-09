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

---

## 代码自测

**题目 1：** 为什么要把非叶子类设计为抽象类？
```cpp
class Shape {  // 非抽象
public:
    virtual double area() const { return 0; }  // 默认实现
    virtual ~Shape() = default;
};
class Circle : public Shape { ... };
Shape s;  // 创建 Shape 实例有意义吗？
```

<details>
<summary>参考答案</summary>

创建 `Shape` 实例没有意义——形状必须是一个具体形状。如果允许 `Shape s;`，`s.area()` 返回 0，这是无意义的默认值。将 `Shape` 设计为抽象类（纯虚函数）：
```cpp
class Shape {
public:
    virtual double area() const = 0;  // 纯虚
    virtual ~Shape() = default;
};
// Shape s;  // 编译错误，无法实例化抽象类
```
好处：1) 防止无意义的实例化；2) 强制派生类实现接口；3) 架构上更清晰——Shape 是接口不是实现。

</details>
