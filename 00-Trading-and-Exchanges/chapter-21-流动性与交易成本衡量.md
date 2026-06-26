# Ch 21 流动性与交易成本衡量 · Liquidity and Transaction Cost Measurement

> **Trading and Exchanges** · Larry Harris · **精读** · Part VI

本章系统探讨 **交易成本的组成**、**测量指标** 及其 **潜在缺陷** — 对频繁交易或 **大宗交易** 的投资者至关重要。

> **HFT 读者：** **Effective / realized spread、markout、implementation shortfall** 是 maker **PnL 归因** 与 buy-side **TCA** 的共同语言；与 [Ch 14](./chapter-14-买卖价差.md)、[Ch 20 Roll](./chapter-20-波动性.md)、[Ch 13](./chapter-13-做市商.md)、[Ch 18](./chapter-18-买方交易者.md)、[Ch 7 best execution](./chapter-07-经纪人.md) 衔接。

---

## 1. 交易成本的三大组成部分

交易成本 **≠ 仅佣金**，包含：

### 1.1 显性成本 (Explicit Costs)

成本会计可 **直接识别** 的支出：

| 项目 | 例子 |
|------|------|
| **佣金** | 付给经纪人 |
| **交易所费用** | 撮合、数据、接入 |
| **税费** | 印花税等 |
| **交易台固定成本** | 人员、软硬件 |

| HFT 视角 |
|----------|
| **Maker rebate / taker fee** — 显性，可精确建模 |
| **Co-lo、feed、cross-connect** — 固定 + 变动显性成本 |

### 1.2 隐性成本 (Implicit Costs)

对 **市场价格产生影响** 导致的成本：

| 类型 | 说明 |
|------|------|
| **买卖价差** | 市价单交易者支付 — [Ch 14](./chapter-14-买卖价差.md) |
| **市场冲击 (Market impact)** | 大买单 **推高**、大卖单 **压低** 价格 |

→ [Ch 18 暴露冲击](./chapter-18-买方交易者.md) · [Ch 15 大宗](./chapter-15-大宗交易者.md)

### 1.3 错失交易的机会成本 (Missed Trade Opportunity Costs)

| 情形 | 损失 |
|------|------|
| **未及时完成** | 订单拖延 |
| **未完全成交** | 部分 fill |
| **限价未成交** | 挂低价买单，价格 **一路上涨** → 错失利润 |

→ [Ch 4 限价期权](./chapter-04-交易指令与订单类型.md)

| HFT 视角 |
|----------|
| **Opportunity cost** 是 algo **urgency vs price** 的核心 trade-off |
| **Partial fill + hedge lag** — arb **leg risk** 的机会成本版 |

---

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

→ [Ch 13 逆向选择](./chapter-13-做市商.md) · [Ch 14 永久/暂时成分](./chapter-14-买卖价差.md)

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

→ [Ch 18](./chapter-18-买方交易者.md)

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

## 3. 测量偏差与经纪人博弈 (Biases and Gaming)

**没有完美基准** — 实际应用中的 **偏差** 与 **操纵**：

### 3.1 分拆订单偏差 (Split Orders)

| 问题 | 大单拆 **多笔小单** |
|------|---------------------|
| **低估** | **Effective spread**、**VWAP** — 首笔冲击 **抬高** 后续基准 |
| **稳健** | **Implementation shortfall**（decision price 基准）|

### 3.2 投资风格偏差

| 风格 | 基准偏差 |
|------|----------|
| **动量**（追涨杀跌） | 以 **开盘价** 为基准 → **高估** 成本 |
| **逆向** | → **低估** 成本 |

### 3.3 知情交易者偏差

| 模式 | 买后涨、卖后跌 |
|------|----------------|
| **滞后基准**（收盘、VWAP） | **低估** 知情者 **真实** 成本 |

→ [Ch 10](./chapter-10-知情交易者与市场效率.md)

### 3.4 经纪人博弈 (Gaming)

客户 **单一指标考核** → 经纪人 **牺牲客户利益优化指标**：

| 指标 | Gaming 行为 |
|------|-------------|
| **Effective spread** | 只挂限价、不 **主动取流动性** — 即使价格 **飞速逃离** |
| **VWAP** | 故意 **全天缓慢拆单** — 错过 **早盘最佳时机** |

→ [Ch 7 委托代理](./chapter-07-经纪人.md) · **多指标 TCA + IS**

