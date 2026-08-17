# 18 · Rust demo（对照 P10）

> 不是实盘。没有交易所协议、没有无锁环、不能上真钱。  
> 语义对齐 [P10 C++ demo](../../projects/P10-hft-prototype/part-a-demo/)：整数 tick、FIFO、STP、本地风控、库存偏斜做市。

## 跑测试

WSL（和本仓库 P1 / P10 同一习惯）：

```bash
cd 18-rust-quant/demo
cargo test
```

Windows PowerShell：

```powershell
wsl -d Ubuntu -- bash -lc 'cd /mnt/c/Users/12392/Desktop/hft/18-rust-quant/demo && cargo test'
```

## 文件

| 文件 | 对应 P10 | 干什么 |
|------|----------|--------|
| `src/types.rs` | `types.hpp` | 价格用 `i64` tick，不用 `f64` 撮合 |
| `src/book.rs` | `orderbook.hpp` | 限价簿：价格优先 + FIFO + STP |
| `src/strategy.rs` | `strategy.hpp` | 做市：中间价 ± 价差，库存偏斜 |
| `src/risk.rs` | `risk.hpp` | 出门前几次 `if`：价格带 / 仓位 / 流速 |
| `src/lib.rs` | `self_test.hpp` | `cargo test` 覆盖上面几条 |

笔记入口：[../README.md](../README.md) · 卡住先翻 [第2章](../chapter-02-Rust基础与交易工程搭建.md) 和 [第9章](../chapter-09-风险控制与仓位管理.md)。

`unsafe`：本 crate **零处**。无锁队列留给以后抄 [14 §7.2](../../14-hft-engineering/chapter-07-lockless-data-structures-memory-layout/7.2-无锁FIFO队列.md)。
