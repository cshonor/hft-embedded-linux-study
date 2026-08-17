# 18 · Rust 量化交易

> **文件夹 18** · Phase 6 拓展 · [返回总清单](../README.md)  
> **前置：** [17-rust-foundation](../17-rust-foundation/)（所有权 / Result / 并发）  
> **对照：** [14-hft-engineering](../14-hft-engineering/)（工程骨架）· [P10 C++ demo](../projects/P10-hft-prototype/)

语法不在这里重讲。`17` 是字典；本文件夹只讲 **Rust 怎么落到行情 / 回测 / 引擎 / 风控**。

热路径仍以 C++ 为主（见 [14 Ch1.5](../14-hft-engineering/chapter-01-hft-fundamentals-ecosystem/1.5-编程语言选择.md)）。Rust 适合：同一套语义的安全对照、回测、冷路径、以及 P8 规划里的引擎重写。

---

## 章节

| 章 | 笔记 | 一句话 |
|----|------|--------|
| 1 | [概述](./chapter-01-rust与量化交易概述.md) | 为什么用 Rust、和 C++/P10 怎么分工 |
| 2 | [工程搭建](./chapter-02-Rust基础与交易工程搭建.md) | 热路径禁 clone；tick 用 i64；Cargo workspace |
| 3 | [行情采集](./chapter-03-行情数据采集与清洗.md) | 字节流 → 内部 Event；gap 要恢复 |
| 4 | [K 线与时间序列](./chapter-04-时间序列与K线处理.md) | 慢策略用 bar；HFT 热路径用 tick |
| 5 | [策略模型](./chapter-05-量化策略模型开发.md) | Signal ≠ Execution；做市等式 |
| 6 | [回测框架](./chapter-06-策略回测框架实现.md) | 同一套 Book/Strategy 重放；防未来函数 |
| 7 | [交易引擎](./chapter-07-实盘交易引擎开发.md) | 同步循环 + SPSC；热路径不用 async |
| 8 | [OMS 与路由](./chapter-08-订单管理与路由系统.md) | 生命周期 + 内部风控前置 |
| 9 | [风控仓位](./chapter-09-风险控制与仓位管理.md) | 价格带 / 仓位 / 流速，策略过不了门 |
| 10 | [监控调优](./chapter-10-系统监控与性能调优.md) | p50/p99；日志离开热路径 |
| 11 | [部署总结](./chapter-11-生产部署与全链路总结.md) | 全图收口；demo 怎么跑；明确不做 |

动手：[`demo/`](./demo/)（订单簿 + 做市 + 风控，`cargo test`）。

---

## 和别的文件夹

| 你卡住了… | 去哪 |
|-----------|------|
| `clone` / 生命周期 / `Result` | [17 The Book](../17-rust-foundation/00-Book/) |
| Gateway / LOB / 无锁环 | [14](../14-hft-engineering/) |
| 已经能跑的 C++ 全链路 | [P10 part-a](../projects/P10-hft-prototype/part-a-demo/) |
| 微观结构（FIFO / 价差） | [19](../19-markets-microstructure/) |

**不是实盘。** 没有交易所协议、没有 DPDK、不能上真钱。
