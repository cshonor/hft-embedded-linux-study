## 2. 价差的两大组成部分 (Spread Components)

| 成分 | 别名 | 含义 | 价格效应 |
|------|------|------|----------|
| **交易成本成分** | **暂时性价差 (Transitory)** | 补偿 **正常经营成本**（融资、人力、清算等） | 价格在 bid/ask 间 **来回跳动** — **Bid/ask bounce** |
| **逆向选择成分** | **永久性价差 (Permanent)** | 补偿 **输给知情者** 的预期损失；从 **不知情者** 多收以 **交叉补贴** | 成交后 mid **单向漂移** — [Glosten-Milgrom](https://en.wikipedia.org/wiki/Glosten%E2%80%93Milgrom_model) 订单流更新信念 |

### 2.1 交易成本 / 暂时性成分

- 覆盖 **可预测、可分摊** 的运营成本
- 价格在 spread 内 **均值回归** — 不构成长期 adverse move

### 2.2 逆向选择 / 永久性成分

- 做市商与知情者交易 → **低卖高买** → **系统性亏**
- 须 **加宽 spread**，从 **不知情 taker** 多收 → **弥补** informed 损失
- 做市商根据 **订单流** **更新对基本面的预期**

→ [Ch 13 §4.2](../chapter-13-dealers/)

| HFT 视角 |
|----------|
| **Realized spread vs effective spread** 分解 — 暂时 vs 永久 |
| **Markout analysis**（成交后 1s/5s/60s mid move）量化 adverse selection |
| **VPIN** 等高 toxicity 代理 ↑ → **permanent component** ↑ |

---
