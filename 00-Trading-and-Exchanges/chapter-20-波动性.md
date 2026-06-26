# Ch 20 波动性 · Volatility

> **Trading and Exchanges** · Larry Harris · **选读** · Part V

本章全面解析 **市场波动性** 的本质、分类、成因，以及如何 **衡量与区分**。理解波动性对 **交易者风险管理、获利**，以及 **监管政策** 至关重要。

> **HFT 读者：** **总 vol 分解** → 做市 **inventory / spread** 定价；**负序列相关** → microstructure noise；与 [Ch 14 暂时性价差](./chapter-14-买卖价差.md)、[Ch 19 弹性](./chapter-19-流动性.md)、[Ch 16 价值回归](./chapter-16-价值交易者.md) 衔接。

---

## 1. 核心定义与各方关注

### 1.1 定义

**波动性 (Volatility)**：价格发生 **意外变化** 的倾向。

价格变化来自：

| 驱动 | 说明 |
|------|------|
| **新信息** | 有关 **价值** 的信息 |
| **流动性需求** | **急躁交易者** 对流动性的需求 |

### 1.2 各方为何关注

| 主体 | 关切 |
|------|------|
| **交易者** | 巨大意外变动 → **风险与机会** |
| **清算所** | 极端 vol → 交易者 **破产**、**交割失败** |
| **交易所 / 经纪人** | 巨量交易 → 系统 **超载崩溃** |
| **经济学家 / 监管者** | 资源 **错配**；负面 **财富效应** → 宏观投资与消费 |

| HFT 视角 |
|----------|
| **Vol targeting**、**position sizing**、**circuit breaker** 应对 |
| **Peak message rate** — vol 日 infra 容量规划 |
| 区分 vol 来源 → **该 widen 还是该 hold through news** |

---

## 2. 两大类型 (Two Types of Volatility)

**总波动性 (Total volatility)** = **基本面波动性** + **暂时性波动性**

> **区分二者 = 理解本章的关键**

```
Total vol  =  Fundamental vol  +  Transitory vol
               （不回归）            （会回归）
```

| HFT 视角 |
|----------|
| **Realized vol** 混有两类 — **markout / reversal** 分离 transitory |
| **News days** → fundamental ↑；**illiquid chop** → transitory ↑ |

---

## 3. 基本面波动性 (Fundamental Volatility)

### 3.1 成因与特征

当决定 **基本面价值** 的因素 **意外改变** 时产生：

- 商品供需、利率、管理质量、宏观政策等

有效市场中价格已含 **已知信息** → 纯粹的 **基本面价格变化不可预测** — **随机游走 (Random walk)**。

→ [Ch 10](./chapter-10-知情交易者与市场效率.md) · [Ch 9 信息效率](./chapter-09-好市场.md)

### 3.2 加剧基本面波动性的因素

| 因素 | 例子 / 机制 |
|------|-------------|
| **储存成本与易腐性** | 电力、农产品 — 供需失衡 → **价格反应极剧烈** |
| **基本面不确定性** | 研发型新公司 — 价值取决于 **未出现产品 / 不确定研究** |
| **政治风险** | 国有化、主权违约风险 |
| **高杠杆** | 债务远高于股权 — 股权 **放大** 基本面冲击 |

| HFT 视角 |
|----------|
| **Event risk**、**earnings**、**biotech binary** — 不宜纯 MM |
| **Leveraged ETF rebalance** — fundamental + flow 混合 |

---

## 4. 暂时性波动性 (Transitory Volatility)

### 4.1 成因与特征

**急躁的不知情交易者** 的流动性需求 → 价格 **偏离基本面** → **暂时性波动**。

**「暂时」**：[Ch 16 价值交易者](./chapter-16-价值交易者.md)、[Ch 17 套利者](./chapter-17-套利者.md) 介入后，价格 **回归 (Revert)** 基本面。

→ [Ch 19 弹性 (Resiliency)](./chapter-19-流动性.md)

### 4.2 买卖价差跳动 (Bid/ask Bounce)

暂时性波动 **最简单形式**：

```
买方按 ask 成交  →  观测价跳上
卖方按 bid 成交  →  观测价跳下
        ↓
价格在 bid/ask 间来回跳动 — 无基本面变化
```

→ [Ch 14 暂时性价差成分](./chapter-14-买卖价差.md)

### 4.3 与交易成本的关系

| 关联 | 说明 |
|------|------|
| **暂时性 vol ↔ 不知情者成本** | 流动性溢价、**市场冲击** |
| **低流动性市场** | 暂时性 vol **高** + 交易成本 **高** |

