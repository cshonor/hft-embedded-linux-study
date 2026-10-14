# 条款 24：需要所有参数都支持隐式类型转换时，使用非成员函数

## 本节讲什么

**Declare non-member functions when type conversions should apply to all parameters.**
成员函数的左操作数是 `this`——**隐式转换对 this 不生效**，所以 `half * 2` 能编、
`2 * half` 不能编（本机实测）。混合运算要对所有操作数对称时，运算符必须是**非成员函数**。
这条同时解释了为什么 `std::string` 的 `operator+` 是非成员。

← 上一条 [item23 优先非成员非友元](./item23-优先用非成员、非友元函数替代成员函数.md)；
下一条 [item25 不抛异常的 swap](./item25-考虑提供不抛异常的swap重载.md)。

---

## 1. 问题：this 不接受隐式转换（本机实测）

```cpp
class Rational {
    int n_, d_;
public:
    Rational(int n, int d = 1) : n_(n), d_(d) {}   // 非 explicit：int → Rational 隐式可转
    Rational operator*(const Rational& rhs) const { return Rational(n_ * rhs.n_, d_ * rhs.d_); }
};
Rational half(1, 2);
Rational ok = half * 2;      // ✅ 等价于 half.operator*(Rational(2))——2 隐式转换成功
Rational bad = 2 * half;     // ❌ 编译错误！等价于 2.operator*(half)——int 没有成员函数
```

**机制**：成员运算符的左操作数绑定到隐式 `this` 参数，而 `this` 的类型
在类定义那一刻就钉死了——**隐式类型转换只作用于"显式参数"，this 不在其列**。
所以成员版运算符天然不对称：左边必须是"已经是本类"的对象，右边才能享受转换。

## 2. 解法：非成员函数让两个参数地位平等（实测）

```cpp
Rational operator*(const Rational& a, const Rational& b) {
    return Rational(a.num() * b.num(), a.den() * b.den());
}
Rational half(1, 2);
half * 2;      // ✅ operator*(half, Rational(2))
2 * half;      // ✅ operator*(Rational(2), half)——两个参数都走隐式转换，对称！
```

本机实测同款模式（`half / 2` 经非成员版成功，`2 * half` 经成员版编译失败）。

**这就是为什么标准库里**：
- `std::string` 的 `operator+` 是**非成员**——`"a" + str` 和 `str + "a"` 都要能编
- `std::chrono` 的 `operator*`（duration × 数）是非成员——`2 * 100ms` 和 `100ms * 2` 对称

## 3. 什么时候留在类内 / 什么时候必须非成员（对照表）

| 运算符 | 形态 | 原因 |
|---|---|---|
| `=` `[]` `()` `->` | **必须成员**（语言规定） | 左操作数语义特殊 |
| `+=` `-=` 等复合赋值 | **惯例成员** | 修改左操作数本身，本就不对称 |
| `==` `<`（比较） | 非成员或成员均可，**非成员更对称** | `2 == x` 与 `x == 2` 应同义 |
| `+` `-` `*` `/`（二元算术） | **非成员** | 混合类型运算要对两个参数都开放转换 |
| `<<` `>>`（流） | **必须非成员** | 左操作数是 `ostream`（不是你的类） |

## 4. 与 explicit 的联动（别在两边同时开门）

非成员 `operator*` 依赖 `Rational(int)` 的**隐式**转换——
如果你按 item（接口防误用）把构造标了 `explicit`，`2 * half` 又编不过了：

```cpp
explicit Rational(int n, int d = 1);
// 2 * half  → 编译错误（explicit 挡住了转换）——要写得 Rational(2) * half
```

**设计选择**：
- 数学/值类型（Rational、Money、Duration）：转换**有意义**，留隐式 + 非成员运算符
- 资源/句柄类型：转换是**事故源**，explicit 封死 + 不提供混合运算
- 别半开半关：隐式构造 + 成员运算符 = 不对称的陷阱；explicit + 非成员混合运算符 = 自相矛盾

## HFT 关联

- **价格/数量值类型**（Price/Qty 封装类）的混合运算必须非成员：
  `qty * 2`（减仓一半）与 `2 * qty` 在策略代码里都会出现，不对称就是 bug 培养皿
- 反过来，**OrderId/InstrumentId 这类 ID 包装**：explicit 构造 + 不提供算术运算——
  ID 做加减本身就是逻辑错误，类型系统应该拒绝（与 item18 接口防误用配套）
- 对称性还影响**常量折叠**：`2 * PRICE_TICK` 在编译期常量表达式里能否成立
  取决于运算符是否非成员 + constexpr——协议常量表设计时留意

## 代码自测

**题目 1：** 为什么 `half * 2` 能编译而 `2 * half` 不能（成员运算符版）？

<details>
<summary>参考答案</summary>

成员 `operator*` 等价于 `half.operator*(2)`：右参数 `2` 经隐式转换变成 `Rational(2,1)`，
编译通过。而 `2 * half` 等价于 `2.operator*(half)`——`int` 没有成员函数，
且隐式转换**不作用于 this 指针**（this 的类型在类定义时已固定，不参与转换推导）。
所以成员版运算符对左操作数是"必须已是本类"的硬要求，天然不对称。

</details>

**题目 2：** `std::string s; auto r = "prefix" + s;` 能编译，说明了什么设计决策？

<details>
<summary>参考答案</summary>

说明 `std::string` 的 `operator+` 是**非成员函数**（本条款的应用）：
左操作数 `"prefix"`（`const char*`）经隐式转换构造出临时 `string`，
`operator+(string&&, const string&)` 重载命中。
若 `operator+` 是成员函数，`"prefix" + s` 等价于 `"prefix".operator+(s)`——
const char* 没有成员函数，编译失败。标准库让所有"混合运算"走非成员，
保证 `str + "x"`、`"x" + str`、`str + str` 三种形态全对称。

</details>

**题目 3：** 把 `Rational(int)` 标成 `explicit` 后，非成员 `operator*` 还能支持 `2 * half` 吗？
这个联动关系对设计有什么启示？

<details>
<summary>参考答案</summary>

**不能**——`explicit` 关闭了隐式转换，`2 * half` 编译失败，必须写 `Rational(2) * half`。
启示：隐式转换和非成员运算符是**配套设计**——
数学/值类型：隐式构造 + 非成员运算符（对称优先）；
资源/ID 类型：explicit + 不提供混合运算（防误用优先）。
最差的组合是半开半关：隐式构造 + 成员运算符（不对称陷阱），
或 explicit + 指望隐式转换的非成员运算（自相矛盾）。

</details>
