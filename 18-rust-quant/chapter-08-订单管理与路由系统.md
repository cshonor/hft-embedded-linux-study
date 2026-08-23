# 第8章 订单管理与路由系统

> OMS 记住每张单的一生；路由决定发去哪个所。风控在出门前，不在 RTT 之后。

← [第7章](./chapter-07-实盘交易引擎开发.md) · 下一章：[第9章 风控](./chapter-09-风险控制与仓位管理.md)

---

## 生命周期

```
Created → Sent → Ack → PartialFill → Done
                   ↘ Reject / Cancelled
```

策略只产生「意图」。OMS 才持有 `order_id`、剩余量、是否在途。重复点撤、乱序成交回报，都在这里对账。

对照 [14 §2.3](../14-hft-engineering/chapter-02-exchange-architecture-matching/2.3-策略引擎与OMS.md)：违规单 **本地拒**，不要占用到交易所的往返。demo 还没有独立 OMS 对象：`live_bid_id` / `live_ask_id` 就是最小「在途表」。生产要把状态机写全，否则会重复挂单或撤已经没了的单。

---

## Cancel-Replace

FIFO 市场里改价 = 撤掉重挂 = **去队尾**（[14 §3.3](../14-hft-engineering/chapter-03-orderbook-depth-market-data/3.3-同价匹配算法.md)）。  
P10 / demo 每 tick 撤旧挂新，是教学简化，实盘要算「丢队首值不值」。价没变就不要撤。

Rust 里用 `HashMap<u64, LiveOrder>` 管在途单；热路径避免按 `String` 当 key。demo 的 `OrderBook` 内部已有 `index: HashMap<u64, BookLoc>`，撤单 O(价位队列扫描)，正确性优先。

订单类型（demo `self_test` / `cargo test` 都覆盖了）：

| 类型 | 行为 |
|------|------|
| Limit | 能成则成，剩余排队 |
| Market | 能吃多少吃多少，不挂 |
| IOC | 立刻成，剩余扔掉 |
| FOK | 不够量则整单不做 |

---

## 路由

多市场才需要：同一信号去 A 所还是 B 所，看手续费、队列、延迟。单市场 demo 路由 = 「只有一个下游 `submit`」。  
腿风险、跨所套利是 [14 §12.5](../14-hft-engineering/chapter-12-market-making-arbitrage/12.5-跨所套利与腿风险.md)，本模块不展开实现。

---

## 卡住翻哪篇

| 卡住了… | 翻这里 |
|---------|--------|
| 订单类型 IOC/FOK | [14 §3.2](../14-hft-engineering/chapter-03-orderbook-depth-market-data/3.2-撮合引擎三种场景.md) · demo `cargo test` |
| 网关出入 | [14 §2.1](../14-hft-engineering/chapter-02-exchange-architecture-matching/2.1-关键路径总览与网关.md) |
