## 1. 交易成本的三大组成部分

交易成本 **≠ 仅佣金**，包含：

### 1.1 显性成本 (Explicit Costs)

成本会计可 **直接识别** 的支出：

| 项目 | 例子 |
|------|------|
| **佣金** | 付给经纪人 |
| **交易所费用** | 撮合、数据、接入 |
| **税费** | 印花税等 |
| **交易台固定成本** | 人员、软硬件 |

| HFT 视角 |
|----------|
| **Maker rebate / taker fee** — 显性，可精确建模 |
| **Co-lo、feed、cross-connect** — 固定 + 变动显性成本 |

### 1.2 隐性成本 (Implicit Costs)

对 **市场价格产生影响** 导致的成本：

| 类型 | 说明 |
|------|------|
| **买卖价差** | 市价单交易者支付 — [Ch 14](../chapter-14-bid-ask-spreads/) |
| **市场冲击 (Market impact)** | 大买单 **推高**、大卖单 **压低** 价格 |

→ [Ch 18 暴露冲击](../chapter-18-buy-side-traders/) · [Ch 15 大宗](../chapter-15-block-traders/)

### 1.3 错失交易的机会成本 (Missed Trade Opportunity Costs)

| 情形 | 损失 |
|------|------|
| **未及时完成** | 订单拖延 |
| **未完全成交** | 部分 fill |
| **限价未成交** | 挂低价买单，价格 **一路上涨** → 错失利润 |

→ [Ch 4 限价期权](../chapter-04-orders-and-order-types/)

| HFT 视角 |
|----------|
| **Opportunity cost** 是 algo **urgency vs price** 的核心 trade-off |
| **Partial fill + hedge lag** — arb **leg risk** 的机会成本版 |

---
