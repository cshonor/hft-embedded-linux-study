# 条款 34：区分接口继承和实现继承

## 本节讲什么

**Differentiate between inheritance of interface and inheritance of implementation.**
public 继承其实继承了两样东西：**接口**（函数的声明，"必须提供"）和
**实现**（函数体，"可以直接用"）。四种虚函数形态对应四种不同的继承意图——
选错形态，派生类就会被强迫继承不该继承的东西。

← 上一条 [item33 避免名称遮掩](./item33-避免遮掩继承而来的名称.md)；
下一条 [item35 虚函数以外的选择](./item35-考虑virtual函数以外的其他选择.md)。

---

## 1. 四种形态 = 四种意图（对号入座）

| 形态 | 继承接口 | 继承实现 | 语义 | 例子 |
|---|---|---|---|---|
| **纯虚**（`= 0`） | ✅ 强制 | ❌ | "你必须提供这个能力" | `virtual void draw() = 0;` |
| **非纯虚**（虚 + 默认实现） | ✅ 强制 | ✅ 可选 | "默认这么干，不满意可换" | `virtual void log() { /*默认*/ }` |
| **非虚** | ✅ 强制 | ✅ 强制 | "接口和实现都不许动" | `int id() const { return id_; }` |
| **NVI**（非虚外壳 + 私有虚） | ✅ 强制 | ✅ 受控 | "框架管骨架，你只填定制点" | → item35 实测 |

**关键区分**：纯虚函数**也可以有实现**——

```cpp
class Shape {
public:
    virtual void draw() const = 0;   // 纯虚：接口强制
};
void Shape::draw() const { /* 默认渲染逻辑 */ }   // 但能给实现！
// 派生类必须 override，但可以显式调用 Shape::draw() 复用默认逻辑：
class Circle : public Shape {
    void draw() const override { Shape::draw(); /*+ 自己的*/ }
};
```

"纯虚 = 没有实现"是常见误解；真实语义是"**派生类必须显式表态**"
（要 override，但可以用基类提供的默认零件）。

## 2. 选错形态的代价

**意图"必须定制"却给了默认实现（非纯虚）**：

```cpp
class FeedHandler {
public:
    virtual void on_message(const Msg& m) { /* 默认：丢弃 */ }   // ❌ 危险默认值
};
class MyHandler : public FeedHandler {
    // 忘了 override on_message——编译通过，运行期消息全被静默丢弃
};
```

写成**纯虚**就能让"忘记实现"变成编译错误——**强制派生类表态**。

**意图"不许动"却用了虚函数**：虚函数的默认语义是"可以被 override"——
想表达"行为契约，不许改"就用**非虚**（或 NVI，把定制点收窄到明确的位置）。

## 3. 与"实现复用"混淆的解毒（回到 item32）

想继承**实现**（复用代码）而不想要接口（不做 is-a 承诺）——
那根本不该用 public 继承：**复合**（item38）或**私有继承**（item39）。
public 继承的第一身份是接口继承（item32 的 LSP 合同），
实现继承只是顺带的赠品——把赠品当主菜，设计就开始腐烂。

## HFT 关联

- **策略框架的回调必须纯虚**：`virtual void on_tick(const Tick&) = 0;`——
  策略忘实现某个回调必须编译期爆炸，不能"默认丢弃"
  （默认空实现的 on_order 让"策略没收到回报"变成静默事故）
- 引擎的**骨架行为用非虚/NVI**：`Strategy::run()` 非虚（事件循环骨架不许改），
  定制点 `do_on_tick()` 私有虚（→ item35 实测）——
  这正是本条第四种形态在真实框架里的样子
- 协议解码器族：共同解码骨架（非虚）+ 协议定制点（纯虚 `decode_body()`）——
  形态四（NVI）比"非纯虚默认可覆盖"更能防止"策略覆盖了不该覆盖的骨架"

## 代码自测

**题目 1：** 纯虚函数可以有实现吗？如果有，意义是什么？

<details>
<summary>参考答案</summary>

可以。`virtual void f() = 0;` 后仍可在类外写 `void Base::f() { ... }`。
意义：纯虚的语义是"**派生类必须显式 override**"（接口强制），
但基类仍可提供一份**默认零件**——派生类 override 时通过 `Base::f()` 显式调用复用。
典型场景：基类想把"骨架逻辑"共享出来，又不希望派生类"忘记表态"——
比非纯虚（静默继承默认）更能强迫设计意图显式化。

</details>

**题目 2：** 什么时候该把虚函数设计成"非纯虚 + 默认实现"而不是纯虚？

<details>
<summary>参考答案</summary>

当**默认行为对绝大多数派生类是正确且安全的**时：
例如 `virtual void on_disconnect() { reconnect(); }`——
断线重连是 90% 场景的合理默认，个别策略（高频做市断线即停）才需要覆盖。
判别标准：**默认行为被静默继承时，出错的代价可接受吗？**
可接受 → 非纯虚；不可接受（像消息被丢弃、订单被重复）→ 纯虚强制表态。
"忘了 override 的后果"决定形态，不是"有没有默认逻辑可写"。

</details>

**题目 3：** 想让"事件循环骨架"对所有策略不可改，但允许定制 tick 处理——用哪种形态？

<details>
<summary>参考答案</summary>

**NVI（形态四）**：

```cpp
class Strategy {
public:
    void run() {                 // 非虚：骨架锁死，不许覆盖
        while (running_) { pre_risk(); do_on_tick(fetch()); post_log(); }
    }
private:
    virtual void do_on_tick(const Tick&) = 0;   // 私有纯虚：唯一定制点
};
```

`run()` 非虚 → 骨架不可覆盖（接口+实现双锁定）；
`do_on_tick` 私有纯虚 → 派生类必须实现，但**只能在骨架规定的位置**被调用——
风控前置、日志后置这些引擎纪律不可能被策略"顺手覆盖"绕开（→ item35 实测）。
这就是"接口继承与实现继承分离"在框架设计里的标准答案。

</details>
