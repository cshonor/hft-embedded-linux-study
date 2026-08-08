# P8 — 迷你撮合引擎（终极大作业）

> 实现一个限价订单簿撮合引擎：无锁 ring buffer + 绑核/Hugepage + Rust 重写。把前面所有模块的能力收口到一个 HFT 核心组件。

## 项目目标

这是整条学习路线的终局——一个能跑、能测、能剖的撮合引擎。C++ 版沉淀工程能力，Rust 版验证内存安全与零成本抽象。每一行代码都能对应到前面某个模块学过的原理。

## 交付物

### Version A：C++ 版

- [ ] 限价订单簿（LOB）数据结构（按价格层级 + 时间优先）
- [ ] 撮合引擎：限价单/市价单、IOC/FOK、成交回报
- [ ] 无锁 SPSC ring buffer（行情入 / 撮合出）
- [ ] 绑核 + `SCHED_FIFO` + 大页 + `mlock`
- [ ] 行情输入：UDP 组播行情解析（复用 P6/P7）
- [ ] 延迟基准：单笔撮合 p50/p99/p999

### Version B：Rust 重写

- [ ] 同功能 Rust 实现（`unsafe` 仅限无锁队列）
- [ ] 所有权/借用验证无数据竞争
- [ ] 对比 C++ 版：延迟、代码安全、开发体验

## 覆盖模块

| 模块 | 用到什么 |
|------|----------|
| [`21` hft-engineering](../../21-hft-engineering/) | LOB、撮合、无锁、绑核、HFT 工程全链 |
| [`22` rust-quant](../../22-rust-quant/) | Rust 所有权、零成本抽象、unsafe 边界 |
| [`23` markets-microstructure](../../23-markets-microstructure/) | Harris：订单类型、撮合规则、queue priority |

## 前置

[P4](../P4-kernel-module/) + [P5](../P5-raspberry-pi-embedded/) + [P7](../P7-dpdk-forwarder-profiling/)（内核/嵌入式/网络性能全过关）。

## 学习目标

- LOB 数据结构设计（价格层级、时间优先、O(1) 撮合）
- 无锁数据结构的 memory order 与 ABA 问题
- 绑核/大页/mlock 的端到端低延迟配置
- UDP 组播行情解析与撮合引擎的衔接
- Rust 在 HFT 场景的安全/性能取舍

## 里程碑

1. **M1** LOB 数据结构 + 限价单撮合（单线程正确性）
2. **M2** 市价单 + IOC/FOK + 成交回报
3. **M3** 无锁 ring buffer 衔接行情输入
4. **M4** 绑核/大页/mlock，延迟基准
5. **M5** perf/bpftrace 剖析，优化尾延迟
6. **M6** Rust 重写，对比验证

## 参考模块

- [21-hft-engineering/](../../21-hft-engineering/) — ch01-12 全章（撮合/无锁/调优/延迟测量）
- [22-rust-quant/](../../22-rust-quant/) — Rust 量化
- [23-markets-microstructure/](../../23-markets-microstructure/) — Harris 交易与交易所、Go LOB/DEX 练手
