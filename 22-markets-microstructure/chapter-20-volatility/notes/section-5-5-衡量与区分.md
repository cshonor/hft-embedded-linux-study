## 5. 衡量与区分 (Measuring Volatility)

### 5.1 基本衡量

| 方法 | 说明 |
|------|------|
| **方差 / 标准差** | 常用 **总波动性** 指标 |
| **分解** | Total = Fundamental + Transitory（概念上） |

### 5.2 统计学区分

| 类型 | 价格变化特征 |
|------|--------------|
| **基本面 vol** | **不可预测**、**不回归** |
| **暂时性 vol** | 变化往往 **回归（反转）** |

### 5.3 负序列相关性 (Negative Serial Correlation)

价格 **回归** → 收益率 **负自相关**：

```
上涨后倾向下跌  ·  下跌后倾向上涨
        ↓
识别暂时性 vol 的强指标
```

### 5.4 Roll 模型

**Richard Roll** 的 **序列协方差价差估计** — 利用 **负相关**（由 **bid/ask bounce** 引起）估计 **交易成本** 与 **波动性构成** 的经典方法。

| HFT 视角 |
|----------|
| **Roll spread estimator**、**Corwin-Schultz** 等 — 从 OHLC 推 effective spread |
| **Autocorrelation of tick returns** @ lag 1 — transitory 诊断 |
| **Variance ratio tests** — random walk vs mean reversion |

---
