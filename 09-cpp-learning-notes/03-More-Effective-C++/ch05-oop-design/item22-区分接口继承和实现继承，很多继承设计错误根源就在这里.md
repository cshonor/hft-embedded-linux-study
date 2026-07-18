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
