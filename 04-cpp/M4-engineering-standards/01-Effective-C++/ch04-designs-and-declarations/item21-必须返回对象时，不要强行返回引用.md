# 条款 21：必须返回对象时，不要强行返回引用

## 本节讲什么

**Don't try to return a reference when you must return an object.** 为了"省一次拷贝"
而把返回值写成引用，是 C++ 最经典的自作聪明——三种写法全是悬垂/泄漏。
C++17 起 RVO 是**强制的**：按值返回根本不拷贝（本机实测）。三层处理：
**三种错误写法 → RVO/NRVO 实测 → 什么时候返回引用才对**。

← 上一条 [item20 优先 const 引用传参](./item20-优先使用const引用传参，而非值传递.md)；
下一条 [item22 成员变量一律 private](./item22-成员变量一律声明为private.md)。

---

## 1. 三种"省拷贝"的错误写法（全是坑）

```cpp
// ❌ 错误一：返回局部对象引用——悬垂
const Rational& operator*(const Rational& a, const Rational& b) {
    Rational result(a.n * b.n, a.d * b.d);
    return result;          // result 离开作用域即析构，引用指向尸体
}

// ❌ 错误二：返回 new 出来的对象引用——谁来 delete？
const Rational& operator*(const Rational& a, const Rational& b) {
    Rational* result = new Rational(a.n * b.n, a.d * b.d);
    return *result;         // 调用方拿到的是引用，没有指针，永远不会 delete → 泄漏
}

// ❌ 错误三：返回 static 局部对象引用——线程灾难 + 代数错误
const Rational& operator*(const Rational& a, const Rational& b) {
    static Rational result;
    result = Rational(a.n * b.n, a.d * b.d);
    return result;          // 多线程共享同一 static；且 (a*b) == (c*d) 永真（同一对象）
}
```

错误三最值得展开：`if ((a*b) == (c*d))` 在 static 写法下**永远为真**——
两次调用返回同一个 static 对象，自己比自己。这类 bug 在数学库/价格计算里是静默错误。

## 2. 正确写法：按值返回，RVO/NRVO 让它零成本（本机实测）

```cpp
Rational operator*(const Rational& a, const Rational& b) {
    return Rational(a.n * b.n, a.d * b.d);   // 按值返回——不拷贝！
}
```

**本机实测**（g++ 13.3，**-O0** 关优化）：

```
--- RVO（return Rational(...)）---
构造 1/2                  ← 只有一次构造，没有拷贝/移动！
--- NRVO（return 具名局部变量）---
构造 3/4                  ← 同样零拷贝零移动
```

- **RVO**（Return Value Optimization）：`return T(...)` 直接在调用方的接收空间构造——
  C++17 起**标准强制**（不再是"优化"，是语言保证）
- **NRVO**（具名 RVO）：`return local;` 编译器可以做但**不强制**——主流编译器
  （g++/clang）即使 -O0 也做（本机实测）
- 推论：`return std::move(local);` 是**反优化**——阻止 NRVO 强制造一次移动
  （→ 19.7 ⑤ 翻车点三）

## 3. 什么时候返回引用才对（对照表）

| 场景 | 返回什么 | 例子 |
|---|---|---|
| 函数**制造**新对象 | **按值**（RVO 零成本） | `operator*`、`make_rational()` |
| 返回**已存在**的对象（寿命超过调用） | const 引用 | 容器 `operator[]`、类的 getter 返成员 |
| 返回**已存在**且可修改 | 引用 | `vector::at`、`map::operator[]` |
| 返回资源**所有权** | 智能指针（值） | `make_unique`、工厂函数 |

判断口诀：**对象是谁的？** 函数自己造的 → 按值；别人的/成员的 → 引用（const 优先）；
所有权要移交 → 智能指针按值。

## HFT 关联

- 行情解码函数返回解析结果：**按值返回小 POD**（<= 16B 通常走寄存器/SRA 完全消失），
  别为了"零拷贝"返回 buffer 引用——接收空间复用应该由**调用方**显式传参解决
  （`decode(const Buf&, Tick& out)` 输出参数模式，热路径更常见）
- 错误三的 static 陷阱在**多线程行情分发**里是真实事故源：
  "上次的价格怎么变了"——static 返回引用 + 多线程 = 数据竞争
- 输出参数 vs 按值返回的选择：大对象/复用缓冲 → 输出参数（避免反复分配）；
  小 POD → 按值返回（代码干净、编译器全消掉）

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

**题目 2：** `Widget make_widget() { Widget w; return std::move(w); }` 比 `return w;` 快吗？

<details>
<summary>参考答案</summary>

**更慢或一样，绝不更快。** `return w;` 触发 NRVO（具名返回值优化），g++/clang 在
-O0 下都能零拷贝（本机实测）。`return std::move(w);` 把 w 显式转右值后，
返回路径变成"移动构造一个临时"——**阻止了 NRVO**，强制造一次移动构造。
C++17 起 RVO 强制，NRVO 不强制但主流编译器全做；写 `return w;` 就好，
move 留给"返回成员变量/参数"这类 NRVO 不适用的场景。

</details>

**题目 3：** 为什么 `operator[]` 可以返回引用而 `operator+` 不行？

<details>
<summary>参考答案</summary>

`operator[]` 返回的是**容器内部已存在的元素**——元素寿命由容器持有，
引用随容器存活（悬垂只在容器先死时发生，那是调用方的责任）。
`operator+` 的结果是**本次调用新造的对象**——不存在的对象没法返引用，
只能按值（RVO 保证零拷贝）。
口诀还是那个：对象是谁的？容器的→引用；函数现造的→按值。

</details>
