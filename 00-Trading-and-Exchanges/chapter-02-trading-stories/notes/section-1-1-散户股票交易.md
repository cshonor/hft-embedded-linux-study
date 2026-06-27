## 1. 散户股票交易 (Retail Equity Trades)

### NYSE 上市股票

散户 Jennifer 购买 200 股 AT&T：

1. 经纪人查看 **综合报价系统**（最优买卖价 BBO）
2. 可选 **市价单 (Market order)** 或 **限价单 (Limit order)**
3. 订单经 **SuperDot** 路由至 NYSE 大厅 **专家 (Specialist / 做市商)**
4. 在 **电子限价指令簿** 中等待撮合，或由专家自行促成

| 概念 | HFT 关联 |
|------|----------|
| BBO / 限价簿 | 现代 LOB 的原型；HFT 竞争 queue position |
| Specialist | 演化 today's designated market maker (DMM) |
| SuperDot | 早期电子路由 →  today's direct market access |

### Nasdaq 股票

Jennifer 购买微软 (Nasdaq)：

1. 查看 **Nasdaq Level II**——多做市商 + ECN 分层报价
2. 订单路由至特定做市商（可能含 **订单流返点 payment for order flow**）
3. 或通过 **SuperSOES** 等自动执行系统成交

| 概念 | HFT 关联 |
|------|----------|
| Level II 深度 | 订单簿多档报价；HFT 读 full book |
| ECN | 与交易所竞争的电子化 venue |
| 订单流返点 | 零售流 vs 机构流；HFT 通常走 DMA 不经此路径 |

---

### 与 go-dex：理解订单簿的最好切入点

**散户故事 = 你撮合引擎里最小闭环** — Jennifer 的 200 股，换成 BTC/USDT，逻辑一样：

| 书上（散户） | 你的代码（go-dex） |
|--------------|-------------------|
| **限价单 / 市价单** | `Order.OrderType`（`OrderLimit` / `OrderMarket`）→ `Match()` **先分支**，再价格–时间优先（[orderbook.go](../../00-practice-go-dex/code/orderbook.go)） |
| **电子限价指令簿** | `Orderbook` · `Bids` / `Asks` · 每档 `Limit.Orders` 队列 |
| 订单 **挂进簿里等** / **立刻吃对手盘** | 限价 → 进簿；市价 → `Match()` walk-the-book（M2 待写） |
| 专家 / 做市商 **接单、报价、消化散户流** | 簿上 **挂着的限价单** 就是「等人来吃」；做市商 = 大量挂限价 + 快速撤改 |

**Jennifer 的单在市场里怎么流转（对应你写的引擎）：**

```
散户下单（限价/市价）
    → 路由进交易所（你：API 进 Orderbook）
    → 限价：挂到 Bids/Asks 某档，按 Timestamp 排队
    → 市价：从最优对手档开始扫（价格优先 → 时间优先）
    → 成交 → 更新 TotalVolume / 删空档 → （以后）推送行情
```

**和 Ch 1 §4.2「限价单被吃」怎么接上：**

- 散户（或你）在 **10500 挂限价卖**，以为「涨到了再卖」  
- **做市商 / 大资金** 的角色，就是书上 **专家、Nasdaq 做市商** — 他们 **盯着簿上的挂单**，用 **市价扫单、推价** 等方式 **消化** 或 **触发** 这些单  
- 你在 §4.2 慢动作里写的「先托价 → 市价买扫你 → 再砸盘」，就是 **「你的限价单最后怎么被别人接走」** 的微观版；Jennifer 的故事告诉你 **这类单在真实市场里本来就会流向做市商**

| 读完本节应懂 | 对照代码 |
|--------------|----------|
| 限价单 = 挂在簿上提供 **免费期权** | `Limit` + `Order` 进 `Bids`/`Asks` |
| 市价单 = **立刻** 找对手成交 | `Match()` 扫对手盘 |
| 做市商 = 吃/挂散户流，不是魔法 | 对手方也是 **簿上的 Order**，或 **incoming 主动单** |

#### SuperDOT → 订单优先级：路由也是撮合逻辑的一部分

书上 Jennifer 的订单经 **SuperDot** 送进 NYSE — 本质是早期的 **「谁先被交易所处理」**：

| 时代 | 机制 | 在问什么问题 |
|------|------|--------------|
| **SuperDot（NYSE）** | 电子路由进大厅 / 专家 | 订单 **从哪条通道进**、**先处理谁** |
| **SuperSOES（Nasdaq）** | 自动执行系统 | 能否 **绕过** 人工、直接撞簿 |
| **现在 · 加密所** | REST/WebSocket API、**做市商专用低延迟通道**、内部撮合队列 | 同一毫秒谁进 `Match()` **更前** |

**和 `Match()` 的关系：**

```
API 收到订单
  → （可选）路由层：散户通道 vs 做市商通道 vs 内部优先级  ← SuperDot 的现代版
  → Match()：OrderType 分支（限价进簿 / 市价扫簿）
  → 簿内：价格优先 → 同价 Timestamp 时间优先
```

- **散户能用到的** 就是 **市价 + 限价** — 也是你引擎 **M1–M2 的核心对象。  
- **OrderType** 是 **簿内规则** 的第一道分支；**路由/通道** 是 **进引擎之前** 谁先谁后 — 两层都要想清楚，HFT 争的往往是 **both**。

> go-dex **现在**：单通道 + `OrderType` + 价格–时间优先先把 **簿内逻辑** 写对；**M4+** 若要模拟做市商 API，再加 **队列优先级 / 多 ingress**（仍须公平、可审计 — §3 信息同步）。

→ 继续读 [Ch 4 订单类型](../../chapter-04-orders-and-order-types/) · [Ch 6 指令驱动市场](../../chapter-06-order-driven-markets/) · 动手 [M1 里程碑](../../00-practice-go-dex/notes/milestone-01-订单类型与LOB/)  
→ 被踩单 / 外部性：[Ch 1 §4.2 · 限价单期权](../../chapter-01-introduction-market-microstructure/notes/section-4-4-贯穿全书的关键主题.md#42-期权与外部性-options-and-externalities)

---
