# 条款 23：杜绝向下转型（downcasting），它破坏多态设计初衷

## 本节讲什么

**Eliminate downcasts.** 向下转型（基类 → 派生）是多态的**反面**：
多态的意义是"调用方不需要知道具体类型"，向下转型却逼调用方
"先查明具体类型再区别对待"——用了它，虚函数的存在价值就被自己人拆了。
本条给出向下转型的三个替代方案（→ Effective item27 治本手法、18.4 RTTI 的联动）。

← 上一条 [item22 接口 vs 实现继承](./item22-区分接口继承和实现继承，很多继承设计错误根源就在这里.md)；
下一条 [item24 虚函数与 MI 的开销](./item24-理解虚函数、多重继承带来的内存布局、开销、歧义问题.md)。

---

## 1. 为什么向下转型是设计败笔

```cpp
void process(MsgBase* m) {
    if (auto* s = dynamic_cast<SnapMsg*>(m)) { handle_snap(s); }
    else if (auto* t = dynamic_cast<TradeMsg*>(m)) { handle_trade(t); }
    // 每加一种消息类型，这里就要加一个分支——
    // 类型知识散落在每个 process 里，多态的"类型透明"荡然无存
}
```

三宗罪：
1. **性能**：dynamic_cast 沿继承链运行时遍历（→ 18.4 ③：cache miss + 链越长越慢）
2. **维护**：类型集膨胀时，每个转型链都要手动加分支（漏一个 = 静默跳过）
3. **设计**：它宣告"虚函数分发失败"——行为没有挂在类型上，
  而是挂在调用方的 if-else 上（→ Effective item27 ④ 的"教科书级坏味道"）

## 2. 三个替代方案（按类型集开放性选）

**方案一：虚函数**（类型集开放）

```cpp
class MsgBase { public: virtual void handle(Engine&) = 0; };
class SnapMsg : public MsgBase { void handle(Engine& e) override { /*...*/ } };
m->handle(engine);          // 行为回挂到类型上——新增类型不用改任何调用方
```

**方案二：variant + visit**（类型集封闭，→ Effective item35、item54）

```cpp
using Msg = std::variant<SnapMsg, TradeMsg, HeartbeatMsg>;
std::visit(overloaded{
    [&](const SnapMsg& s) { /*...*/ },
    [&](const TradeMsg& t) { /*...*/ },
    [](const HeartbeatMsg&) { /*...*/ },
}, msg);
// 编译期分发：无 vtable、无 RTTI、可内联；漏一个分支 = 编译错误（穷尽性检查）
```

**方案三：查表**（协议场景，类型由字段标识）

```cpp
static constexpr Handler handlers[] = { &on_snap, &on_trade, &on_heartbeat };
handlers[msg.type](msg);    // msg_type 字段 + 数组索引（→ 19.3 成员指针表）
```

## 3. 向下转型的"合法残余"（诚实清单）

不是绝对禁区，以下场景它仍有位置：
- **缓存后一次转型**：入口处 dynamic_cast 一次，缓存结果到类型化通道
  （之后全程静态类型）——转型成本摊一次
- **C 回调边界**：`void*` 用户数据 cast 回具体类型（→ Effective item27 HFT）——
  C ABI 没有类型系统，这是边界税不是设计选择
- **调试/测试代码**：断言类型用（`assert(dynamic_cast<...>(p))`）

## HFT 关联

- 行情分发**禁用** dynamic_cast 链（18.4 HFT 同一结论）：
  协议 msg_type + 查表（方案三）是标准答案——一次数组索引，
  间接跳转目标规律（BTB 友好），且协议加类型 = 加表项不动分发代码
- 策略内部"这批订单里挑 IOC 单"：先想能不能在**订单创建时**就分通道
  （IOC 队列/Limit 队列分开）——类型知识在源头分好，
  下游永远不需要向下转型（类型分桶是交易系统的本能设计）
- variant + visit 的穷尽性检查在协议升级时是编译期验收：
  新消息类型加进 variant，所有没处理的 visit 直接编译错误——
  比 if-else 链"静默跳过新类型"安全一个时代

## 代码自测

**题目 1：** 为什么说向下转型"拆了多态的台"？

<details>
<summary>参考答案</summary>

多态的承诺是**类型透明**：调用方拿着基类接口，不需要知道、
也不该关心对象的具体类型——行为差异由虚函数在类型内部消化。
向下转型要求调用方**先查明具体类型**（dynamic_cast），
再按类型分派行为——类型知识从"类型内部"泄漏到"每个调用方"：
新增派生类时，所有转型链都要手动加分支（漏一个静默跳过）；
虚函数的存在价值（一处 override、处处生效）被架空。
用了向下转型，等于为多态付了全部成本（vptr/vtable/间接），
却放弃了它的全部收益——两头落空。

</details>

**题目 2：** variant + visit 相比 dynamic_cast 链，三个优势？

<details>
<summary>参考答案</summary>

① **编译期分发**：visit 展开为按索引的直接调用（甚至内联）——
无 vtable、无 RTTI 链遍历，热路径成本与查表同级；
② **穷尽性检查**：variant 加新类型后，没处理它的 visit 直接**编译错误**——
协议升级的验收在编译期，if-else 链则是"静默跳过"（运行时才发现）；
③ **类型知识集中**：类型集合是 variant 声明的一部分
（`variant<Snap, Trade, Heartbeat>`）——一目了然且不可扩散，
dynamic_cast 链的类型知识散落在每个调用点。
代价：类型集必须**封闭**（variant 写死成员类型）——
开放扩展（插件注册新类型）仍要虚函数（→ Effective item35 的选型表）。

</details>

**题目 3：** "IOC 订单要在订单列表里被特殊处理"，不用向下转型的两个设计？

<details>
<summary>参考答案</summary>

① **源头分桶**：创建时就按类型入不同容器——
`ioc_orders_` / `limit_orders_` 两个队列，处理各自遍历自己的桶——
类型知识在订单进入系统的那一刻分好，下游永远静态类型
（这是交易系统的本能：订单类型决定队列拓扑）；
② **行为前置**：把"特殊处理"定义为订单的虚函数/策略
（`order->on_risk_check(risk)`——IOC 订单的 override 自然不同），
或创建时注入行为标志（`order.flags |= IOC_BIT`，
处理处按 flag 分支——位测试比 dynamic_cast 便宜且类型无关）。
反模式就是"遍历基类列表 + dynamic_cast<IocOrder*>"——
类型知识泄漏 + RTTI 成本 + 每加订单类型多一个分支的三连。

</details>
