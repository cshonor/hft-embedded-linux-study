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
