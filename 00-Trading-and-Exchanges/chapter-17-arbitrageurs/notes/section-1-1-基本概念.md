## 1. 基本概念

### 1.1 定义

**套利者**：根据 **相对价值 (Relative values)** 信息交易的投机者。

```
买入相对便宜  +  卖出相对昂贵
        ↓
基差向正常关系回归（收敛）→ 获利
```

| vs 价值交易者 | Value 估 **绝对 V**；Arb 估 **相对关系** |
|---------------|----------------------------------------|

→ [Ch 10 §2.1](../chapter-10-informed-traders-market-efficiency/) · [Ch 16](../chapter-16-value-traders/)

### 1.2 对冲投资组合 (Hedge Portfolios)

通过 **多头 + 空头** 尽量消除 **市场层面共同风险** — 组合的各 **腿 (Legs)**。

| HFT 视角 |
|----------|
| **Basket / spread / basis trade** — 多腿同步是 **execution 核心** |
| **Beta-neutral、sector-neutral** 是 **hedge portfolio** 的现代说法 |

### 1.3 基差 (Basis) 与套利价差 (Arbitrage Spread)

| 术语 | 定义 |
|------|------|
| **基差 (Basis)** | 对冲组合中 **不同工具之间的价差** |
| **套利价差** | 基差与其 **公平价值 (Fair value)** 之间的差额 |
| **入场条件** | 仅当套利价差 **足够大** — 超出 **套利边界 (Arbitrage band)** |

```
Fair value of basis  =  运输/融资/股息/便利收益等
Arbitrage spread     =  Actual basis − Fair value
Trade if |spread| > transaction costs + risk premium
```

| HFT 视角 |
|----------|
| **Fair value model**（期货–现货、ETF–NAV、ADR–本地）驱动 **signal** |
| **Band** 含 **fees + half-spread × legs + slippage buffer** |

---
