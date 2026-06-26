## 3. 库存管理与两大库存风险

**库存 (Inventories)** = 做市商 **手头头寸**；通常有 **目标库存 (Target inventories)**。

库存 **偏离目标** → 两类风险：

### 3.1 可分散的库存风险 (Diversifiable Inventory Risk)

| 来源 | **不可预测的随机价格波动** |
|------|---------------------------|
| **性质** | 相对温和 — 涨跌 **概率大致相等** |
| **分散** | 同时交易 **多种工具** 分散 |

| HFT 视角 |
|----------|
| **Multi-symbol MM**、**beta hedge** 降低 idiosyncratic inventory risk |
| **Vol scaling** — 高 vol 日缩小 per-name size |

### 3.2 逆向选择风险 (Adverse Selection Risk) — 最致命

**与知情交易者 (Informed traders) 交易的风险**（[Ch 10](../chapter-10-informed-traders-market-efficiency/)）。

| 模式 | 知情者在 **将涨时买、将跌时卖** |
|------|--------------------------------|
| **结果** | 与做市商成交后，价格常向 **不利于做市商** 方向走 → **亏损** |

| HFT 视角 |
|----------|
| **Maker PnL** 核心减法项；**VPIN / toxicity** 度量 |
| **Quote fade**  after large informed hit |

### 3.3 库存控制：调整报价

| 库存状态 | 报价调整 | 意图 |
|----------|----------|------|
| **太少，想买入** | **同时提高** bid **和** ask | 鼓励别人 **卖给你**；阻碍别人 **从你买走** |
| **太多，想清仓** | **同时降低** bid **和** ask | 鼓励别人 **买入**；减少继续 **卖给你** |

> **关键：** 双边 **同向移动**（shift whole quote），而非只动一侧 — 在 **维持 spread 宽度** 的同时 **偏移 mid** 调节 inventory。

| HFT 视角 |
|----------|
| **Inventory skew algorithm**：`mid' = mid + γ × (inventory − target)` |
| Go DEX / Rust 引擎若只做 **matching** 无 **自营 skew** — 用户各自为 maker 承担此逻辑 |

---
