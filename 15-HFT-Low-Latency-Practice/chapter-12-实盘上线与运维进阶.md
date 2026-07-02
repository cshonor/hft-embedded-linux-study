# 第12章 实盘上线与运维进阶

> **上线清单 · 监控 · 故障与降级**

← [chapter-01 实战启动](./chapter-01-高频交易基础与生态.md#实战启动建议) · [chapter-10 测量](./chapter-10-延迟测量与基准压测.md)

<!-- 章节深化待补充：canary、kill switch、capture/replay 运维 -->

---

## 上线前检查（摘要）

| 项 | 说明 |
|----|------|
| **T2T 基准** | p99 达标 · 无退化 |
| **绑核/BIOS** | 与压测环境一致 |
| **风控** | OMS 限参 **生产值** |
| **Capture** | 行情/订单 **可 replay** |

→ [13-Systems-Performance](../13-Systems-Performance-2nd/)
