# Trading and Exchanges — 全书目录（29 章）

> **Trading and Exchanges: Market Microstructure for Practitioners** · Larry Harris

| 标签 | HFT 读法 |
|------|----------|
| 🔴 | 精读 · 本仓库有笔记或待写 |
| 🟡 | 选读 · 场景触发 |
| ⚪ | 跳过 · 默认不建笔记 |

---

## 引言部分

| 章 | 英文 | 笔记 | HFT |
|----|------|------|-----|
| 1 | Introduction | [chapter-01](./chapter-01-introduction-market-microstructure/) | 🔴 |
| 2 | Trading Stories | [chapter-02](./chapter-02-trading-stories/) | 🔴 |

## Part I · 交易的结构 (The Structure of Trading)

| 章 | 英文 | 笔记 | HFT |
|----|------|------|-----|
| 3 | The Trading Industry | [chapter-03](./chapter-03-trading-industry/) | 🔴 |
| 4 | Orders and Order Properties | [chapter-04](./chapter-04-orders-and-order-types/) | 🔴 |
| 5 | Market Structures | [chapter-05](./chapter-05-market-structures/) | 🔴 |
| 6 | Order-driven Markets | [chapter-06](./chapter-06-order-driven-markets/) | 🔴 |
| 7 | Brokers | [chapter-07](./chapter-07-brokers/) | 🟡 |

## Part II · 交易的益处 (The Benefits of Trade)

| 章 | 英文 | 笔记 | HFT |
|----|------|------|-----|
| 8 | Why People Trade | [chapter-08](./chapter-08-why-people-trade/) | 🟡 |
| 9 | Good Markets | [chapter-09](./chapter-09-good-markets/) | 🟡 |

## Part III · 投机者 (Speculators)

| 章 | 英文 | 笔记 | HFT |
|----|------|------|-----|
| 10 | Informed Traders and Market Efficiency | [chapter-10](./chapter-10-informed-traders-market-efficiency/) | 🔴 |
| 11 | Order Anticipators | [chapter-11](./chapter-11-order-anticipators/) | 🔴 |
| 12 | Bluffers and Market Manipulation | [chapter-12](./chapter-12-bluffers-market-manipulation/) | 🟡 |

## Part IV · 流动性提供者 (Liquidity Suppliers)

| 章 | 英文 | 笔记 | HFT |
|----|------|------|-----|
| 13 | Dealers | [chapter-13](./chapter-13-dealers/) | 🔴 |
| 14 | Bid/Ask Spreads | [chapter-14](./chapter-14-bid-ask-spreads/) | 🔴 |
| 15 | Block Traders | [chapter-15](./chapter-15-block-traders/) | 🟡 |
| 16 | Value Traders | [chapter-16](./chapter-16-value-traders/) | 🟡 |
| 17 | Arbitrageurs | [chapter-17](./chapter-17-arbitrageurs/) | 🔴 |
| 18 | Buy-Side Traders | [chapter-18](./chapter-18-buy-side-traders/) | 🟡 |

## Part V · 流动性与波动性的起源

| 章 | 英文 | 笔记 | HFT |
|----|------|------|-----|
| 19 | Liquidity | [chapter-19](./chapter-19-liquidity/) | 🔴 |
| 20 | Volatility | [chapter-20](./chapter-20-volatility/) | 🟡 |

## Part VI · 评估与预测

| 章 | 英文 | 笔记 | HFT |
|----|------|------|-----|
| 21 | Liquidity and Transaction Cost Measurement | [chapter-21](./chapter-21-transaction-cost-measurement/) | 🔴 |
| 22 | Performance Evaluation and Prediction | [chapter-22](./chapter-22-performance-evaluation-prediction/) | 🟡 |

## Part VII · 市场结构（原书标 Part VIII）

| 章 | 英文 | 笔记 | HFT |
|----|------|------|-----|
| 23 | Index and Portfolio Markets | [chapter-23](./chapter-23-index-portfolio-markets/) | 🟡 |
| 24 | Specialists | [chapter-24](./chapter-24-specialists/) | 🟡 |
| 25 | Internalization, Preferencing, and Crossing | [chapter-25](./chapter-25-internalization-preferencing-crossing/) | 🔴 |
| 26 | Competition Within and Among Markets | [chapter-26](./chapter-26-competition-within-among-markets/) | 🔴 |
| 27 | Floor Versus Automated Trading Systems | [chapter-27](./chapter-27-floor-vs-automated-trading/) | 🔴 |
| 28 | Bubbles, Crashes, and Circuit Breakers | [chapter-28](./chapter-28-bubbles-crashes-circuit-breakers/) | 🟡 |
| 29 | Insider Trading | [chapter-29](./chapter-29-insider-trading/) | 🟡 |

---

## HFT 推荐阅读顺序（在 29 章框架内）

```
Ch 1–6   结构基础（LOB / 撮合）
Ch 10–11 知情交易 · 指令预期（adverse selection / front-running）
Ch 13–14 做市商 · 价差（HFT 做市核心）
Ch 17    套利者
Ch 19    流动性
Ch 25–27 内部化 · 多市场竞争 · 电子化（colocation 语境）
Ch 21    成本衡量（回测与实盘 KPI）
```

---

## 配套练手 · Go DEX

| | 路径 |
|---|------|
| **导读** | [00-practice-go-dex/README.md](./00-practice-go-dex/README.md) |
| **里程碑 ↔ 章节** | [00-practice-go-dex/OUTLINE.md](./00-practice-go-dex/OUTLINE.md) |
| **实践笔记** | [00-practice-go-dex/notes/](./00-practice-go-dex/notes/) |
| **Go 源码** | [00-practice-go-dex/code/](./00-practice-go-dex/code/) |

理论笔记 **在各章 `chapter-XX-*/notes/`**；练手代码 **不混入** `09` OS / `05` 内核等大模块。

完整路线 → [HFT-READING-ROADMAP.md](../HFT-READING-ROADMAP.md)
