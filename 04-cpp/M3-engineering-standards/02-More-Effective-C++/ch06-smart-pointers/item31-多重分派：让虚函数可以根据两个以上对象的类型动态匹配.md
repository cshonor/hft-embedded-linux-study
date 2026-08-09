# 条款 31：多重分派：让虚函数可以根据两个以上对象的类型动态匹配

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
class Shape { public: virtual void collide(Shape &) = 0; };
class Rect : public Shape {
public:
    void collide(Shape &other) override {
        other.collideWithRect(*this);  // 双重分派模式
    }
};
```
