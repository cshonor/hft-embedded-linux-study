# 条款 46：需要类型转换时请为模板定义非成员函数

## 本节讲什么

**Define non-member functions inside templates when type conversions are desired.**
item24 说"混合运算要对称就该用非成员函数"——到了**类模板**这里，
非成员函数还有一个模板特有的坑：**模板实参推导不做隐式转换**。
`half * 2` 里的 `2` 不会被推导成 `Rational<int>`。解法：
把非成员运算符**定义成类内 friend**（本机 g++ 13.3 实测）。

← 上一条 [item45 成员函数模板](./item45-运用成员函数模板接受所有兼容类型.md)；
下一条 [item47 traits classes](./item47-请使用traits-classes表现类型信息.md)。

---

## 1. 模板特有的坑：推导不做隐式转换

```cpp
template <typename T>
class Rational { /*...*/ };

template <typename T>
Rational<T> operator*(const Rational<T>& a, const Rational<T>& b);   // 声明在类外

Rational<int> half(1, 2);
half * 2;      // ❌ 编译错误！
```

**机制**：`operator*` 是**函数模板**——编译器要从实参推导 T：
第一个实参 `half` → `T = int`；第二个实参 `2`（int）→ 期望 `Rational<T>`，
但**模板实参推导不考虑隐式转换**（int 不会"先变成 Rational<int>"再推导）——
两次推导结果矛盾，候选丢弃。

非模板世界（item24）里非成员函数就够了，因为参数类型是**固定的**，
转换照常发生；模板世界里参数类型要**推导**，转换的门被推导规则关掉了。

## 2. 解法：friend 函数定义在类内（本机实测）

```cpp
template <typename T>
class Rational {
    T n_, d_;
public:
    Rational(const T& n, const T& d = T(1)) : n_(n), d_(d) {}
    T num() const { return n_; }

    // friend 定义在类模板内：随每个实例化生成一个**非模板**普通函数
    friend Rational operator*(const Rational& a, const Rational& b) {
        return Rational(a.n_ * b.n_, a.d_ * b.d_);
    }
};
Rational<int> half(1, 2);
auto r1 = half * 2;      // 本机实测 ✅
auto r2 = 2 * half;      // 本机实测 ✅——两个参数都能走隐式转换！
```

**为什么这样就通了**：类内 friend 定义**不是函数模板**——
它是"随 `Rational<int>` 实例化而生成的**普通非成员函数**，
签名是 `operator*(const Rational<int>&, const Rational<int>&)`"。
普通函数的参数**不做推导**，隐式转换的大门重新打开：
`2` → `Rational<int>(2, 1)` 照常发生。

这个"friend 定义在类内"的手法（Barton–Nackman trick）还带来一个赠品：
该函数**只能通过 ADL 找到**（→ 18.2 ⑤）——不污染外围命名空间。

## 3. 手法对比总表

| 写法 | 混合运算 `2 * half` | 原理 |
|---|---|---|
| 成员 `operator*` | ❌（this 不转换，item24） | this 类型固定 |
| 类外函数模板 | ❌（推导不转换，本条①） | 模板实参推导规则 |
| **类内 friend 定义** | ✅ | 实例化出普通函数，无推导有转换 |
| C++20：约束 + 显式两个重载 | ✅（更繁琐） | 为对称参数各写一个模板重载 |

## HFT 关联

- 价格/数量值类型模板（`Price<Tick>`/`Qty<Lot>`）的混合运算——
  `qty * 2`、`2 * price` 在策略代码里高频出现，
  类内 friend 是同时满足"模板化 + 对称转换"的唯一干净解
- friend 函数定义在类内 = **隐式 inline**——
  头文件库（本仓值类型都是头文件）不用担心 ODR（→ item30 inline 语义）
- 注意约束配套：`Rational<int>` 与 `Rational<double>` 之间**不**互转
  （T 不同就是不同类）——跨 tick 精度的运算要显式构造，
  这是特性不是缺陷（精度转换必须显式表态，防静默精度损失）

## 代码自测

**题目 1：** 类外的 `template <typename T> operator*(const Rational<T>&, const Rational<T>&)`
为什么连 `half * 2` 都编译不过？

<details>
<summary>参考答案</summary>

模板实参推导**不做隐式转换**：`half` 推导出 `T = int`，
但 `2`（类型 int）要匹配参数 `const Rational<T>&`——
推导规则只允许"实参类型直接对应"，不会先帮你把 int 转成 `Rational<int>` 再推导。
两次推导对不上（int ≠ Rational<T>），候选被丢弃 → 编译错误。
非模板函数的参数类型是固定的，转换正常发生；
模板函数的参数类型要推导，转换被规则关闭——这就是模板特有的坑。

</details>

**题目 2：** friend 定义在类内为什么能绕过推导问题？它生成的是什么？

<details>
<summary>参考答案</summary>

类内 friend 定义**不是模板**——它随宿主类模板的**每个实例化**生成一个
普通的非成员函数：`Rational<int>` 实例化时，世界上就多了一个
`operator*(const Rational<int>&, const Rational<int>&)` 普通函数。
普通函数调用**没有实参推导**——参数类型固定（Rational<int>），
隐式转换规则完全适用：`2` → `Rational<int>(2,1)`。
于是 `half * 2` 和 `2 * half` 都通（本机实测）。
赠品：这类函数只能经 ADL 找到（不污染外围命名空间），
且定义在类内隐式 inline（头文件库无 ODR 问题）。

</details>

**题目 3：** 这个手法和 item24（非成员函数支持全参数转换）是什么关系？

<details>
<summary>参考答案</summary>

item24 是**语义层**："混合运算要对所有参数对称 → 用非成员函数"——
解决的是"this 不参与隐式转换"的不对称。
item46 是**机制层**："到了类模板里，普通非成员函数模板又过不了推导关"——
解决的是"模板实参推导不做隐式转换"。
链条完整版：混合运算要对称（24）→ 非成员函数 → 类模板里非成员模板推导演不通（46）
→ **类内 friend 定义**（实例化出普通函数，转换恢复）。
写模板值类型（Price/Qty/Rational）的混合运算，这条链是标准答案。

</details>
