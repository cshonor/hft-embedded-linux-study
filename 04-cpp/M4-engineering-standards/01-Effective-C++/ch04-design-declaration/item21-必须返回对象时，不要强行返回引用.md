# 条款 21：必须返回对象时，不要强行返回引用

## 本节讲什么

不能返回局部栈对象引用、函数内 `new` 堆对象引用、成员临时引用，悬垂引用崩溃。

## 示例

```cpp
const Rational operator+(const Rational &a, const Rational &b) {
    return Rational(a.n + b.n, a.d);  // 按值返回对象，不要返回局部引用
}
```

---

## 代码自测

**题目 1：** 下面函数返回局部变量的引用有什么问题？
```cpp
const Rational& operator*(const Rational& lhs, const Rational& rhs) {
    Rational result(lhs.n * rhs.n, lhs.d * rhs.d);
    return result;  // 返回局部变量的引用
}
```

<details>
<summary>参考答案</summary>

`result` 是局部变量，函数返回时被销毁，返回的引用成为悬空引用——使用它就是未定义行为。正确做法是按值返回：`Rational operator*(...)`，编译器会做 RVO/NRVO 优化，实际上不会产生额外拷贝。

</details>
