## 2. 三大类型

### 2.1 抢先交易者 (Front Runners)

收集他人 **已决定安排** 的交易信息，赶在 **成交完成前** 抢先入场。

#### A. 抢先激进交易者 (Front Running Aggressive Traders)

| 目标 | **激进交易者** 的大额 **市价单** → 可 **推动价格** |
|------|------------------------------------------------------|
| **策略** | **预判 price impact**，提前同向建仓 |
| **信息来源** | **非法**：broker 泄露客户大单（Ch 7） |
| | **合法/灰色**：floor 观察 broker **肢体语言** 推测大单；现代：**order flow 预测、venue 数据** |

| HFT 视角 |
|----------|
| **Latency arb on institutional flow**、**sniffing** 争议 |
| **Aggressive informed**（Ch 10）与 **front-run 他人已知单** 的 **道德/法律** 分界 |

#### B. 抢先被动交易者 / 报价匹配者 (Quote Matchers)

| 目标 | **被动大额限价单** — 提供流动性 + **免费交易期权** |
|------|-----------------------------------------------------|
| **策略** | 挂出 **略优于大单** 的限价（**Penny jumping / 抢帽子**）， **挡在大单前** |
| **若价向有利** | 抢先者 **大赚** |
| **若价向不利** | 把头寸 **甩给身后大单**（limit 被迫接货）→ **控制损失** |

```
价格向有利移动 → quote matcher 获利平仓
价格向不利移动 → 抛给身后 large limit（对方被迫接盘）
```

| HFT 视角 |
|----------|
| **Queue jumping**、**improve BBO by 1 tick** 在 **tick 小** 时极便宜 |
| LP 大额单 **free option** 被 **penny jumpers** 榨取 → **why widen / hide size** |

---

### 2.2 情绪导向的技术交易者 (Sentiment-Oriented Technical Traders)

与 front runners 不同：预测 **不知情交易者将要（尚未）做出的决策**。

| 方法 | 研究 **历史模式** — 来自投资、借贷、避税、对冲、赌博等 **utilitarian 动机** |
|------|-------------------------------------------------------------------------------|
| **例子** | **一月效应 (January Effect)** — 年底 tax-loss selling 后次年反弹 |
| | **期权做市商 delta 对冲** — 价格变动 → **必然** 的 **dynamic hedge flow** |
| **效果** | 抢在不知情者前行动 → 价格 **偏离基本面** → **降低信息效率** |

| HFT 视角 |
|----------|
| **Seasonality / rebalancing flow** 预测 — 若 **纯 exploit uninformed pattern** → 偏寄生 |
| **Gamma squeeze / dealer hedging flow** 现代变体 |
| 与 Ch 10 **information-oriented technical** 区别：后者 exploit **定价错误**；本章 exploit **可预测 flow** |

---

### 2.3 逼空者 / 挤压者 (Squeezers)

| 策略 | **垄断（囤积）市场一侧** |
|------|--------------------------|
| **受害者** | 必须在 **另一侧平仓** 的交易者（如 **急迫平空** 的空头） |
| **结果** | 只能以 **极不利价格** 与 squeezers 谈判 |
| **场景** | 商品期货（1888 芝加哥小麦）；**低 float + 高 short interest** 股票 |

| HFT 视角 |
|----------|
| **Short squeeze、corner** — 与 **fundamental informed** 无关，纯 **position / supply 垄断** |
| **Meme stock、borrow recall** 现代案例；**locate / stock loan** 是约束 |

---
