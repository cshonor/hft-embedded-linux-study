# Ch 24 专家做市商系统 · Specialists

> **Trading and Exchanges** · Larry Harris · **选读** · Part VII · **历史制度解剖**

本章深入剖析特定交易所（尤其 **NYSE、美国证券交易所**）的 **专家 (Specialist)** 制度 — 交易所指定的 **主要做市商**：承担 **维持有序市场** 的沉重义务，同时享有 **独特交易特权**。

> **HFT 读者：** 现代 **DMM / designated market maker**、**order flow 信息优势**、**internalization** 争议的 **制度史**；与 [Ch 5 混合市场](./chapter-05-市场结构.md)、[Ch 6 单一价格拍卖](./chapter-06-指令驱动市场.md)、[Ch 11 quote match](./chapter-11-指令预期者.md)、[Ch 13 做市商](./chapter-13-做市商.md)、[Ch 25](./chapter-25-内部化优先撮合与交叉交易.md) 衔接。

---

## 0. 制度背景

| 时代 | NYSE **floor specialist** 为核心流动性与拍卖机制 |
|------|--------------------------------------------------|
| **今** | 电子化、**DMM** 义务弱化但 **义务/特权张力** 仍存 — 读本章理解 **监管权衡** 从何而来 |

---

## 1. 专家的四重身份

| 角色 | 职能 |
|------|------|
| **做市商 (Dealer)** | **自有资金** 交易，提供流动性 |
| **经纪人 (Broker)** | 接受 **订单路由** 来的委托代客执行；**口头公告板** — 靠信息网络撮合、建声誉、收佣金 |
| **拍卖人 (Auctioneer)** | 主持 **开盘单一价格拍卖** — 供需得出 **市场出清价** |
| **交易所官员** | 确保交易 **遵规**；订单 **公平展示与执行** |

→ [Ch 7 经纪人](./chapter-07-经纪人.md) · [Ch 6 uniform pricing / call](./chapter-06-指令驱动市场.md)

| HFT 视角 |
|----------|
| **单一实体** 兼 dealer + broker + auctioneer → **利益冲突** 的制度根源 |
| 现代 **禁止或隔离** 职能（Chinese wall、trade-at、OTR） |

---

## 2. 两大义务 (Obligations)

交易所对专家 **自有资金交易** 严格监管 — **换取特权** 的对价。

### 2.1 积极义务 (Affirmative Obligations)

| 要求 | 负责股票须始终有 **「合理且有序」** 的市场 |
|------|------------------------------------------|
| **最后交易者 (Trader of last resort)** | 无公众愿提供流动性时 → **自费** 报 **合理双向报价** |
| **价格连续性 (Price continuity)** | 介入交易 **平滑** 波动，防 **剧烈跳空** |

→ [Ch 19 弹性](./chapter-19-流动性.md) · [Ch 9 公共品](./chapter-09-好市场.md)

| HFT 视角 |
|----------|
| **Obligation to quote** — 类似 **market maker regulatory minimum** |
| **Volatility interruption** 时 **是否仍报价** — 现代 DMM 争议 |

### 2.2 消极义务 (Negative Obligations)

| **公众流动性优先 (Public liquidity preservation)** | 不得与公众 **抢流动性** |
|------------------------------------------------------|-------------------------|
| **价格优先** | 簿上有 **同价或更优** 公众限价单 → 专家 **让位**，公众 **先成交** |
| **何时可做市** | 仅填补公众订单留下的 **流动性真空** |

→ [Ch 6 公众指令优先 / time precedence](./chapter-06-指令驱动市场.md)

| HFT 视角 |
|----------|
| **Retail priority**、**pro-rata vs priority** 规则变体 |
| HFT **不得 step ahead**  of displayed public — 监管同源 |

---

## 3. 特权与盈利策略 (Specialist Privileges)

补偿 **积极义务**（行情差时被迫接盘）的成本 — **极具价值的特权**：

### 3.1 信息优势与投机 (Speculative & Quote-matching)

| 特权 | 见 **全系统未执行限价簿** — **订单流底牌** |
|------|------------------------------------------|
| **能力** | 比他人更准确 **预测短期价格** |
| **策略** | 买单堆积、卖单稀少 → **提前买入**（[Ch 11 报价匹配](./chapter-11-指令预期者.md)）→ 涨价获利 |

| HFT 视角 |
|----------|
| **Full book + order flow** — co-lo 的 **现代版信息优势**（无 floor 人身） |
| **Queue position visibility** 争议 |

### 3.2 撇奶油 (Cream-skimming)

| 特权 | 见 **订单来源** |
|------|----------------|
| **策略** | 区分 **不知情零售** vs **知情机构** |
| **行为** | 自己接 **安全零售**（cream）；把 **危险知情单** 留给 **公众限价单** 承受 adverse selection |

→ [Ch 14 不知情补贴知情](./chapter-14-买卖价差.md) · [Ch 25 internalization](./chapter-25-内部化优先撮合与交叉交易.md)

