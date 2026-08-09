# Harris 29 章 ↔ 练手代码索引

> **理论笔记**在上一级目录 [`../chapter-*.md`](../)（29 章平铺）  
> **本文件**从 `code/` 出发，查「这章读什么 ↔ 代码写在哪」  
> 里程碑详情 → [../OUTLINE.md](../OUTLINE.md)

---

## 当前代码速查

| 代码文件 | 里程碑 | 直接相关的 Harris 章 | 实践笔记 |
|----------|--------|----------------------|----------|
| [orderbook.go](./orderbook.go) | M1–M2 | [Ch 4](../chapter-04-orders-and-order-types/) · [Ch 5](../chapter-05-market-structures/) · [Ch 6](../chapter-06-order-driven-markets/) | [M1](../notes/milestone-01-order-and-lob/) · [M2](../notes/milestone-02-matching-engine/) |
| [marketmaker.go](./marketmaker.go) | M2+ | [Ch 2 §1](../chapter-02-trading-stories/notes/section-1-1-散户股票交易.md) · [Ch 13](../chapter-13-dealers/) | 极简做市：定时挂买一/卖一 |
| [orderbook_test.go](./orderbook_test.go) | M1–M2 | 同上 | 同上 |
| [main.go](./main.go) | demo | [Ch 2 §1](../chapter-02-trading-stories/notes/section-1-1-散户股票交易.md) | MM + 散户市价成交演示 |
| `metrics.go`（待建） | M3 | [Ch 13](../chapter-13-dealers/) · [Ch 14](../chapter-14-bid-ask-spreads/) · [Ch 19](../chapter-19-liquidity/) | [M3 笔记](../notes/milestone-03-spread-and-liquidity/) |
| HTTP / WS（待建） | M4 | [Ch 25](../chapter-25-internalization-preferencing-crossing/)–[27](../chapter-27-floor-vs-automated-trading/) | [M4 笔记](../notes/milestone-04-api-and-symbols/) |

---

## 全书 29 章索引

| 章 | 英文 | 理论笔记 | 练手 | 说明 |
|:--:|------|----------|:----:|------|
| 1 | Introduction | [chapter-01](../chapter-01-introduction-market-microstructure/) | **框架** | 五维市场质量 ↔ 引擎设计总纲 → [§2 笔记](../chapter-01-introduction-market-microstructure/notes/section-2-2-核心目标-五个市场质量特征.md#六与-go-dex-撮合引擎对照) |
| 2 | Trading Stories | [chapter-02](../chapter-02-trading-stories/) | **入门** | [§1 散户交易](../chapter-02-trading-stories/notes/section-1-1-散户股票交易.md#与-go-dex理解订单簿的最好切入点) → 单簿撮合；[§4 套期保值](../chapter-02-trading-stories/notes/section-4-4-期货市场-套期保值.md#与-go-dex-撮合引擎的对照) → 现货+期货 **两腿** vs **组合单联动** |
| 3 | The Trading Industry | [chapter-03](../chapter-03-trading-industry/) | — | 背景：产业全景 |
| 4 | Orders and Order Properties | [chapter-04](../chapter-04-orders-and-order-types/) | **M1** | `Order` · 限价/市价 → [orderbook.go](./orderbook.go) |
| 5 | Market Structures | [chapter-05](../chapter-05-market-structures/) | **M1** | `Bids`/`Asks` 双边簿 → [orderbook.go](./orderbook.go) |
| 6 | Order-driven Markets | [chapter-06](../chapter-06-order-driven-markets/) | **M2** | `AddOrder` · 价格–时间优先 · `BestBid`/`BestAsk` → [orderbook.go](./orderbook.go) |
| 7 | Brokers | [chapter-07](../chapter-07-brokers/) | — | 选读：经纪商角色 |
| 8 | Why People Trade | [chapter-08](../chapter-08-why-people-trade/) | — | 选读：交易动机 |
| 9 | Good Markets | [chapter-09](../chapter-09-good-markets/) | — | 选读：好市场标准 |
| 10 | Informed Traders | [chapter-10](../chapter-10-informed-traders-market-efficiency/) | — | 知情交易 / 逆向选择 |
| 11 | Order Anticipators | [chapter-11](../chapter-11-order-anticipators/) | — | 指令预期 / front-running |
| 12 | Bluffers and Manipulation | [chapter-12](../chapter-12-bluffers-market-manipulation/) | — | 选读：操纵 |
| 13 | Dealers | [chapter-13](../chapter-13-dealers/) | **M2+** | [marketmaker.go](./marketmaker.go) 极简做市 |
| 14 | Bid-Ask Spreads | [chapter-14](../chapter-14-bid-ask-spreads/) | **M3** | spread = best_ask − best_bid |
| 15 | Block Traders | [chapter-15](../chapter-15-block-traders/) | — | 选读：大宗 |
| 16 | Value Traders | [chapter-16](../chapter-16-value-traders/) | — | 选读：价值交易 |
| 17 | Arbitrageurs | [chapter-17](../chapter-17-arbitrageurs/) | — | 套利（后期可扩展多 venue） |
| 18 | Buy-Side Traders | [chapter-18](../chapter-18-buy-side-traders/) | — | 选读：买方 |
| 19 | Liquidity | [chapter-19](../chapter-19-liquidity/) | **M3** | 深度 · 流动性指标 |
| 20 | Volatility | [chapter-20](../chapter-20-volatility/) | — | 选读：波动 |
| 21 | Transaction Cost Measurement | [chapter-21](../chapter-21-transaction-cost-measurement/) | — | 成本衡量（回测 KPI） |
| 22 | Performance Evaluation | [chapter-22](../chapter-22-performance-evaluation-prediction/) | — | 选读：绩效 |
| 23 | Index and Portfolio Markets | [chapter-23](../chapter-23-index-portfolio-markets/) | — | 选读：指数市场 |
| 24 | Specialists | [chapter-24](../chapter-24-specialists/) | — | 选读：专家做市 |
| 25 | Internalization, Preferencing, Crossing | [chapter-25](../chapter-25-internalization-preferencing-crossing/) | **M4** | 内部化 / 交叉交易 |
| 26 | Competition Among Markets | [chapter-26](../chapter-26-competition-within-among-markets/) | — | 多市场竞争 |
| 27 | Floor vs Automated Systems | [chapter-27](../chapter-27-floor-vs-automated-trading/) | **M4** | 电子化 · API 入口 |
| 28 | Bubbles, Crashes, Circuit Breakers | [chapter-28](../chapter-28-bubbles-crashes-circuit-breakers/) | — | 选读：熔断 |
| 29 | Insider Trading | [chapter-29](../chapter-29-insider-trading/) | — | 选读：内幕 |

**练手列：** **M1**–**M4** = 本仓库 Go 代码会覆盖；**—** = 先读理论，暂不写代码。

---

## 怎么用

1. 读完某章 `chapter-XX` → 在本表找「练手」列  
2. 有 **M1** 等标记 → 打开对应代码文件 + [notes/milestone-XX](../notes/)  
3. 写新函数时在文件头注释里加 `# 章`（见 [orderbook.go](./orderbook.go) 示例）

全书目录（含 HFT 标签）→ [../OUTLINE.md](../OUTLINE.md)
