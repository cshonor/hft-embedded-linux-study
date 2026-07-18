# 条款 33：区分接口继承和实现继承

## 本节讲什么

纯虚函数：只继承接口；普通虚函数：继承接口 + 默认实现；非虚函数：强制继承接口 + 固定实现。

## 示例

```cpp
class Shape {
public:
    virtual void draw() = 0;   // 接口继承
    virtual void resize(int) { /* 默认实现 */ }  // 实现继承
};
```
