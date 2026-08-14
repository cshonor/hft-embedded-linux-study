# 2.3 Actors（Actor 模型）

> 所属：**Concurrency Models** · [← 章索引](./README.md)

每个 actor **独占**状态；外部仅通过 **channel 消息**驱动变更 — 内部常无需 `Mutex`。

| 适合 | 风险 |
|------|------|
| 边界清晰、邮箱建模 | 单 actor 过慢 → **吞吐瓶颈** |

需整体设计**背压**与调度。

Book → [16.2 消息传递与通道](../../00-Book/16-fearless-concurrency/16.2-消息传递与通道.md) · ER Item 17 优先 channel
