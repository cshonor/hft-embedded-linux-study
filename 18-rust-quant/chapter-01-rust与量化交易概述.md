# 第1章 Rust 与量化交易概述

> 语法在 17；这里只回答：Rust 在这条 HFT 路线里干什么、不干什么。

← [模块入口](./README.md) · 下一章：[第2章 工程搭建](./chapter-02-Rust基础与交易工程搭建.md)

---

## 先给结论

量化交易系统 = **行情进入 → 更新本地世界 → 决定下单 → 风控 → 发出去**。  
语言不改变这条链，只改变「谁保证你别把内存写爆」。

| | C++（P10 / 14 热路径） | Rust（本模块） |
|--|------------------------|----------------|
| 强项 | 生态、交易所 API、纳秒级调优资料多 | 编译期挡住数据竞争和大部分 UAF |
| 代价 | 正确性靠人肉 | 所有权要过编译器；热路径要少用智能指针 |
| 本仓库角色 | 已能跑的 demo | 同一语义的安全对照 + 回测 / 冷路径 |

P8 规划「C++ 版沉淀、Rust 版验证内存安全」——本文件夹就是那份地图。可运行的对照：

```bash
cd 18-rust-quant/demo
cargo test
cargo run --release
cargo run --release -- --jump 0    # 关掉跳价，对比逆向选择
```

---

## 三个速度档，不要混

| 档 | 延迟量级 | Rust 常见用法 | 本模块哪几章 |
|----|----------|----------------|--------------|
| 研究 / 日频 | 秒～分钟 | CSV 回测，随便 `clone` | Ch3–6 的慢路径 |
| 低延迟交易 | 百微秒～毫秒 | 同步引擎 + 有界队列；少分配 | Ch7–9 + demo |
| HFT 热路径 | 亚微秒 | 预分配、无锁、禁止 `dyn` / `async` / `unwrap` | 原则在这；硬件仍看 14 |

把研究代码的习惯（`unwrap`、`clone` 满天飞、每个 tick `async`）直接搬进引擎，编译能过、实盘会抖。三档用同一套 Book 语义，但 **分配和并发模型不能混**。

---

## 和 P10 的一张图

```
P10 C++                               本模块
replay.hpp  --SPSC-->  engine.hpp     replay.rs  （单线程 Vec<Event>）
                 →  orderbook.hpp          →  book.rs
                 →  strategy.hpp           →  strategy.rs
                 →  risk.hpp               →  risk.rs
                 →  PnL / p50·p99          →  engine.rs 报表
```

P10 用两线程 + 无锁环，是为了让你看见 `queue_wait`。Rust demo **故意单线程**：先把撮合/做市/风控语义写对，不把 `unsafe` 塞进第一课。要无锁环时抄 [14 §7.2](../14-hft-engineering/chapter-07-lockless-data-structures-memory-layout/7.2-无锁FIFO队列.md)，只把 `unsafe` 包在 queue 内部。

---

## 本模块怎么读（新手）

1. 本章建立分工：语法回 17，硬件回 14，这里只谈「落到交易链上」。  
2. 打开 [`demo/src/types.rs`](./demo/src/types.rs)，记住 **tick 是整数**。  
3. `cargo test`，对着失败回 `book.rs` 注释。  
4. `cargo run --release`，再跑一次 `--jump 0`，看 PnL 差在逆向选择，不是「策略更聪明」。  
5. 再读第 2、5、7、9 章；3、4、6、8、10 按需。

不要从本模块学 `let` / 所有权。那是 [17 Book Ch4](../17-rust-foundation/00-Book/04-ownership/) 的事。

---

## 卡住翻哪篇

| 卡住了… | 翻这里 |
|---------|--------|
| 所有权 / 借用 | [17 Book Ch4](../17-rust-foundation/00-Book/04-ownership/) |
| T2T 关键路径 | [14 §1.1](../14-hft-engineering/chapter-01-hft-fundamentals-ecosystem/1.1-系统核心架构.md) |
| 语言怎么选 | [14 §1.5](../14-hft-engineering/chapter-01-hft-fundamentals-ecosystem/1.5-编程语言选择.md) |
| FIFO / 价差业务语言 | [19](../19-markets-microstructure/) |