| HFT 视角 |
|----------|
| **Retail order internalization** — 电子时代的 **cream-skimming** |
| **PFOF** 链 — 谁接 retail、谁吃 institutional toxic |

### 3.3 截停股票与回溯择时期权 (Stopping Stock · Look-back Option)

| **截停 (Stop stock)** | 有权 **截停市价单** — 保证 **至少当前最优价** 成交，但 **挂起** 寻更好价 |
|-----------------------|---------------------------------------------------------------------|
| **实质** | 专家获 **回溯择时期权 (Look-back option)** |
| **有利** | 用自己账户与客户成交 |
| **不利** | **丢给** 簿上其他公众交易者 |

→ [Ch 4 限价单免费期权](./chapter-04-交易指令与订单类型.md) · [Ch 11](./chapter-11-指令预期者.md)

| HFT 视角 |
|----------|
| **Last look**（FX、部分 dark）— 同类 **期权价值** 争议 |
| **Trade-at / NBBO** 规则限制 **劣于公众成交** |

### 3.4 开盘优势 (The Market Open)

| 情形 | 开盘拍卖 **买卖严重失衡** |
|------|---------------------------|
| **专家** | 作 **弱势方** 最后流动性提供者介入 |
| **结果** | 常以 **极优出清价**（如跳空低开时买入）成交 → **随后反转** 获利 |

→ [Ch 6 单一价格拍卖](./chapter-06-指令驱动市场.md) · [Ch 23 program](./chapter-23-指数与投资组合市场.md)

| HFT 视角 |
|----------|
| **Opening auction imbalance** 策略 — 信息仍不对称 |
| **Indicative price** vs **clearing price** |

---

## 4. 监管问题与利益冲突

### 4.1 公共物品的成本

| 公共品 | **价格连续性**、**流动性保障** — 全员受益 |
|--------|------------------------------------------|
| **买单者** | 行使特权时受损的 **公众限价单** — 被截胡、cream-skim |

### 4.2 监管权衡

| 削减特权 / 佣金 | 专家可能在 **剧烈波动时拒提供流动性** 或 **退出做市** |
|-----------------|------------------------------------------------------|
| **特权过大** | 公众沦为 **超额利润** 牺牲品 |

```
义务（last resort、continuity）  ↔  特权（book、source、stop、open）
              ↓
        监管找均衡点
```

| HFT 视角 |
|----------|
| **Maker rebate 够吗？** 覆盖 **affirmative obligation** 成本？ |
| **Tick size、MM incentive** — 同一 **公共品融资** 问题 |
| [Ch 9](./chapter-09-好市场.md) 目标 3 — **建设性 LP** vs **剥削** |

---

## 5. 专家制度 vs 现代电子 LOB

| 维度 | NYSE Specialist（本章） | 现代 HFT / DMM |
|------|-------------------------|----------------|
| **信息** | 见全簿 + 订单来源 | Co-lo full book；**来源** 常匿名 |
| **义务** | 明确 affirmative/negative | **Reg NMS**、交易所 **MM 协议** |
| **特权** | Stop stock、cream-skim | **Last look** 受限；internalization **另轨** |
| **拍卖** | 专家主持开盘 | **交易所 auction 算法** |

> 制度形式变了，**义务–特权–公众成本** 三角 **未消失**。

---

## 6. 本章总结

| 要点 | 含义 |
|------|------|
| **四重身份** | Dealer · Broker · Auctioneer · Official |
| **积极义务** | Last resort、**价格连续性** |
| **消极义务** | **公众流动性优先** — 不与公众抢 |
| **特权策略** | 信息/quote match · **撇奶油** · **stop/look-back** · **开盘** |
| **监管** | 公共品受益 vs **公众限价单买单** |

> **HFT 读者 takeaway：** 专家是 **「有义务的超级做市商 + 看见底牌」** 的原型。今天批评 **PFOF / internalization / last look**，逻辑与 **cream-skimming、stopping stock** 同源。做 maker 时自问：你的 edge 是 **义务允许的流动性服务**，还是 **信息/路由特权**？下一章 [Ch 25](./chapter-25-内部化优先撮合与交叉交易.md) — 特权在 **经纪商/暗池** 轨道的延续。

---

## 相关章节

- 上一章：[chapter-23-指数与投资组合市场.md](./chapter-23-指数与投资组合市场.md)
- 下一章：[chapter-25-内部化优先撮合与交叉交易.md](./chapter-25-内部化优先撮合与交叉交易.md)
- 市场结构：[chapter-05-市场结构.md](./chapter-05-市场结构.md)
- 拍卖与优先规则：[chapter-06-指令驱动市场.md](./chapter-06-指令驱动市场.md)
- 做市与义务：[chapter-13-做市商.md](./chapter-13-做市商.md)
- 指令预期：[chapter-11-指令预期者.md](./chapter-11-指令预期者.md)
