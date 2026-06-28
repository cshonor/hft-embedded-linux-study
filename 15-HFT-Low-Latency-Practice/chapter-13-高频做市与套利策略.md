# 第13章 高频做市与套利策略

> **本仓库扩展 · Signal / Execution · 延迟套利 · 做市逻辑**  
> （原书 Ch9 = Java/JVM → [chapter-09](./chapter-09-java-jvm-低延迟系统.md)）

← 架构：[chapter-08 §7 Strategy](./chapter-08-超低延迟核心引擎开发.md#7-关键路径组件应用层) · 总览：[chapter-01](./chapter-01-高频交易基础与生态.md)

<!-- 章节深化待补充：inventory risk、quote skew、latency arb 实例 -->

---

## 与 Ch1–12 的关系

| 前置 | 本章 |
|------|------|
| 本地 LOB + μs 级 T2T | **何时报价 / 何时 arb** |
| OMS 内部风控 | 策略 **参数边界** |

→ [00-Trading-and-Exchanges](../00-Trading-and-Exchanges/)
