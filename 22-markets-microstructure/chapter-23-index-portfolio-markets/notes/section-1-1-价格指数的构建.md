## 1. 价格指数的构建 (Price Indexes)

价格指数表征 **一组工具的平均价格表现**。

### 1.1 两种常见类型

| 类型 | 英文 | 权重逻辑 | 典型例子 |
|------|------|----------|----------|
| **价格加权** | Price-weighted | 与 **成分股价格总和** 成正比 — **高价股影响最大** | DJIA、日经 225 |
| **市值加权** | Value / Cap-weighted | 与 **总市值** 成正比 — **大市值影响最大** | S&P 500 |

### 1.2 恒定除数 (Constant Index Divisor)

更换成分股或 **股票拆分**（价格加权尤需）时，用 **恒定除数** 调整，避免指数 **不自然跳空**。

| HFT 视角 |
|----------|
| **Index rebalance / reconstitution** — 成分变更 → **可预测 utilitarian flow** |
| **Divisor corporate actions** — 数据管道须正确处理 |

---
