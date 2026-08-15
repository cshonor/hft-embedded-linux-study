## 2. 基准价格方法 (Specified Price Benchmark Methods)

将 **实际成交价** 与 **指定基准价** 对比，衡量隐性成本。

### 2.1 有效价差 (Effective Spread / Liquidity Premium)

```
Effective spread = 2 × |Trade price − Mid at trade|
```

| 特点 | 与 **成交时刻报价中点** 比较 |
|------|------------------------------|
| **用户** | **散户** 最常用成本估算 |
| **含义** | 中点不变时，一笔 **往返交易** 的隐性成本 |

| HFT 视角 |
|----------|
| **Quoted spread vs effective spread** — 改善 BBO 成交 → effective < quoted |
| 每笔 fill 的 **first-order TCA** |

### 2.2 实现价差 (Realized Spread)

```
Realized spread = 2 × (Trade price − Mid at T+Δ)   [Δ = 5/10/60 min 等]
```

| 特点 | 与 **成交后一段时间的中点** 比较 |
|------|----------------------------------|
| **用户** | **做市商** 高度关注 |
| **含义** | 排除价格 **向不利方向漂移** 后，**平仓后真正实现的 spread 利润** |

**分解关系：**

```
Effective spread  ≈  Realized spread  +  Adverse selection (price move against LP)
```

→ [Ch 13 逆向选择](../chapter-13-dealers/) · [Ch 14 永久/暂时成分](../chapter-14-bid-ask-spreads/)

| HFT 视角 |
|----------|
| **Markout** = realized spread 的核心 — 按 **秒/分**  horizon 报表 |
| **Negative markout** → widen / pull quotes |

### 2.3 执行差额 / 实施缺口 (Implementation Shortfall)

**Andre Perold** 推广 — **「纸上投资组合法」**：

```
IS = Value(virtual portfolio at decision mid) − Value(actual portfolio)
```

| 分解 | **已完成** 部分的交易成本 + **未完成** 部分的错失机会成本 |
|------|----------------------------------------------------------|
| **评价** | 衡量交易绩效的 **最佳综合标准** 之一 |

| 基准 | **交易决定那一刻** 的报价中点 — **不受拆单抬价影响** |

→ [Ch 18](../chapter-18-buy-side-traders/)

| HFT 视角 |
|----------|
| **Arrival price benchmark** — 机构 algo **gold standard** |
| **Alpha decay** 从 decision 到 complete 计入 IS |

### 2.4 成交量加权平均价 (VWAP)

| 做法 | 将成交价与 **当日 VWAP** 对比 |
|------|-------------------------------|
| **目的** | 是否优于 **市场平均** 执行价 |

| HFT 视角 |
|----------|
| **VWAP algo** — 目标函数；**gaming** 风险见 §3 |
| 日内 **volume profile** 预测 |

---
