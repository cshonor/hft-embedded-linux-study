# 18 · Rust demo（对照 P10）

> 不是实盘。没有交易所协议、没有无锁环、不能上真钱。  
> 语义对齐 [P10 C++ demo](../../projects/P10-hft-prototype/part-a-demo/)：整数 tick、FIFO、STP、本地风控、库存偏斜做市。

## 跑

WSL（和本仓库 P1 / P10 同一习惯）：

```bash
cd /mnt/c/Users/12392/Desktop/hft/18-rust-quant/demo
cargo test
cargo run --release
cargo run --release -- --jump 0
```

| 命令 | 作用 |
|------|------|
| `cargo test` | 撮合 / 风控 / 做市 / 短回放 |
| `cargo run --release` | 默认 2 万 tick，带跳价 |
| `--jump 0` | 关掉跳价，对比逆向选择 |
| `--ticks N --seed N --hits P` | 与 P10 CLI 同名 |

## 文件

| 文件 | 对应 P10 | 干什么 |
|------|----------|--------|
| `src/types.rs` | `types.hpp` | 价格用 `i64` tick；`Event` |
| `src/book.rs` | `orderbook.hpp` | 限价簿：价格优先 + FIFO + STP |
| `src/strategy.rs` | `strategy.hpp` | 做市：中间价 ± 价差，库存偏斜 |
| `src/risk.rs` | `risk.hpp` | 出门前几次 `if` |
| `src/replay.rs` | `replay.hpp` | 合成行情（单线程 `Vec<Event>`） |
| `src/engine.rs` | `engine.hpp` | 一拍：市场 → 撤旧报价 → 风控 → 挂新 |
| `src/bin/hft_demo.rs` | `main.cpp` | CLI |
| `src/lib.rs` | `self_test.hpp` | `cargo test` |

阅读顺序：`types` → `book` → `strategy` → `risk` → `engine` → `replay`。

笔记入口：[../README.md](../README.md)。`unsafe`：本 crate **零处**。无锁队列留给 [14 §7.2](../../14-hft-engineering/chapter-07-lockless-data-structures-memory-layout/7.2-无锁FIFO队列.md)。
