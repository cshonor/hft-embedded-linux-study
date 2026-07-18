# 条款 21：必须返回对象时，不要强行返回引用

## 本节讲什么

不能返回局部栈对象引用、函数内 `new` 堆对象引用、成员临时引用，悬垂引用崩溃。

## 示例

```cpp
const Rational operator+(const Rational &a, const Rational &b) {
    return Rational(a.n + b.n, a.d);  // 按值返回对象，不要返回局部引用
}
```
