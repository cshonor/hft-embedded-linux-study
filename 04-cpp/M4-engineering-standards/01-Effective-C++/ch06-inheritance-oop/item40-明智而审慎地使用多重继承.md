# 条款 40：明智而审慎地使用多重继承

## 本节讲什么

**Use multiple inheritance judiciously.** MI 是 C++ 争议最大的特性——
它的机制与代价（布局/this 调整/菱形/虚继承）已在 **18.3 详述（含本机实测）**，
本条聚焦**使用决策**：MI 的两个合法用途、名字冲突的处理、以及
"MI 恐惧症"与"MI 滥用"之间的明智位置。

← 上一条 [item39 慎用 private 继承](./item39-明智而审慎地使用private继承.md)；
下一章 [ch07 模板与泛型](../ch07-templates-generics/)。

---

## 1. MI 的两个合法用途（Meyers 的判决）

**用途一：public 接口继承 + private 实现继承的组合**

```cpp
class IOrderGateway {                       // 接口（纯虚，无数据）
public:
    virtual ~IOrderGateway() = default;
    virtual void send(const Order&) = 0;
};
class FixGateway : public IOrderGateway,    // public：is-a 接口承诺
                   private FixSession {     // private：以 FIX 会话实现（item39）
    void send(const Order& o) override {
        session_send(encode(o));            // 用私有基类的实现零件
    }
};
```

**用途二：多个"接口"的混入（mixin）**——第二及以后的基类全是纯虚无数据：

```cpp
class Strategy : public MdSink,             // 行情回调接口
                 public OrderSink,          // 订单回报接口
                 public TimerSink {         // 定时器接口
    // 三个接口都是纯虚 + 无数据——没有菱形风险，没有布局复杂性
};
```

**判别**：MI 里**最多一个基类带数据/实现**，其余都是纯接口——
越过这条线（两个带数据的基类），菱形和布局问题就开始收利息（→ 18.3 ②）。

## 2. 名字冲突与二义性（MI 的日常）

```cpp
struct A { void log(); };
struct B { void log(); };
struct C : A, B {};
C c;
// c.log();        // ❌ 二义性编译错误
c.A::log();        // ✅ 显式限定
```

处理手法：
- **调用处限定**：`c.A::log()`——临时方案
- **派生类内 using/转发**：`using A::log;` 或定义 `C::log()` 消除二义——
  把冲突解决在**类内**而不是每个调用点
- **设计预防**：两个基类的同名成员是 MI 的固有摩擦——
  接口设计时统一命名约定（`on_md/on_order` 而不是都叫 `on_event`）

## 3. 菱形：知道它，然后绕开它（18.3 回顾要点）

菱形继承（A→B, A→C, D:B,C）下 A 存两份——解法虚继承，代价：
虚基指针 + 最派生类负责构造虚基 + 向下转型查虚基表（→ 18.3 ② 实测 sizeof 数据）。
**工程结论**：与其精通虚继承，不如**让菱形在设计上不出现**——
接口 mixin 模式下基类无数据，菱形根本无从形成。
虚继承是"已经撞上了"的补救，不是"设计工具箱"的一员。

## 4. 与替代方案的对比（什么时候连接口 MI 也不用）

| 需求 | MI 接口混入 | 替代方案 |
|---|---|---|
| 一个类吃多种回调 | ✅ 自然（Strategy 例） | 多个成员函数对象注册（更松但样板多） |
| 编译期已知的能力组合 | CRTP mixin（零虚函数开销） | → item35 替代三 |
| 运行期可插拔能力 | 组合 + std::function | → item35 替代二 |

热路径的能力组合优先 **CRTP mixin**（编译期，无 vptr）；
接口 MI 留给"运行期异构容器"（引擎管理 N 种策略）。

## HFT 关联

- **策略类的多接口形态**（Strategy : MdSink, OrderSink, TimerSink）是
  交易框架的标准长相——接口纯虚无数据，避开 18.3 的全部代价
- **回调注册注意 this 调整**（→ 18.3 ① 实测 +4 偏移）：C 风格回调
  （`void*`）配 MI 是经典 crash 源——注册前必须按接口类型 cast 好 this
- 网关类 `FixGateway : public IGateway, private FixSession` 是
  "接口继承 + 实现复用"的教科书组合——接口给引擎看，实现藏给自己用

## 代码自测

**题目 1：** "MI 里最多一个基类带数据"这条纪律背后的推理是什么？

<details>
<summary>参考答案</summary>

MI 的代价几乎全部来自**数据**：菱形（数据重复存两份，要虚继承补救）、
布局复杂（每个带数据的基类各带 vptr/成员，this 调整分散）——
纯接口基类（纯虚 + 无数据）则**不产生这些代价**：没有数据就没有菱形，
接口的 vtable 合并由编译器处理。
所以纪律的实质是：**用 MI 分发接口（零代价），不用 MI 聚合数据（全代价）**。
两个带数据的基类出现在继承表里 = 设计评审该亮红灯了（→ 18.3 的实测代价清单）。

</details>

**题目 2：** `class C : public A, public B` 中 A、B 都有 `log()`，三个消除二义的办法，各自适用场景？

<details>
<summary>参考答案</summary>

① **调用处限定** `c.A::log()`：临时/一次性调用——把冲突成本摊到每个调用点，
适合"极少用到的那一个"；
② **using 声明** `using A::log;`（在 C 内）：明确"以 A 版为准"——
B 版被遮掩（item33），适合两版本语义等价、选谁都行；
③ **C 内定义自己的 log()**（内部可分别调 A::/B::log 或实现新语义）：
两版本语义不同、C 需要统一出口时——把冲突解决在类内，
用户代码永远不用知道分歧存在。
设计预防优于三者：接口命名时就差异化（on_md/on_order 而非都叫 handle）。

</details>

**题目 3：** 热路径需要"策略既吃行情回调又吃定时器回调"，给两个方案并说明取舍。

<details>
<summary>参考答案</summary>

① **接口 MI**（`class S : MdSink, TimerSink`）：运行期多态，能进异构容器
（引擎 `vector<Strategy*>` 统一管理）；代价是每个对象带多个 vptr、
回调经 vtable 间接 + this 调整（→ 18.3 ①）。
② **CRTP 双 mixin**（`class S : MdSink<S>, TimerSink<S>`）：编译期解析、
零 vptr、回调可内联——热路径性能上限更高；代价是不能异构存储
（`MdSink<A>` 与 `MdSink<B>` 类型不同），且每实例化一份代码。
取舍：**引擎需要统一调度多种策略 → ①；单一策略类型的极致热路径 → ②**。
真实系统常混用：框架边界用①，策略内部热分发用②（→ item35 组合用法）。

</details>
