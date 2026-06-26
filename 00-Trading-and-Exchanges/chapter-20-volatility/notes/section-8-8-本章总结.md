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
