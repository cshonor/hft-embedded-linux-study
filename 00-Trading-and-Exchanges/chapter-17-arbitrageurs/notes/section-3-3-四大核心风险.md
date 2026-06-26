## 3. 四大核心风险

套利 **绝非无风险提款机**。

### 3.1 执行风险 (Implementation Risk)

| 问题 | 交易成本 **高于预期** |
|------|----------------------|
| **限价单不确定性** | **「冰块上的探险者」** — 一腿成交，另一腿 **价格跑掉** → **巨大敞口** |

| HFT 视角 |
|----------|
| **Leg risk** — 多 venue 同步、**IOC/FOK**、**synthetic spread order** |
| **Partial fill** 管理 — 自动 **hedge orphan leg** |
| **Latency skew** 跨市场 — 一腿快一腿慢 |

### 3.2 基差风险与规模 (Basis Risk and Scale)

| 问题 | 基差 **向不利方向扩大** |
|------|-------------------------|
| **纪律** | **忌满额杠杆** — 须 **持有能力 (Staying power)** |
| | 基差阶段性扩大时 **不被迫平仓** |
| **教训** | **LTCM** — 杠杆过大 + 基差扩大 → **爆仓** |

| HFT 视角 |
|----------|
| **Risk limits on gross/net exposure** per spread |
| **Drawdown tolerance** vs **margin call** — 与 prime broker **credit line** |
| **Crowded trade** — 基差扩大因 **多人同 arb** |

### 3.3 模型 / 分析风险 (Model / Analytic Risk)

| 错误 | 误解 **真实关系** |
|------|-------------------|
| | 把 **非套利** 当套利 |
| | **错误对冲比率 (Hedge ratio)** |

| HFT 视角 |
|----------|
| **Cointegration break**、**structural change**（并购、退市） |
| **Beta 估计误差** — pairs 关系 **非恒定** |

### 3.4 持有成本风险 (Carrying Cost Risk)

| 来源 | 说明 |
|------|------|
| **追加保证金** | 空头腿标的 **暴涨** |
| **收敛缓慢** | 时间 **耗损 carry** |
| **强制平仓** | **Forced buy-in**、**逼空 (Short squeeze)** |

| HFT 视角 |
|----------|
| **Stock loan recall**、**hard-to-borrow fee spike** |
| **Funding rate**（crypto perp basis） |
| **Dividend / ex-date** 模型错误 |

---
