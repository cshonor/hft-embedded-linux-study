# 第11章 风控合规与滑点控制

> **OMS 内部拒单 · 合规 · 执行质量**

← [chapter-08 OMS](./chapter-08-超低延迟核心引擎开发.md#4-order-manager-oms) · 总览：[chapter-01 §1](./chapter-01-高频交易基础与生态.md#1-系统核心架构关键路径)

<!-- 章节深化待补充：pre-trade risk、fat finger、滑点度量 -->

---

## Ch1 已覆盖要点

- **内部风控**：超限订单 **本地拒绝** — 节省交易所 RTT
- **合规**：禁售/价格带 — 失败则 **不发 Gateway OUT**

→ [00-Trading-and-Exchanges](../00-Trading-and-Exchanges/)
