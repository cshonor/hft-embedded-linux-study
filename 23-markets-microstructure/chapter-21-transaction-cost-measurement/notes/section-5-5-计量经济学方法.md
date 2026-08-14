## 5. 计量经济学方法 (Econometric Methods)

缺乏 **精确日内报价 / 订单流** 时，用 **统计模型** 评估 **市场整体** 成本：

### 5.1 价格反转模型

利用 **买卖价差跳动** 的 **价格反转（负序列相关）** 推算成本。

| 经典 | **Roll 序列协方差价差估计** — [Ch 20](../chapter-20-volatility/) |

### 5.2 订单流模型

| 例子 | **Glosten-Harris 模型** |
|------|-------------------------|
| **方法** | 回归将价格变化拆解为： |
| | **永久性影响** — 知情交易（**逆向选择**） |
| | **暂时性影响** — 提供即时性（**spread / 交易成本**） |

→ [Ch 14](../chapter-14-bid-ask-spreads/) · [Ch 20](../chapter-20-volatility/)

| HFT 视角 |
|----------|
| **Kyle lambda**、**Amihud illiquidity** 同类文献线 |
| **Trade sign regression** — 从 tick 推 **permanent vs temporary impact** |

---