| HFT 视角 |
|----------|
| **Microstructure noise** 主导 **短 horizon** 收益 |
| **Spread capture** 部分来自 **bounce** — 非 alpha |
| **Impact cost** = 暂时性 vol 的 **大单版** |

---

## 5. 衡量与区分 (Measuring Volatility)

### 5.1 基本衡量

| 方法 | 说明 |
|------|------|
| **方差 / 标准差** | 常用 **总波动性** 指标 |
| **分解** | Total = Fundamental + Transitory（概念上） |

### 5.2 统计学区分

| 类型 | 价格变化特征 |
|------|--------------|
| **基本面 vol** | **不可预测**、**不回归** |
| **暂时性 vol** | 变化往往 **回归（反转）** |

### 5.3 负序列相关性 (Negative Serial Correlation)

价格 **回归** → 收益率 **负自相关**：

```
上涨后倾向下跌  ·  下跌后倾向上涨
        ↓
识别暂时性 vol 的强指标
```

### 5.4 Roll 模型

**Richard Roll** 的 **序列协方差价差估计** — 利用 **负相关**（由 **bid/ask bounce** 引起）估计 **交易成本** 与 **波动性构成** 的经典方法。

| HFT 视角 |
|----------|
| **Roll spread estimator**、**Corwin-Schultz** 等 — 从 OHLC 推 effective spread |
| **Autocorrelation of tick returns** @ lag 1 — transitory 诊断 |
| **Variance ratio tests** — random walk vs mean reversion |

---

## 6. 监管启示

公众遇 **高波动** 常要求监管 **压 vol** — 监管者须分清 **来源**：

| 波动类型 | 监管立场 |
|----------|----------|
| **基本面 vol** | **无法也不应消除** — 价格须随世界变 → **有效资源配置** |
| **暂时性 vol** | **规则与设计可显著影响** — 应 **提高流动性**、完善微观结构 → **降低不必要波动** |

| 政策工具 | 目标 |
|----------|------|
| **Tick size、time priority、transparency** | 降 transitory — [Ch 6](./chapter-06-指令驱动市场.md) · [Ch 11](./chapter-11-指令预期者.md) |
| **Circuit breakers、halts** | 防 **系统超载** — 非消除 fundamental |
| **Maker obligation、fee schedule** | 吸引 LP — [Ch 19](./chapter-19-流动性.md) |

| HFT 视角 |
|----------|
| **Volatility interruption** 触发 → 策略 **pause** |
| 监管 **打击操纵/spoofing** → 降 **虚假 transitory** |
| 勿将 **legitimate news vol** 与 **microstructure chop** 混为一谈 |

---

## 7. 与全书概念对照

| 概念 | 章节 | 与 vol 关系 |
|------|------|-------------|
| 暂时性价差 | Ch 14 | bounce → transitory vol |
| 弹性 | Ch 19 | transitory 恢复速度 |
| 价值交易者 | Ch 16 | 回归基本面 |
| 价差三要素 | Ch 14 | vol ↑ → spread ↑ |
| 好市场 | Ch 9 | 降 **不必要** transitory |

---

## 8. 本章总结

| 要点 | 含义 |
|------|------|
| **定义** | 意外价格变化 — 信息 + 流动性需求 |
| **分解** | **Fundamental**（随机游走）+ **Transitory**（回归） |
| **Fundamental 驱动** | 供需、不确定性、政治、杠杆 |
| **Transitory 驱动** | 不知情 urgent flow、bounce、低流动性 |
| **识别** | **负序列相关**、Roll 模型 |
| **监管** | 不消 fundamental；降 transitory via **流动性** |

> **HFT 读者 takeaway：** **短周期策略** 大量 PnL 来自 **transitory**（spread + reversal）；**新闻策略** 赌 **fundamental**。做市 **vol 报价**：fundamental vol ↑ → ** widen + reduce size**；transitory chop ↑ → **markout 模型** 调 adverse selection。对 `orderbook.go` 回放 tick：算 **lag-1 自相关** 可粗看 **bounce 主导** 还是 **趋势主导**。

---

## 相关章节

- 上一章：[chapter-19-流动性.md](./chapter-19-流动性.md)
- 下一章：[chapter-21-流动性与交易成本衡量.md](./chapter-21-流动性与交易成本衡量.md)
- 价差成分：[chapter-14-买卖价差.md](./chapter-14-买卖价差.md)
- 弹性与回归：[chapter-16-价值交易者.md](./chapter-16-价值交易者.md) · [chapter-19-流动性.md](./chapter-19-流动性.md)
- 政策框架：[chapter-09-好市场.md](./chapter-09-好市场.md)
