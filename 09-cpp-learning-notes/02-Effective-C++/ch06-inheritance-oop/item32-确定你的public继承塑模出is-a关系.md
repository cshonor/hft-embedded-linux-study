# 条款 32：确定你的 public 继承塑模出 is-a 关系

## 本节讲什么

公有继承严格满足「派生类是一种基类」，不符合语义不要用 public 继承。

## 示例

```cpp
class Person { /* 人 */ };
class Student : public Person { /* Student is-a Person */ };
void study(const Person &p) { /* 接受任何人 */ }
```
