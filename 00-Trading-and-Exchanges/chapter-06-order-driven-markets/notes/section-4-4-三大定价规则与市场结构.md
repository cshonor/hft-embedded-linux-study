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
