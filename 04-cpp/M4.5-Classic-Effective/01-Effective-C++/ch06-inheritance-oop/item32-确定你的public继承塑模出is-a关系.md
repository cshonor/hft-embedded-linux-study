# 条款 32：确定你的 public 继承塑模出 is-a 关系

## 本节讲什么

公有继承严格满足「派生类是一种基类」，不符合语义不要用 public 继承。

## 示例

```cpp
class Person { /* 人 */ };
class Student : public Person { /* Student is-a Person */ };
void study(const Person &p) { /* 接受任何人 */ }
```

---

## 代码自测

**题目 1：** 以下继承关系是否正确？为什么？
```cpp
class Bird { public: virtual void fly(); };
class Penguin : public Bird {};
Penguin p;
p.fly();  // 企鹅能飞？
```

<details>
<summary>参考答案</summary>

不正确。`Penguin` is-a `Bird` 暗示企鹅具备鸟的所有行为，包括飞——但企鹅不会飞。`is-a` 关系要求派生类能替换基类而程序行为正确。修正：要么去掉 `Bird::fly()`（不是所有鸟都能飞），要么区分 `FlyingBird` 和 `Bird`。is-a 不是「在现实世界中属于同一类别」，而是「在程序中能正确替换」。

</details>
