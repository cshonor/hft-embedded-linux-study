# 第2章 交易所架构与撮合原理

> **交易所边界 · Gateway 接入点 · 与本地 LOB 的关系**

← 总览：[chapter-01 §1 架构](./chapter-01-高频交易基础与生态.md#1-系统核心架构关键路径)

---

## 1. 两套「订单簿」

| | **交易所 LOB** | **本地 Book Builder** |
|---|----------------|----------------------|
| **真相源** | 撮合引擎内 **权威** | Gateway 事件 **重建** |
| **延迟** | 远程 | **内存** — 策略直接读 |
| **用途** | 成交、监管 | **Signal 决策** |

HFT **不**依赖远程查询 BBO — 必须 **本地维护** 与 feed **同步** 的副本。

→ [chapter-03 订单簿解析](./chapter-03-订单簿深度与行情解析.md) · [00-Trading-and-Exchanges](../00-Trading-and-Exchanges/)

---

## 2. Gateway 在架构中的位置

```
        ┌─────────────────┐
        │   Exchange      │
        │  (Matcher+LOB)  │
        └───┬─────────┬───┘
            │ MD      │ Orders
      Gateway IN   Gateway OUT
            │         │
        ┌───▼─────────▼───┐
        │   HFT System    │
        └─────────────────┘
```

| Feed 类型 | 常见内容 |
|-----------|----------|
| **Incremental** | Add/Modify/Delete/Trade |
| **Snapshot** | 周期全量 — **recovery** |

---

## 3. 撮合原理（策略视角）

| 概念 | HFT 含义 |
|------|----------|
| **Price-time priority** | 同价 **先到先得** — 排队位置影响 fill |
| **BBO** | 策略最常消费的 **最优买卖** |
| **Latency arb** | 更快 **看见或预测** BBO 变化 |

**本章边界：** 深入撮合规则与监管 → [00-Trading-and-Exchanges](../00-Trading-and-Exchanges/)；本地实现 → Ch3/Ch8。
