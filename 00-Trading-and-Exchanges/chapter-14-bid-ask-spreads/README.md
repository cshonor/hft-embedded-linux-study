# Ch 14 买卖价差 · Bid/Ask Spreads

> **Trading and Exchanges** · Larry Harris · **精读** · Part IV

本章深入剖析 **决定买卖价差宽窄的因素**，以及价差如何影响 **交易者策略与盈亏**。

> **Harris 全书最重要的一课（作者原话）：** **理解为什么不知情交易者总会亏损。**

> **HFT 核心章：** spread 分解、**Glosten-Milgrom**、maker/taker 权衡、**flow toxicity**；与 [Ch 13](../chapter-13-dealers/)、[Ch 10](../chapter-10-informed-traders-market-efficiency/)、[Ch 4 订单类型](../chapter-04-orders-and-order-types/)、[00-practice-go-dex M3](./00-practice-go-dex/notes/milestone-03-价差与流动性/) 直接衔接。

---

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 0. 为什么本章至关重要 | [notes/section-1-0-为什么本章至关重要.md](./notes/section-1-0-为什么本章至关重要.md) |
| 1. 价差的本质与竞争机制 | [notes/section-2-1-价差的本质与竞争机制.md](./notes/section-2-1-价差的本质与竞争机制.md) |
| 2. 价差的两大组成部分 (Spread Components) | [notes/section-3-2-价差的两大组成部分.md](./notes/section-3-2-价差的两大组成部分.md) |
| 3. 本书最重要的一课：不知情交易者为何总是亏损？ | [notes/section-4-3-本书最重要的一课-不知情交易者为何总是亏损.md](./notes/section-4-3-本书最重要的一课-不知情交易者为何总是亏损.md) |
| 4. 指令驱动市场中的均衡价差 | [notes/section-5-4-指令驱动市场中的均衡价差.md](./notes/section-5-4-指令驱动市场中的均衡价差.md) |
| 5. 决定价差的三大主要因素 (Primary Determinants) | [notes/section-6-5-决定价差的三大主要因素.md](./notes/section-6-5-决定价差的三大主要因素.md) |
| 6. 次要决定因素与代理变量 (Secondary Proxies) | [notes/section-7-6-次要决定因素与代理变量.md](./notes/section-7-6-次要决定因素与代理变量.md) |
| 7. 市场失灵 (Market Failure) | [notes/section-8-7-市场失灵.md](./notes/section-8-7-市场失灵.md) |
| 8. 指令类型 × 价差（总表） | [notes/section-9-8-指令类型-价差-总表.md](./notes/section-9-8-指令类型-价差-总表.md) |
| 9. 本章总结 | [notes/section-10-9-本章总结.md](./notes/section-10-9-本章总结.md) |

---

## 相关章节

- 上一章：[chapter-13-dealers](../chapter-13-dealers/)
- 下一章：[chapter-15-block-traders](../chapter-15-block-traders/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
