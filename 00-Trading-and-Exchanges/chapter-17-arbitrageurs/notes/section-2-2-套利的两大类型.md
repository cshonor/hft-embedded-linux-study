## 2. 套利的两大类型

按 **基差风险性质** 划分：

### 2.1 纯粹套利 (Pure Arbitrages)

对冲组合价值 **均值回归 (Mean-reverting)** — 长期风险 **极低**；有 **明确机制** 保证收敛。

| 子类 | 机制 |
|------|------|
| **运输套利 (Shipping)** | 跨市场买卖 **同一实物**；价差 > 运输成本 → 运货或等收敛 |
| | **实际承运人** vs **虚拟承运人**（不运货，等价格收敛） |
| **交割套利 (Delivery)** | **期货–现货** 价差；**到期交割** 强制收敛 |
| **转换套利 (Conversion)** | 相同风险、不同形式 — **期权动态对冲**、大豆→豆油+豆粕 **压榨套利** |
| | 类似金融工程 **「制造」(Manufacturing)** |

| HFT 视角 |
|----------|
| **Cash-and-carry、reverse basis** — 交割套利电子化 |
| **Index arb、ETF creation** — conversion 变体 |
| **Box spread、put-call parity** — 期权 conversion |

### 2.2 投机性套利 / 风险套利 (Speculative / Risk Arbitrages)

存在 **工具特有估值因素** → 组合价值 **非平稳 (Nonstationary)**，但 **短期强均值回归** — **风险较高**。

| 子类 | 说明 |
|------|------|
| **价差交易 (Spreads)** | 除一特征外 **几乎相同** — **日历价差 (Calendar)**、收益率曲线 |
| **配对交易 (Pairs)** | 高相关工具（如 Ford / GM）**相对价错位** — 买便宜卖贵 |
| **统计套利 (Statistical arbitrage)** | **多因子模型** 识别多工具定价不一致 → **优化组合** |
| **风险套利 (Risk arb)** | 常指 **并购套利 (Merger/Takeover)** — 宣布后 **买目标、空收购方** |
| | 风险：**交易失败 / 条款变更** |

| HFT 视角 |
|----------|
| **Stat arb / pairs** = HFT **主策略类之一** — 本章 **risk arb 框架** 直接适用 |
| **Merger arb** — event-driven，非典型 HFT 但 **deal break gap** 同类 |
| **Nonstationary** → **model drift**、**regime change** 须监控 |

---
