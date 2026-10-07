# 条款 33：避免遮掩继承而来的名称

## 本节讲什么

**Avoid hiding inherited names.** 派生类声明一个与基类**同名**的函数，
会把基类的**所有**同名重载全部藏掉——哪怕参数列表不同。
这不是"覆盖"，是作用域规则下的**名称遮掩**（name hiding）。
修法：`using` 声明把基类重载拉回视野。本机 g++ 13.3 实测遮掩与修复。

← 上一条 [item32 is-a 关系](./item32-确定public继承塑模出is-a关系.md)；
下一条 [item34 接口继承 vs 实现继承](./item34-区分接口继承和实现继承.md)。

---

## 1. 现象：同名即全部遮掩（本机实测）

```cpp
struct Base {
    virtual void f() {}
    virtual void f(int x) {}       // 两个重载
};
struct Derived : Base {
    void f() override {}           // 只想覆盖无参版
};
Derived d;
d.f();        // ✅
// d.f(42);   // ❌ 编译错误：Base::f(int) 被 Derived::f 藏掉了！
```

**规则**（作用域查找）：编译器在 `Derived` 作用域找到名字 `f` 就**停止查找**——
不再去基类找其他重载。名字查找**先于**重载决议：
"`f` 找到了（Derived::f）→ 重载决议（只有无参版）→ 42 匹配不上 → 编译错误"。
基类的 `f(int)` 从头到尾没进入候选集。

**与覆盖（override）的区别**：覆盖是"同一签名的虚函数替换实现"（运行期多态）；
遮掩是"同名即隐藏一切"（编译期查找规则）——哪怕签名不同、哪怕基类是虚函数。

## 2. 修法：using 声明拉回基类重载（实测）

```cpp
struct DerivedFix : Base {
    using Base::f;                 // 把 Base 的所有 f 重载引入本作用域
    void f() override {}           // 再覆盖自己关心的那个
};
DerivedFix df;
df.f();       // ✅ DerivedFix::f()
df.f(42);     // ✅ Base::f(int)——本机实测输出 "Base::f(int) 42"
```

**public 继承下，遮掩几乎总是 bug**：is-a 意味着基类的接口应该全部可用（item32），
`using` 声明就是"保留接口"的声明。

## 3. 什么时候遮掩是有意的（罕见但存在）

- **私有继承**（item39）：本来就不想继承接口，遮掩反而是"特性"——
  只 `using` 你想暴露的个别重载，其余自然藏掉
- **有意的接口收窄**：派生类明确要禁用基类的某些重载
  （但更干净的做法是 item32 的接口拆分——遮掩是隐式收窄，reviewer 容易漏看）
- **基类模板**（→ ch07 item43）：模板基类的名称查找还有两阶段查找的坑，
  `this->f()` 或 `using Base<T>::f;` 是标准解法

## 4. 快速自查清单

代码里出现以下任一形态，立刻想到本条：

```cpp
// ① 派生类只覆盖部分重载
struct D : B { void f() override; };          // B 还有其他 f 重载吗？

// ② 派生类新增与基类同名的非虚函数
struct D : B { void log(int); };              // B::log(const char*) 被藏了

// ③ 三参数的警告：-Woverloaded-virtual（g++/clang）——
//    派生类函数与基类虚函数同名但签名不同，直接告警，务必打开
```

## HFT 关联

- **策略基类的回调重载**是重灾区：`Strategy::on_tick(const Tick&)`
  与 `on_tick(const Tick&, SeqNo)` 两个重载，策略只覆盖一个——
  另一个被遮掩后引擎调用落空，**编译期静悄悄、运行期没回调**；
  基类加 `using` 约定 + `-Woverloaded-virtual` 双重防线
- 协议解码器的版本演进：v2 解码器覆盖 `decode()` 但忘了 `using Base::decode`——
  老接口调用方集体编译错误，好在编译错误比静默遮掩好发现
- 名称遮掩与 ADL（18.2 ⑤）叠加时最绕：遮掩发生在普通查找阶段，
  ADL 救不回来——`this->` 或 using 是唯一解

## 代码自测

**题目 1：** 为什么 `Derived::f()` 会把 `Base::f(int)` 也藏掉？它们签名明明不同。

<details>
<summary>参考答案</summary>

因为 C++ 的名字查找**先于**重载决议，且按**作用域**进行：
编译器从内向外逐层找"名字叫 f 的声明"，在 Derived 作用域找到 `Derived::f` 就
**停止**（不会继续到 Base 作用域收集其他重载），然后只在已找到的集合里做重载决议。
集合里只有无参版，`f(42)` 自然匹配失败。
"同名即全部隐藏"是查找规则，与签名、虚函数、参数都无关（本机实测复现）。

</details>

**题目 2：** 想把基类的所有重载暴露给派生类用户，正确写法是什么？

<details>
<summary>参考答案</summary>

```cpp
struct Derived : Base {
    using Base::f;          // using 声明：把 Base 作用域里的所有 f 引入 Derived 视野
    void f() override;      // 再覆盖自己需要定制的那个
};
```

using 声明把基类的**整个重载集**并回候选集——本机实测后 `df.f()` 走覆盖版、
`df.f(42)` 走基类版。public 继承下这应该成为"覆盖部分重载"时的默认动作
（is-a 要求基类接口完整可用，item32）。

</details>

**题目 3：** 私有继承时，只想暴露基类的 `f()` 和 `g()`，不想暴露其他几十个成员——怎么做？

<details>
<summary>参考答案</summary>

私有继承下基类的 public 成员**全部**变成派生类的 private——用户本来就用不了。
此时遮掩不是问题而是工具：在派生类的 public 区写

```cpp
class D : private B {
public:
    using B::f;     // 只把这两个重载提回 public
    using B::g;
    // B 的其余成员保持 private（被私有继承"藏"着，正合心意）
};
```

这就是"只暴露选定接口"的标准手法（item39 私有继承的配套）——
using 声明在 public 继承里是"防遮掩"，在私有继承里是"选择性曝光"。

</details>
