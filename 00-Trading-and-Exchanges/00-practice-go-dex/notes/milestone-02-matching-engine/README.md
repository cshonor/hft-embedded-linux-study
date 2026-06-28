# M2 · 撮合引擎（价格–时间优先）

> **理论：** [Ch 6 指令驱动市场](../../chapter-06-order-driven-markets/)

## 本里程碑目标

- **价格优先**：买价高者优先；卖价低者优先
- **同价时间优先**：FIFO
- 输出 **Trade** 列表；支持 **部分成交**、剩余挂单
- **`BestBid()` / `BestAsk()`**：簿顶最优价 — M4 Level 1 行情
- **`MarketMaker`**：[marketmaker.go](../../code/marketmaker.go) 定时挂买一/卖一，作散户对手盘（[Ch 2 §1](../../chapter-02-trading-stories/notes/section-1-1-散户股票交易.md)）

## 代码

→ [../../code/](../../code/) · `go run .` 看 MM + 散户市价成交 demo

## 笔记

| 小节 | 文件 |
|------|------|
| 待补充 | [section-1-待补充.md](./section-1-待补充.md) |
