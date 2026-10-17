# 条款 46：在模板内定义非成员函数，使用友元模板

## 本节讲什么

运算符重载嵌入类内友元，自动实例化，支持参数推导。

## 示例

```cpp
template<typename T>
class Widget {
    friend void doStuff(const Widget<T> &w) { /* 可访问私有 */ }
};
```

---

## 代码自测

**题目 1：** 以下模板中的 `operator+` 为什么不能隐式转换？如何修复？
```cpp
template<typename T>
class Rational {
    T num, den;
public:
    Rational(const T& n = 0, const T& d = 1);
};
template<typename T>
Rational<T> operator+(const Rational<T>& a, const Rational<T>& b);
// 调用
Rational<int> r(1, 2);
r + 2;  // 能隐式转换 2 为 Rational<int> 吗？
```

<details>
<summary>参考答案</summary>

不能。模板参数推导时不会考虑隐式转换——编译器看到 `r + 2`，需要推导 `T`，但 `2` 是 int 不是 `Rational<int>`，推导失败。修复：在类内声明友元模板：
```cpp
template<typename T>
class Rational {
    friend Rational<T> operator+(const Rational<T>& a, const Rational<T>& b) {
        return Rational<T>(a.num + b.num, a.den + b.den);
    }
};
```
友元声明让编译器在实例化类时就能生成对应 `operator+`，此时 `T` 已知，可以隐式转换。

</details>
