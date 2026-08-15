## 3. 外部价差 vs 做市商价差 (Outside vs Dealer Spreads)

**外部价差 通常 远宽于 做市商内部价差 (Dealer spread / BBO)**。

| 维度 | 做市商 (Dealer / HFT MM) | 价值交易者 |
|------|--------------------------|------------|
| **速度目标** | **快进快出** — 不关心绝对 V，只关心 **迅速平仓** | **设定价格的人** — 持有至 **回归基本面** |
| **时间风险** | 短 | **巨大不确定性** |
| **头寸规模** | 相对小、分散 | **单笔远大于 MM** |
| **库存 / 融资** | 低–中 | **高** |
| **交易频率** | **高** — 固定成本 **大量分摊** | **低** — 仅 **严重偏离** 才交易 |

```
BBO (dealer)     ——— 窄 ———  高频 turnover
Outside spread   ——— 宽 ———  低频、大 size、长 holding
```

| HFT 视角 |
|----------|
| **LOB 顶窄** 不代表 **无 deep value bid/offer** — value 在 **电话/ upstairs / 暗池 negotiate** |
| MM **在 BBO 竞争**；value **在 mispricing 阈值外等待** |
| **Flash crash** — MM 撤 BBO；value **若到位** 提供 **弹性** |

---
