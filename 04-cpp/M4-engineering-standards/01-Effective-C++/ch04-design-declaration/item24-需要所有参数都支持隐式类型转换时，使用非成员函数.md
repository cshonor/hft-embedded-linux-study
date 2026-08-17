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

---

## 代码自测

**题目 1：** 下面代码能否隐式转换？为什么需要非成员函数？
```cpp
class Rational {
public:
    Rational(int numerator = 0, int denominator = 1);
    // 成员函数版
    Rational operator*(const Rational& rhs) const;
};
Rational r(1, 2);
Rational result1 = r * 2;   // OK？
Rational result2 = 2 * r;   // OK？
```

<details>
<summary>参考答案</summary>

`result1` OK：`2` 隐式转换为 `Rational(2)`。
`result2` 错误：成员函数要求左操作数是 `Rational`，`2` 不是 `Rational` 且不能调用成员函数。改为非成员函数可解决：
```cpp
const Rational operator*(const Rational& lhs, const Rational& rhs);
// 此时 2 * r 等价于 operator*(Rational(2), r)，两参数都能隐式转换
```

</details>
