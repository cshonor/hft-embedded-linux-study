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

---

## 代码自测

**题目 1：** 什么是多重分派？为什么 C++ 不直接支持？
```cpp
class Shape {
public:
    virtual bool intersects(const Shape& s) const = 0;
    virtual bool intersectsCircle(const Circle& c) const = 0;
    virtual bool intersectsRect(const Rect& r) const = 0;
};
// Circle 和 Rect 相交判断需要知道双方类型——虚函数只分派一个对象
```

<details>
<summary>参考答案</summary>

多重分派是根据两个或多个对象的运行时类型选择函数。C++ 虚函数只支持单分派（根据 this 的实际类型）。实现多重分派的常见方法：1) 双重分派（ visitor 模式或互递归）——`a.intersects(b)` 中 `a` 先分派到具体类型，再调 `b.intersectsCircle(*this)` 二次分派；2) 类型表 + 函数指针；3) `std::variant` + `std::visit`（C++17）。多重分派代码复杂，但能避免类型 switch。

</details>
