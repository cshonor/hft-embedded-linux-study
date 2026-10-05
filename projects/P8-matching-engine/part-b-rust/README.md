# P8 Part B (Rust) — LOB 撮合引擎 Rust 版

> P8 规划的原话：**"C++ 版沉淀工程能力，Rust 版验证内存安全与零成本抽象"**。
> 本目录是 [part-a-lob](../part-a-lob/)（C++，181 行单文件）的 Rust 对照 + Phase 2 扩展。
> 地图与纪律在 [18-rust-quant](../../../18-rust-quant/)（尤其 ch02「热路径禁 clone、tick 用 i64」）。

## 范围

| 项 | 状态 |
|----|------|
| Phase 1：限价单撮合（价格优先 + FIFO + 部分成交 + 撤单） | ✅ 与 part-a 五用例**逐断言对齐** |
| Phase 2：Market / IOC / FOK + 成交回报（Trade 含 taker/maker id） | ✅ 6 个新增用例 |
| Phase 3：无锁 SPSC ring 衔接行情输入 | ⬜ 未做（届时才需要 `unsafe`，且只允许在 ring 的 push/pop 内） |
| Phase 4：绑核/大页/mlock + 延迟基准 | ⬜ 未做（目标 Pi 5，同 P10 part-b Phase 5） |

**安全边界：整个 crate 零 unsafe**（本阶段没有无锁队列，所以一行都不需要）。

## 构建与测试

```bash
cd projects/P8-matching-engine/part-b-rust
make test          # = cargo test --release（12 用例，与 part-a 对齐 5 + Phase 2 新增 6 + 对称性 1）
cargo clippy       # 干净（无警告）
```

本机验证（2026-10，rustc 1.99.0 / Ubuntu 24.04 x86_64）：**12/12 通过**。

## 设计要点（与 C++ 版的语义对照）

| 决策 | C++ part-a | Rust 版 | 理由 |
|------|-----------|---------|------|
| 价位索引 | `std::map`（红黑树） | `BTreeMap` | 同构；asks 取 `iter().next()`、bids 取 `iter().next_back()` |
| 价位内队列 | `std::deque` | `VecDeque` | 同构 FIFO |
| 价格/数量 | `int64_t`（×10000） | `type Price = i64` | 相同；浮点误差不进场 |
| 残余挂簿 | `rest()` | 同 | Limit 才挂；Market/IOC 剩余丢弃，FOK 先预扫描 |
| FOK 预扫描 | README 骨架 | `available()` 只读遍历 | 不够量 → 一笔不成交，对手簿原封不动 |
| 撤单 | 双簿线性扫 + id 索引 TODO | 同款线性扫 | 生产化时加 `id → (side, price)` 哈希索引（两版同债） |

## 与 18-rust-quant/demo 的关系

`18-rust-quant/demo/` 是 **P10 part-a 全链路**（行情回放→簿→策略→风控→PnL）的 Rust 对照；
本目录只做**撮合引擎内核**（P8 Phase 1+2），单测更细（FOK 预扫描、卖单侧对称、市价穿档）。
Phase 3 接无锁环时，从 demo 抄 `Book` 不如直接用本 crate——demo 作者笔记里也这么说
（18 ch11：「P8 的 Rust 重写可以直接抄 demo 的 Book，再换无锁环」）。

## 下一步（Phase 3/4 动工前必读）

- 无锁环的 unsafe 边界 → [18-rust-quant ch02](../../../18-rust-quant/chapter-02-Rust基础与交易工程搭建.md)
- 延迟基准口径 → [14-hft ch09](../../../14-hft-engineering/chapter-09-latency-measurement-benchmarking/README.md) · [06.6.5 SysPerf ch13 perf](../../../06.6.5-systems-performance/chapter-13-perf/)
- 延迟数字只在 Pi 5（绑核+隔离）上才有意义，本机 x86 桌面数字仅作功能回归
