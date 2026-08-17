# 条款 22：区分接口继承和实现继承，很多继承设计错误根源就在这里

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
class Interface {
public:
    virtual void draw() = 0;        // 接口
    virtual void resize(int) { }    // 可选默认实现
};
```

---

## 代码自测

**题目 1：** 以下设计中，哪些函数应该是纯虚、虚、非虚？
```cpp
class Shape {
    virtual double area();      // 所有形状都能算面积，但公式不同
    virtual void draw();        // 默认实现可复用
    int id() const;             // ID 不随形状变化
};
```

<details>
<summary>参考答案</summary>

`area()`：应为纯虚（`= 0`）——每个形状的面积公式不同，且 Shape 本身无法计算面积。
`draw()`：保持虚函数（impure）——可以有默认实现，派生类可选择覆盖或复用。
`id()`：应保持非虚——ID 的行为不变，不应被覆盖。纯虚 = 只继承接口；虚 = 接口 + 默认实现；非虚 = 接口 + 强制实现。

</details>
