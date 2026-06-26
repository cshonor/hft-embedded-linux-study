# Ch 6 指令驱动市场 · Order-driven Markets

> **Trading and Exchanges** · Larry Harris · **精读** · Part I

本章深入 **用特定交易规则安排成交** 的市场：口头拍卖、单一价格拍卖、连续电子拍卖、交叉网络等。  
Ch 5 已区分 quote-driven / order-driven；本章把 **order-driven 内部的规则差异** 讲透——**谁在规则下获利、该提供还是索取流动性**。

> **与工程对接：** LOB + 撮合规则 = matching engine 的「产品规格」；与 [00-practice-go-dex M2](./00-practice-go-dex/notes/milestone-02-撮合引擎/) · `15-HFT-Low-Latency-Practice` 直接相关。

---

## 1. 交易规则的两大支柱

所有指令驱动市场都靠两套规则运作：

| 支柱 | 英文 | 回答的问题 |
|------|------|------------|
| **指令优先规则** | Order precedence rules | 买方与卖方 **谁和谁先匹配** |
| **交易定价规则** | Trade pricing rules | 成交 **按什么价格执行** |

| HFT 视角 |
|----------|
| 交易所 **Rule book** → 你的引擎里的 `match()` / priority queue 逻辑 |
| 改优先规则（如 pro-rata vs FIFO）= 改策略 edge 分布，不是「实现细节」 |

---

## 2. 口头拍卖 (Oral Auctions)

交易者在 **交易所大厅面对面** 竞价——许多期货、期权、部分股票仍保留或曾采用此形式。

### 2.1 公开喊价 (Open-outcry rule)

所有买卖报价须 **公开表达**，保证每位参与者公平看见、参与。

### 2.2 指令优先机制

| 层级 | 规则 | 含义 |
|------|------|------|
| **首要** | **价格优先 (Price priority)** | 买方只接受 **最低** 卖价；卖方只接受 **最高** 买价 |
| **次要** | **时间优先 (Time precedence)** | 先报出 **改善当前最优价** 的交易者优先 |
| **次要** | **公众指令优先 (Public order precedence)** | 禁止会员抢在 **同价公众单** 之前成交 |

### 2.3 定价机制

**歧视性定价 (Discriminatory pricing)**：每笔成交按 **被接受的那条具体报价** 执行——不同对手方可以不同价。

| HFT 视角 |
|----------|
| 电子 LOB 的 price-time priority 是 open-outcry 的 **程序化继承** |
| 「会员优先 vs 公众优先」→ 现代 **order type / participant flag**（如 retail priority、broker 内部化）的立法根源 |

---

## 3. 基于规则的指令撮合系统 (Rule-Based Order-Matching)

绝大多数 **ECN / 电子化交易所** 采用此类系统：交易者提交限价单，**预设层级规则** 自动撮合。

在 **价格优先** 前提下，常见 **次要优先规则**：

| 规则 | 英文 | 要点 |
|------|------|------|
| **时间优先** | Time precedence | 同价 FIFO；最常见 |
| **展示优先** | Display precedence | **可见单** 优于 **隐藏单**，鼓励公开意图 |
| **规模/数量优先** | Size precedence | 按比例分配 (pro-rata) 或大单优先——因市场而异 |

| HFT 视角 |
|----------|
| **Display vs hidden** → iceberg、reserve size、dark pool 路由策略 |
| **Pro-rata**（部分期货）vs **FIFO**（多数 equity）→ queue position 估值完全不同 |
| 引擎须实现：**price → time / display / size** 的 **确定性排序**，否则 replay 对不上交易所 |

---

## 4. 三大定价规则与市场结构

**市场结构决定定价规则**；定价规则又反过来塑造 **策略与盈亏分配**。

### 4.1 单一价格拍卖 + 统一定价 (Single Price Auction · Uniform Pricing)

| 项目 | 内容 |
|------|------|
| **机制** | 所有成交在同一 **市场出清价 (Market-clearing price)** 执行 |
| **出清价** | 供给曲线与需求曲线 **相交** 的价格（集合竞价常见） |
| **效果** | 最大化整体 **交易者盈余 (Trader surplus)**——所有参与者从交易中获得的 **总收益** |
| **偏好者** | **限价单 / 流动性提供者** 通常更偏好——不会在连续撮合里被「逐笔歧视」 |

