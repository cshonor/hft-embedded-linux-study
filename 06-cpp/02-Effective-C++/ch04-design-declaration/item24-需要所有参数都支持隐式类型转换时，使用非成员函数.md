# 条款 24：需要所有参数都支持隐式类型转换时，使用非成员函数

## 本节讲什么

成员函数 `this` 不参与隐式转换，运算符重载写全局非成员函数才能两边都自动转换。

## 示例

```cpp
class Rational {
    int n, d;
    friend Rational operator*(const Rational &, const Rational &);
};
Rational operator*(const Rational &a, const Rational &b) {
    return Rational(a.n * b.n, a.d * b.d);
}
```
