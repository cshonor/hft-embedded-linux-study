# 第2章 Rust 基础与交易工程搭建

> 不重讲语法。只订热路径规矩：类型、错误、目录、编译档。

← [第1章](./chapter-01-rust与量化交易概述.md) · 下一章：[第3章 行情](./chapter-03-行情数据采集与清洗.md)

---

## 价格与数量

和 P10 一样：**禁止用 `f64` 当撮合价格**。

```rust
type Price = i64; // 1 tick = 0.01，100.00 元写成 10000
type Qty   = i64;
```

demo 里写在 [`types.rs`](./demo/src/types.rs)。研究侧算夏普可以用 float；一旦进 Book / 风控 / 发单，切回整数。

对照 [17 溢出](../17-rust-foundation/00-Book/09-error-handling/9.1.4-整数溢出与wrapping.md)：release 默认溢出是 wrap，热路径要自己想清楚，不要靠 panic。中间价 `(bid+ask)/2` 对正数是截断，P10 C++ 一样，两套对照时不要改成四舍五入。

---

## 热路径五条（新手最容易犯规）

| 不要 | 为什么 | 改成 |
|------|--------|------|
| `clone()` 行情结构 | 隐性分配 + 拷贝 | 传引用；所有权留在环的 slot 里 |
| 热路径 `Box` / `Vec::push` 扩容 | 分配器抖动 | 启动时 `with_capacity`；满了丢冷事件 |
| `unwrap()` / `expect()` | 实盘会直接炸进程 | `Result`，失败变成拒单/记日志 |
| `dyn Strategy` 每个 tick | 虚调用 + 难内联 | 泛型 `S: Strategy` 或 enum dispatch |
| `async` 收 tick | 执行器不在热路径预算里 | 同步 `loop { pop(); handle(); }` |

本模块 demo 为了代码短，`engine` 里对入簿订单用了一次 `clone`。这是教学债：生产应把 Event 存在环的槽里，引擎只借引用。看见 `clone` 不要当成「热路径标准写法」。

所有权细账去 [17 Book 所有权](../17-rust-foundation/00-Book/04-ownership/)。`Result` 去 [17 §9.2](../17-rust-foundation/00-Book/09-error-handling/9.2-Result-与可恢复的错误.md)。HFT 的 `dev/release` 对照 [17 §9.1.2](../17-rust-foundation/00-Book/09-error-handling/9.1.2-dev-release与HFT编译配置.md)：debug 要 overflow panic，release 要自己保证不变成 silent wrap。

---

## 工程目录（建议）

```
workspace/
  Cargo.toml          # [workspace] members
  market-data/        # 解析、归一化（可稍慢）
  engine/             # Book + 策略 + 风控（热）
  backtest/           # 回放，复用 engine
```

`17` 里已有 cargo 工作区示意：[14.3-hft-workspace-demo](../17-rust-foundation/00-Book/14-cargo-crates/14.3-hft-workspace-demo/)。本模块 [`demo/`](./demo/) 是单 crate：`lib` 测规则，`src/bin/hft_demo.rs` 跑回放。

`unsafe` **只允许**出现在无锁队列的 `push/pop`。其余撮合、策略、风控必须安全 Rust。这就是 P8「unsafe 边界」那句话。当前 demo **零处 unsafe**。

---

## 本仓库怎么编

Windows 没有原生 `g++`/`rustc` 也没关系，和 P1/P10 一样走 WSL：

```bash
cd /mnt/c/Users/12392/Desktop/hft/18-rust-quant/demo
cargo test
cargo run --release
```

`target/` 不要提交（已 gitignore）。不要为了「快」在热路径开 `lto` 之前先把测试写对。

---

## 卡住翻哪篇

| 卡住了… | 翻这里 |
|---------|--------|
| trait / 泛型 | [17 Book Ch10](../17-rust-foundation/00-Book/10-generics-traits-lifetimes/) |
| 无锁环语义 | [14 §7.2](../14-hft-engineering/chapter-07-lockless-data-structures-memory-layout/7.2-无锁FIFO队列.md) |
| 本仓库最小代码 | [`demo/`](./demo/) |