| HFT 视角 |
|----------|
| **开盘 / 收盘 auction**、IPO 定价；策略：imbalance 预测、indicative price 跟踪 |
| 与 continuous 段 **不同 matching 状态机**——OMS 须区分 order type |

### 4.2 连续双向拍卖 + 歧视性定价 (Continuous Two-Sided Auction · Discriminatory Pricing)

| 项目 | 内容 |
|------|------|
| **机制** | 按 **订单簿 (Order book)** 中挂单的限价，**逐笔** 决定成交价 |
| **效果** | 大且急切的投资者可将大单 **拆分**，前段以较优 BBO 成交 → **价格歧视** 对自己有利 |
| **对比 single price** | 给定相同订单流，连续市场通常 **成交量更大** |
| **偏好者** | **索取流动性的大交易者**（taker、urgency 高） |

| HFT 视角 |
|----------|
| **现代 equity HFT 主战场**：LOB + continuous discriminatory matching |
| 做市 = 挂在簿上被动成交；吃单 = 按对手限价逐档成交 → **impact 路径可建模** |
| M2 练手：[价格–时间优先撮合](./00-practice-go-dex/notes/milestone-02-撮合引擎/) |

### 4.3 交叉网络 + 衍生定价 (Crossing Networks · Derivative Pricing)

| 项目 | 内容 |
|------|------|
| **机制** | 网络 **不发现价格**，只撮合 **交易意愿** |
| **价格来源** | **衍生** 自其他市场——主所收盘价、NBBO 中间价、VWAP 等 |
| **风险 1：过时价格 (Stale prices)** | 基本面已变而交叉价未更新 → **知情交易者** 单向涌入 → **逆向选择** |
| **风险 2：价格操纵 (Price manipulation)** | 大资金为在交叉网获好价，在主市场 **小额拉/砸** 基准价 |

| HFT 视角 |
|----------|
| Dark pool / internalizer / midpoint peg → 本章框架的 **现代实例** |
| **Reference price feed** 延迟与 **marking the close** 监管——与 Ch 25 内部化、Ch 12 操纵衔接 |
| 策略：何时 **avoid crossing**、何时利用 **stale quote arbitrage**（合规边界内） |

---

## 5. 三种结构对照

| 结构 | 定价规则 | 权力偏向 | 流动性角色 |
|------|----------|----------|------------|
| **单一价格拍卖** | 统一定价 | 小型 **流动性提供者** | 限价单在出清价成交，盈余分配更「平均」 |
| **连续双向拍卖** | 歧视性定价 | **急切的大 taker** | 可拆分吃簿；LP 逐笔让价 |
| **交叉网络** | 衍生定价 | **知情者 / 操纵者**（若监管弱） | 不报价，只「意愿匹配」 |

```
规则 → 谁有特权 → 提供 vs 索取流动性 → 策略存活条件
```

---

## 6. 本章总结

| 要点 | 含义 |
|------|------|
| **规则不是背景** | 决定 **权力、特权、盈余分配** |
| **优先规则** | Price → Time / Display / Size；即 **queue 逻辑** |
| **定价规则** | Uniform vs Discriminatory vs Derivative；即 **成交价函数** |
| **选市场 = 选规则** | 同一订单流在不同结构下 **成交量、成本、被 picked off 概率** 不同 |

> **HFT 读者 takeaway：** 读 matching engine 或写 Go/Rust 撮合前，先答四个问题——**(1)** continuous 还是 call？**(2)** 歧视性还是统一定价？**(3)** 次要优先是 time、display 还是 pro-rata？**(4)** 有没有衍生定价的交叉子系统？  
> 答案即 **spec**；Ch 4 的订单类型 + 本章规则 = 完整 LOB 语义。

---

## 相关章节

- 上一章：[chapter-05-市场结构.md](./chapter-05-市场结构.md)
- 下一章：[chapter-07-经纪人.md](./chapter-07-经纪人.md)
- 订单类型基础：[chapter-04-交易指令与订单类型.md](./chapter-04-交易指令与订单类型.md)
- 练手：[00-practice-go-dex M2 撮合引擎](./00-practice-go-dex/notes/milestone-02-撮合引擎/)
