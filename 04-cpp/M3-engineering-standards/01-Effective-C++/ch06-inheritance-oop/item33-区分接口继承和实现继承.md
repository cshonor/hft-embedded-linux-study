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

---

## 代码自测

**题目 1：** 纯虚函数、虚函数（impure virtual）、非虚函数分别表达什么继承语义？
```cpp
class Shape {
    virtual void draw() const = 0;  // ?
    virtual void error(const string& msg);  // ?
    int objectId() const;  // ?
};
```

<details>
<summary>参考答案</summary>

纯虚函数（`= 0`）：只继承接口，派生类必须实现。
虚函数（impure）：继承接口 + 默认实现，派生类可 override。
非虚函数：继承接口 + 强制实现，派生类不应 override（不变性）。

</details>
