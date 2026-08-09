# 条款 39：谨慎使用私有继承

## 本节讲什么

私有继承代表「复用实现」，不是 is-a；能用组合就不用私有继承。

## 示例

```cpp
class Engine { void tune(); };
class Car : private Engine {};  // 谨慎：实现继承，不是 is-a
```

---

## 代码自测

**题目 1：** 私有继承和组合都能表达 is-implemented-in-terms-of，何时选哪个？
```cpp
// 方式A：私有继承
class Timer : private Widget { /* ... */ };
// 方式B：组合
class Timer { Widget w; /* ... */ };
```

<details>
<summary>参考答案</summary>

优先用组合（方式B）。私有继承在以下情况才考虑：1) 需要访问 protected 成员；2) 需要重写虚函数。如果基类是空类（EBO，Empty Base Optimization），私有继承可以省 1 字节（组合方式空成员也占至少 1 字节）。其余情况组合更好——更灵活、更易理解。

</details>