| HFT 视角 |
|----------|
| **Broker algo gaming** vs **honest IS minimization** |
| 考核 **IS + fill rate + risk** 组合，非单一 spread |

---

## 4. 交易成本冰山 (Plexus Iceberg)

**Plexus Group** 隐喻：

```
        ╱ 显性：佣金、部分 impact  ╲   ← 水面之上
═══════╪═══════════════════════════╪═══════
        ╲  经理择时成本              ╱
         ╲ 交易员择时成本（交单延误）╱   ← 水面之下
          ╲  错失机会成本           ╱
```

| 水下成本 | 说明 |
|----------|------|
| **经理择时** | **决定交易** → **提交交易台** 期间不利变动 |
| **交易员择时** | **交易台** → **交给经纪人** 期间延误与变动 |
| **错失机会** | 未成交损失 |

> **水下隐形成本往往远大于水上可见成本**

| HFT 视角 |
|----------|
| **Latency** 压缩 **交易员择时** 层 — HFT 价值主张之一 |
| **PM alpha timing** 若差 → IS 巨大，与 **执行无关** 也记在执行账上 |

---

## 5. 计量经济学方法 (Econometric Methods)

缺乏 **精确日内报价 / 订单流** 时，用 **统计模型** 评估 **市场整体** 成本：

### 5.1 价格反转模型

利用 **买卖价差跳动** 的 **价格反转（负序列相关）** 推算成本。

| 经典 | **Roll 序列协方差价差估计** — [Ch 20](./chapter-20-波动性.md) |

### 5.2 订单流模型

| 例子 | **Glosten-Harris 模型** |
|------|-------------------------|
| **方法** | 回归将价格变化拆解为： |
| | **永久性影响** — 知情交易（**逆向选择**） |
| | **暂时性影响** — 提供即时性（**spread / 交易成本**） |

→ [Ch 14](./chapter-14-买卖价差.md) · [Ch 20](./chapter-20-波动性.md)

| HFT 视角 |
|----------|
| **Kyle lambda**、**Amihud illiquidity** 同类文献线 |
| **Trade sign regression** — 从 tick 推 **permanent vs temporary impact** |

---

## 6. 指标选用速查

| 指标 | 最适合 | 主要缺陷 |
|------|--------|----------|
| **Effective spread** | 单笔、散户、中点稳定 | 拆单低估；忽略 opportunity |
| **Realized spread / Markout** | **做市商 PnL**、adverse selection | 需选 horizon；inform 后 drift |
| **Implementation shortfall** | **机构综合绩效**、大单 | 需准确 **decision time** |
| **VWAP** | 日内相对市场平均 | Gaming；拆单偏差 |
| **Roll / G-H** | 无 L2 数据的市场级研究 | 假设强、估计噪声 |

---

## 7. 本章总结

| 要点 | 含义 |
|------|------|
| **三类成本** | 显性 · 隐性 · 错失机会 |
| **基准法** | Effective · Realized · IS · VWAP — **各有意涵** |
| **分解** | Effective = Realized + **输给知情者** |
| **偏差** | 拆单、风格、知情、**gaming** |
| **冰山** | 择时 + 机会成本 **> 佣金** |
| **计量** | Roll 反转 · Glosten-Harris 订单流 |

> **HFT 读者 takeaway：** 做市日报 **realized spread @ 1s/5s/60s**；buy-side 用 **IS vs arrival price**。若只优化 **effective spread** 会 **gaming**（Ch 7 代理问题）。`orderbook.go` 加 **成交日志 + 事后 mid** 即可算 **markout** — M3 自然延伸。

---

## 相关章节

- 上一章：[chapter-20-波动性.md](./chapter-20-波动性.md)
- 下一章：[chapter-22-绩效评估与预测.md](./chapter-22-绩效评估与预测.md)
- 价差理论：[chapter-14-买卖价差.md](./chapter-14-买卖价差.md)
- 做市与逆向选择：[chapter-13-做市商.md](./chapter-13-做市商.md)
- 买方执行：[chapter-18-买方交易者.md](./chapter-18-买方交易者.md)
- Best execution：[chapter-07-经纪人.md](./chapter-07-经纪人.md)
- 练手：[00-practice-go-dex M3](./00-practice-go-dex/notes/milestone-03-价差与流动性/)
