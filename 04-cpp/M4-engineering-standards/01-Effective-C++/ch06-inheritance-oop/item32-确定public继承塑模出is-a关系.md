# 条款 32：确定 public 继承塑模出 is-a 关系

## 本节讲什么

**Make sure public inheritance models "is-a."** public 继承不是代码复用工具，
是一句**语义声明**："Derived 的每个对象都是 Base 的一种"——凡是 Base 适用的地方，
Derived 都必须能顶上（LSP，里氏替换原则）。违背后果不是编译错误，
而是**语义灾难**：能编译、能跑、结果是错的。

← 上一条 [item31 降低编译依赖](../ch05-implementations/item31-降低文件之间的编译依赖.md)；
下一条 [item33 避免名称遮掩](./item33-避免遮掩继承而来的名称.md)。

---

## 1. is-a 的检验标准：LSP（里氏替换）

> **凡是接受 Base 的代码，传入 Derived 也必须行为正确。**

```cpp
class Price { public: virtual bool valid() const { return ticks_ > 0; } /*...*/ };
class TickPrice : public Price { /*...*/ };   // TickPrice is-a Price？
// 检验：所有写 valid() 语义（"价格必须为正"）的代码，对 TickPrice 同样成立吗？
```

**编译器只检查类型兼容，不检查语义兼容**——LSP 是你（设计者）向用户签的合同。

## 2. 两个经典反例（能编译的语义灾难）

**反例一：正方形继承矩形**

```cpp
class Rectangle {
public:
    virtual void set_width(int w)  { w_ = w; }
    virtual void set_height(int h) { h_ = h; }
    int area() const { return w_ * h_; }
private: int w_, h_;
};
class Square : public Rectangle {              // "正方形 is-a 矩形"？数学上是的！
public:
    void set_width(int w)  override { w_ = h_ = w; }   // 保持四边相等
    void set_height(int h) override { w_ = h_ = h; }
};
void stretch(Rectangle& r) { r.set_width(10); }   // 用户预期：只改宽，面积 = 10 × 原高
// 传入 Square：高也被改了——行为违反 Rectangle 的语义合同
```

数学上"正方形是矩形"，**代码上不是**——因为代码里的 Rectangle 承诺了
"宽高可独立设置"，Square 守不住。LSP 管的是**行为**不是数学分类。

**反例二：企鹅继承鸟（会飞）**

```cpp
class Bird { public: virtual void fly(); };
class Penguin : public Bird {};      // 企鹅 is-a 鸟——但 fly() 怎么办？
// 选项① fly() 抛异常/打日志 → 违反"鸟会飞"的合同（调用方没准备接异常）
// 选项② fly() 空实现 → 调用方以为企鹅飞走了，其实没有——静默错误更糟
```

## 3. is-a 失败时的三个正确出路

| 真实关系 | 正确建模 |
|---|---|
| has-a（有一个） | **复合**：`class Car { Engine engine_; };`（→ item38） |
| is-implemented-with（以某物实现） | **私有继承**或复合（→ item39） |
| 部分接口共享但语义不同 | **接口拆分**：把"会飞"从 Bird 拆成 `IFlyable`，Penguin 不继承它 |

正方形/矩形的正解：若都需要只读查询，抽 `IShape` 接口（`area()`）；
可写的 Rectangle 和 Square 各自独立——**共享查询接口，不共享可写语义**。

## 4. 与"实现复用"的混淆（public 继承不是干这个的）

想要 Base 的**代码**而不想要它的**语义** → 用复合（item38）或私有继承（item39）。
public 继承把 Base 的接口**全部承诺**给用户——包括你不想要的那些。
"Stack 继承 vector"是教科书级反面教材：vector 承诺按下标随机访问，
Stack 守不住（也不该守）——`class Stack { std::vector<T> impl_; }` 才对。

## HFT 关联

- **消息类型层次**：`SnapMsg is-a FeedMessage` 成立的前提是——
  所有处理 FeedMessage 的代码（路由/落盘/回放）对 SnapMsg 语义兼容；
  若 SnapMsg 的 `seq` 语义与 TradeMsg 不同（快照序号 vs 逐笔序号），
  统一基类的 `seq()` 接口就是 LSP 地雷——拆成各自访问器
- 策略框架里 `class MyStrat : public Strategy`：基类的 `on_tick/on_order` 回调
  语义（调用时机/线程/禁止阻塞）是合同——派生类在回调里做阻塞 IO 就是 LSP 违反，
  引擎的超时假设被破坏
- 继承审查口诀：**写 `class D : public B` 时，把"凡是 B 能用的场景 D 都必须对"
  念一遍**——念不通就改复合

## 代码自测

**题目 1：** 数学上"正方形是矩形"，为什么 `class Square : public Rectangle` 仍然错了？

<details>
<summary>参考答案</summary>

public 继承承诺的是**行为可替换性**（LSP），不是数学分类。
代码里的 Rectangle 提供了"宽高独立设置"的语义（`set_width` 不改高），
而 Square 的不变量是"四边相等"——这两个合同**互不兼容**：
接受 Rectangle 的代码（`stretch`）传入 Square 后行为被静默改变。
数学上的"是"管的是集合包含；LSP 管的是**可观察行为**——两者只在
类型不可变（只读）时才重合，可写对象几乎必然冲突。

</details>

**题目 2：** `class Stack : public std::vector<T>` 错在哪？正确做法？

<details>
<summary>参考答案</summary>

错在 is-a 不成立：vector 的接口承诺（任意位置 insert/erase/下标访问）
Stack 根本不想提供——继承把这些承诺**全部公开**给了用户，
`stack[3] = x; stack.insert(...)` 都能编译，栈的不变量（只能顶进顶出）裸奔。
正确做法：**复合**——`class Stack { std::vector<T> impl_; public: push/pop/top; }`，
只暴露栈该有的接口，vector 沦为实现细节（item38）。
要"复用代码"选复合/私有继承，要"承诺接口"才选 public 继承。

</details>

**题目 3：** 交易系统里 `class IocOrder : public Order`，但 IOC（立即成交否则取消）
订单**不允许** `modify_price()`——基类 Order 有这个虚函数。三个处理方案，哪个不违反 LSP？

<details>
<summary>参考答案</summary>

方案一（`modify_price` 抛异常）和方案二（空实现/no-op）**都违反 LSP**：
接受 Order 的代码（改价引擎）没准备接异常/没预期静默失败——语义被偷袭。
方案三（**接口拆分**）正确：把 `modify_price()` 从 Order 基类移到
`IModifiable` 子接口，普通限价单继承它，IocOrder 不继承——
改价引擎接受 `IModifiable&`，IOC 订单在**编译期**就进不了改价流程。
原则：不能替换的语义就别继承——把差异变成类型系统能检查的东西，
而不是运行时才能发现的坑。

</details>
