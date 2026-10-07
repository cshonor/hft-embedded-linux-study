# 条款 43：学习处理模板化基类内的名称

## 本节讲什么

**Know how to access names in templatized base classes.** 派生类模板继承
**模板基类**时，基类的成员（函数/类型）直接写名字**查不到**——
编译器报 "there are no arguments that depend on a template parameter"。
这是 C++ **两阶段查找**（two-phase lookup）的直接后果，三个解法各有适用场景。
本机 g++ 13.3 实测三种解法均通过。

← 上一条 [item42 typename 双重意义](./item42-了解typename的双重意义.md)；
下一条 [item44 抽离参数无关代码](./item44-将与参数无关的代码抽离templates.md)。

---

## 1. 现象：模板基类的成员"消失"了

```cpp
template <typename T>
struct Base { void hello() { std::cout << "Base<T>::hello\n"; } };

template <typename T>
struct Derived : Base<T> {
    void greet() {
        // hello();          // ❌ 编译错误：hello 未声明——基类的成员哪去了？
        this->hello();       // ✅ 解法①
    }
};
```

**为什么查不到（两阶段查找）**：
- **第一阶段**（模板定义时）：查找**不依赖**模板参数的名字——
  此时编译器不知道 `Base<T>` 会被特化成什么样（可能有个特化 `Base<int>`
  根本没有 hello！），所以**不查模板基类**
- **第二阶段**（实例化时）：查依赖名——但 `hello()` 写成裸调用时，
  它在第一阶段就被判定为"不依赖"而错过了

编译器的谨慎是合理的：`Base<T>` 的全特化可能完全改变基类长相，
裸 `hello()` 到底是基类的还是全局的，必须到实例化才知道——
而你要做的是**把这个名字标记成依赖名**，推到第二阶段。

## 2. 三个解法（本机实测全部通过）

```cpp
// 解法① this->（最常用）：this 依赖 T，其后的名字自然进入第二阶段
this->hello();

// 解法② using 声明：把基类名字引入派生类作用域
using Base<T>::hello;
hello();

// 解法③ 显式限定：写清从哪来
Base<T>::hello();
```

| 解法 | 适用 | 注意 |
|---|---|---|
| `this->hello()` | **默认首选**——一处一处标，意图清晰 | 对虚函数保留多态（经 this 是虚调用） |
| `using Base<T>::hello` | 基类同名重载多（整个重载集引入） | 引入的是**全部**重载 |
| `Base<T>::hello()` | 偶尔一处、或要明确"就用基类版" | ⚠ **关闭虚函数多态**（显式限定 = 静态绑定） |

## 3. 孪生兄弟：依赖名是模板时的 `.template` 语法

名字依赖参数且是**模板**时，还要再加一个消歧（item42 ④ 预告的兄弟）：

```cpp
template <typename T>
void f(T& obj) {
    obj.template convert<int>();     // template 关键字必须——
}                                    // 否则 '<' 被解析成小于号：((obj.convert) < int) > ()
```

规则成对记：**依赖名是类型 → `typename`**；**依赖名是模板 → `template`**。
allocator 代码里 `alloc.template rebind<U>::other` 是两个消歧同台的经典现场。

## 4. 与名称遮掩（item33）的关系

普通继承的遮掩（item33）发生在**非模板**类：派生类同名函数藏掉基类重载。
模板基类的问题是**更底层**的：名字连"被查"的资格都没有（两阶段查找）。
`using Base<T>::f;` 这个解法和 item33 的 `using Base::f;` 写法相同、目的不同：
- item33：把被遮掩的**已有**名字拉回候选集
- item43：把**尚未进入查找**的名字标记为依赖名

两者经常同时出现（模板 + 继承的代码里），知道根因不同才能对症下药。

## HFT 关联

- CRTP 基类（`StrategyBase<Derived>`）是模板基类的主战场：
  派生策略调基类的 `send_order()`/辅助函数时必须 `this->`——
  CRTP 代码里满屏 `this->` 不是风格怪癖，是两阶段查找的硬要求
- allocator/池模板的 `rebind` 家族是 `.template` 消歧的日常——
  写自己的池分配器时 `Alloc::template rebind<T>::other` 是绕不开的形态
- 模板库的阅读路径：看到 `this->` 密集出现 = 模板基类；
  看到 `typename` 密集 = 依赖类型——两个关键字是模板代码的"路标"

## 代码自测

**题目 1：** 为什么派生类模板里裸写 `hello()` 查不到 `Base<T>::hello()`？

<details>
<summary>参考答案</summary>

C++ 两阶段查找：模板**定义时**（第一阶段）只查不依赖模板参数的名字——
`Base<T>` 可能被特化（特化版可能没有 hello），编译器保守地**不查模板基类**，
裸 `hello()` 被判为未声明。到实例化（第二阶段）才查依赖名——
但裸写法的查找时机已经错过。
解法是让名字"依赖化"：`this->hello()`（this 依赖 T）、
`using Base<T>::hello`（显式引入）、`Base<T>::hello()`（显式限定）——
三种写法都把查找推迟到第二阶段（本机实测全通过）。

</details>

**题目 2：** 三个解法里哪个会关闭虚函数多态？为什么？

<details>
<summary>参考答案</summary>

**`Base<T>::hello()` 显式限定**。`X::f()` 的显式限定调用是**静态绑定**——
编译器直接生成"调 Base<T>::hello"的代码，绕过 vtable；
即使 hello 是虚函数、对象是派生类，也调不到 override 版。
`this->hello()` 和 `using` 版保留正常的虚分发（经对象调用）。
推论：想"显式用基类实现"时显式限定正合适（item34 的 `Shape::draw()` 用法）；
想保留多态就必须走 `this->` 或 using。

</details>

**题目 3：** `obj.template convert<int>()` 里的 `template` 和 `typename T::type` 里的
`typename` 各自解决什么问题？

<details>
<summary>参考答案</summary>

一对孪生消歧，管的都是"**依赖名**"（`::`/`.` 左边依赖模板参数）：
- `typename`：声明"这个依赖名是**类型**"——否则编译器默认它是值
  （`T::iterator * x` 被当成乘法）；
- `template`：声明"这个依赖名是**模板**"——否则 `<` 被当成小于号
  （`obj.convert < int > ()` 被解析成两个比较运算）。
都是"编译器在实例化前无法确定，需要程序员消歧"——
allocator 的 `a.template rebind<U>::other` 是两者同台的经典例
（rebind 是模板 → template；other 是类型 → 整体若要再取类型还要 typename）。

</details>
