# 第 14 章 操作重载与类型转换

本章介绍如何让自定义类类型在直观上像内置类型一样使用，核心是通过重载机制给内置运算符赋予新含义。

## 小节

- [运算符重载](./14.1-运算符重载.md)
- [函数调用运算符与函数对象](./14.2-函数调用运算符与函数对象.md)
- [类型转换运算符](./14.3-类型转换运算符.md)


## 章节摘要

运算符重载（让自定义类型像内置类型一样使用运算符）、函数调用运算符与函数对象（lambda 的底层机制）、类型转换运算符。

### 和 C 的区别

| C | C++ |
|---|-----|
| 无运算符重载 | 可重载 `+`/`-`/`[]`/`()`/`->` 等 |
| 函数指针回调 | 函数对象/lambda（可内联+有状态） |
| `(int)x` C 风格转换 | 可自定义类型转换运算符 |

## 章节自测

### Q1: 成员 vs 非成员

```cpp
class Money {
    int cents;
public:
    Money(int c) : cents(c) {}
    Money operator+(const Money &rhs) const { return Money(cents + rhs.cents); }
};
Money m(100);
// m + 50;    // A: 合法吗？
// 50 + m;    // B: 合法吗？
```

> A 和 B 分别合法吗？如何让 B 也合法？

<details>
<summary>答案与复习指引</summary>

**A: 合法。** `m + 50` 等价于 `m.operator+(50)`，`50` 隐式转换为 `Money(50)`。
**B: 编译错误。** `50 + m` 等价于 `50.operator+(m)`，`int` 没有 `operator+` 成员接受 `Money`。

**修复：** 用非成员函数重载：
```cpp
Money operator+(const Money &lhs, const Money &rhs) {
    return Money(lhs.get_cents() + rhs.get_cents());
}
```
非成员函数允许左右操作数都隐式转换。

**规则：** 如果左操作数可能需要隐式转换，用非成员函数；如果必须访问私有成员且不需要左操作数转换，用成员函数。

**复习：** → [运算符重载](./14.1-运算符重载.md)
</details>

### Q2: 函数对象

```cpp
class Multiplier {
    int factor;
public:
    Multiplier(int f) : factor(f) {}
    int operator()(int x) const { return x * factor; }
};
Multiplier triple(3);
std::cout << triple(5);  // 输出什么？
```

> 输出是什么？函数对象和函数指针有什么区别？

<details>
<summary>答案与复习指引</summary>

**输出：** `15` — `triple(5)` 调用 `operator()(5)` 即 `5 * 3`

**和函数指针的区别：**
1. **有状态**：函数对象可保存数据（`factor`），函数指针不行
2. **可内联**：编译器能看到函数对象类型，可内联 `operator()`；函数指针是间接调用，难以内联
3. **STL 算法接受函数对象**：`std::sort(v.begin(), v.end(), Multiplier(3))` — 有状态的比较器

**lambda 本质：** lambda 就是匿名函数对象——编译器为每个 lambda 生成一个含 `operator()` 的类。

**复习：** → [函数调用运算符与函数对象](./14.2-函数调用运算符与函数对象.md)
</details>

### Q3: 类型转换运算符

```cpp
class IntWrapper {
    int value;
public:
    IntWrapper(int v) : value(v) {}
    operator int() const { return value; }  // 隐式转换
};
IntWrapper w(42);
int x = w;        // A: 合法？
int y = w + 10;   // B: 合法？
```

> A 和 B 合法吗？隐式转换有什么风险？

<details>
<summary>答案与复习指引</summary>

**A 和 B 都合法。** `operator int()` 允许 `IntWrapper` 隐式转换为 `int`。

**风险：** 隐式转换可能在你不期望的地方发生——函数期望 `int` 参数但传了 `IntWrapper`，静默转换可能隐藏 bug。多个转换路径会导致二义性。

**最佳实践：** 用 `explicit` 修饰转换运算符（C++11 起），禁止隐式转换：
```cpp
explicit operator int() const { return value; }
// int x = w;       // 编译错误
// int x = static_cast<int>(w);  // OK
```

**复习：** → [类型转换运算符](./14.3-类型转换运算符.md)
</details>
