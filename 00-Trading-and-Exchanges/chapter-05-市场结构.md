# Ch 5 市场结构 · Market Structures

> **Trading and Exchanges** · Larry Harris · **精读**

不同 **交易规则** 与 **信息系统** 如何组织交易、影响流动性、成本与价格效率——理解各类交易者 **如何制定策略** 的基础。

> **核心问题：** 市场如何运作？谁提供流动性？谁信息最多、行动最快？→ 决定哪些策略能赚钱。

---

## 1. 交易时段 (Trading Sessions)

| 类型 | 机制 | 示例 / HFT |
|------|------|-----------|
| **连续市场 (Continuous markets)** | 时段内买卖意愿随时匹配即成交 | NYSE/Nasdaq 日间连续竞价；**HFT 主战场** |
| **集合市场 / 集合竞价 (Call markets)** | 固定时点统一撮合全部意愿 | 开盘/收盘 auction；低流动性时段 call |

| HFT 视角 |
|----------|
| 连续市场 → latency 竞争；auction → **开盘竞价策略**、closing imbalance |
| 引擎须区分 **continuous vs auction** 订单类型与 matching 逻辑 |

---

## 2. 交易执行系统 (Execution Systems)

市场结构中 **最重要的特征**——买方与卖方如何匹配。

### 2.1 报价驱动 (Quote-driven / 做市商市场)

- **做市商参与每一笔交易**，提供 **全部流动性**
- 公众 **不能直接互成交**，须与做市商（或其 broker）谈判

| 特征 | HFT |
|------|-----|
| Dealer 控 spread | 传统 Nasdaq 结构；现代仍见 **internalization** |
| 无 peer-to-peer | HFT 若做市 = 扮演 dealer 角色 |

### 2.2 指令驱动 (Order-driven)

- 买卖双方 **直接互成交**，无需做市商介入
- 靠 **交易规则** 匹配：**指令优先规则** + **交易定价规则**
- 流动性来自公众 **限价单**

| 特征 | HFT |
|------|-----|
| 拍卖 / 电子撮合 | **现代 equity HFT 核心**：LOB + price-time priority |
| 规则即代码 | matching engine 实现 = 本章 abstract → 代码 |

### 2.3 经纪人市场 (Brokered markets)

- 经纪人 **主动搜索** 对手方
- 用于 **大宗**、房地产、**极低流动性** 工具

| HFT 视角 |
|----------|
| 与 HFT 热路径分离；block negotiation upstairs |

### 2.4 混合市场

现实市场常 **组合** 上述类型（如 exchange LOB + designated market maker + broker upstairs）。

---

## 3. 市场透明度 (Market Transparency)

透明度决定 **谁获利**、**交易成本** 高低。

### 3.1 事前透明度 (Ex ante / Pre-trade)

公布 **报价与订单信息**（交易前）：

| 级别 | 内容 |
|------|------|
| **Full book** | 全部限价单簿（按价格展示） |
| **Top of book** | 仅最优买卖价 (BBO) |

| HFT 视角 |
|----------|
| Level II / full depth feed → **订单簿重建** |
| 仅 BBO → 信息劣势；HFT 付费买 depth |
| **Hidden / iceberg** → 透明度不完整 |

### 3.2 事后透明度 (Ex post / Post-trade)

迅速、完整公布 **已成交** 信息（价、量）。

| HFT 视角 |
|----------|
| Trade tape → 推断 hidden flow、venue 分布 |
| 延迟 publication → **trade reporting** arb 窗口（监管收紧） |

### 3.3 不透明市场 (Opaque markets)

公众 **看不到** 上述信息 → 信息优势更集中在 insiders / participants。

| HFT 视角 |
|----------|
| **暗池 (Dark pools)**：pre-trade opaque，post-trade 可能披露 |
| HFT 在 lit vs dark 之间的 **路由策略** |

---

## 4. 市场信息与路由系统

### 4.1 指令路由系统 (Order-routing systems)

在客户、broker、dealer、exchange 间传递：

- 新单、撤单
- **Execution reports（成交回报）**

**速度与准确性** → 能否抓住转瞬即逝的机会。

| HFT 视角 |
|----------|
| **OMS/EMS → exchange gateway → matching engine** 全链路 latency |
| 错单、慢回报 = 风险；co-located **binary protocol** (FIX/native) |

### 4.2 指令簿 (Order books)

保存尚未成交订单（限价单、止损单等）——电子库或历史纸质簿。

| HFT 视角 |
|----------|
| **LOB = HFT 核心数据结构**；详见 [chapter-06-限价订单簿LOB.md](./chapter-06-限价订单簿LOB.md) |
| 多 venue → **多个 order book** 需聚合 (SIP vs direct feed) |

### 4.3 交易代码 (Ticker symbols)

识别工具的唯一编码（股票、衍生品到期/行权等）。

| HFT 视角 |
|----------|
| Symbology mapping（ISIN/CUSIP/exchange symbol）是 **数据管道** 基础 |

---

## 本章总结

| 维度 | 影响 |
|------|------|
| 连续 vs 集合 | 策略时间粒度 |
| Quote vs Order vs Broker | 你在 ecosystem 中的角色 |
| Pre / Post transparency | 信息 edge 与 adverse selection |
| Routing + Order book | 系统架构与 latency 预算 |

```
市场结构
    → 谁提供流动性、谁索取
    → 谁看见什么、多快看见
    → 最终：流动性 · 成本 · 价格效率 · 哪些策略存活
```

> **HFT 读者 takeaway：** 第五章是 **交易所产品规格说明书** 的业务版——读 matching engine 代码前，先能回答：这是 order-driven 还是 quote-driven？full book 还是 top？continuous 还是 call？这些答案决定你的 Rust 引擎该实现哪些模块。

---

## 相关章节

- 上一章：[chapter-04-交易指令与订单类型.md](./chapter-04-交易指令与订单类型.md)
- 下一章：[chapter-06-指令驱动市场.md](./chapter-06-指令驱动市场.md)
