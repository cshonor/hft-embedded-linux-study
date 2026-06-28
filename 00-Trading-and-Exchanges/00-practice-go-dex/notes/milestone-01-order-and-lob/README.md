# M1 · 订单类型与 LOB 数据结构

> **理论：** [Ch 2 §1 散户股票交易](../../chapter-02-trading-stories/notes/section-1-1-散户股票交易.md#与-go-dex理解订单簿的最好切入点)（**订单簿入门故事**）· [Ch 4 交易指令与订单类型](../../chapter-04-orders-and-order-types/) · [Ch 5 市场结构](../../chapter-05-market-structures/)

## 本里程碑目标

- 定义 **市价单 / 限价单**（与 Harris 术语一致）
- **双边 LOB**：bid 降序、ask 升序；同价位 **时间优先** 队列
- 仅内存、单线程 — 先 correctness，再谈延迟

## 笔记

| 小节 | 文件 |
|------|------|
| 三层结构体（视频） | [section-1-三层结构体解析.md](./section-1-三层结构体解析.md) |
| 待补充 | [section-1-待补充.md](./section-1-待补充.md) |

## 代码

→ [../../code/](../../code/)（M1：`orderbook.go` · `orderbook_test.go`）
