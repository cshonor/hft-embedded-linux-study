# 第二部分 运算符重载（Operators）

写自定义类运算符的规范与避坑。

## 条款

- [条款 5：警惕编译器隐式类型转换函数带来的意外调用](./item05-警惕编译器隐式类型转换函数带来的意外调用.md)
- [条款 6：分清前置 ++/-- 和后置 ++/-- 重载写法、性能差异](./item06-分清前置++--和后置++--重载写法、性能差异.md)
- [条款 7：永远不要重载逻辑与 &&、逻辑或 ||、逗号运算符](./item07-永远不要重载逻辑与&&、逻辑或、逗号运算符.md)
- [条款 8：吃透 new、delete 多种重载形式的不同含义（全局/类专属/placement new）](./item08-吃透new、delete多种重载形式的不同含义（全局类专属placementnew）.md)


## 章节摘要

运算符重载：警惕隐式类型转换、前置/后置 `++` 差异、不要重载 `&&`/`||`/`,`、理解 `new`/`delete` 重载形式。

## 代码自测

### Q1: 前置 vs 后置 ++

```cpp
class Counter {
    int n;
public:
    Counter& operator++() { ++n; return *this; }      // 前置
    Counter operator++(int) { Counter old = *this; ++n; return old; }  // 后置
};
```

> 前置和后置 `++` 的重载签名有什么区别？性能差异？

<details>
<summary>答案与复习指引</summary>

**签名区别：** 前置 `operator++()` 无参数；后置 `operator++(int)` 有 `int` 参数（编译器传入 0 作为占位符区分）。

**性能差异：** 后置版本需要拷贝旧值（`Counter old = *this`）再返回——对内置类型无差异，但对复杂类型（迭代器/自定义类）前置更高效（无拷贝）。

**习惯：** C++ 优先用前置 `++it`，尤其对迭代器。STL 迭代器的前置 `++` 不需要保存旧值。

**复习：** → [条款 6：分清前置 ++/-- 和后置 ++/--](./item06-分清前置++--和后置++--重载写法、性能差异.md)
</details>

### Q2: 不要重载 && 和 ||

```cpp
class Bool {
    bool val;
public:
    Bool(bool v) : val(v) {}
    bool operator&&(const Bool& rhs) const { return val && rhs.val; }
};
Bool checkA();
Bool checkB();
if (checkA() && checkB()) { ... }  // 如果重载了 &&，求值顺序是什么？
```

> 重载 `&&` 后，`checkB()` 一定会在 `checkA()` 之后执行吗？

<details>
<summary>答案与复习指引</summary>

**不保证。** 内置 `&&` 有**短路求值**——左边为 `false` 时右边不求值。但重载的 `operator&&` 是函数调用——函数参数的求值顺序在 C++17 前是未指定的。`checkA()` 和 `checkB()` 可能以任意顺序求值，甚至 `checkB()` 先执行。

**这破坏了短路语义：** 如果 `checkB()` 依赖 `checkA()` 的结果（如 `ptr != nullptr && ptr->value > 0`），重载 `&&` 会导致 `ptr->value` 在 `ptr` 为空时被访问 → 崩溃。

**规则：** 永远不要重载 `&&`/`||`/`,`——它们的行为依赖短路/顺序语义，重载后无法保证。

**复习：** → [条款 7：永远不要重载 &&、||、逗号](./item07-永远不要重载逻辑与&&、逻辑或、逗号运算符.md)
</details>
