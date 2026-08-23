# 第10章 系统监控与性能调优

> 先能量，再谈快。平均值会撒谎，看 p50 / p99 / p999。

← [第9章](./chapter-09-风险控制与仓位管理.md) · 下一章：[第11章 总结](./chapter-11-生产部署与全链路总结.md)

---

## 测什么

| 指标 | 含义 | 本模块 demo | P10 |
|------|------|-------------|-----|
| compute | 弹出后策略+风控+下单 | `LatencyHist` p50/p99 | `latency` |
| queue | 事件在环里等多久 | **没有**（单线程） | `queue_wait`（WSL 上往往远大于 compute） |
| T2T | 交易所 NIC 入 → 我方 NIC 出 | 没有 | 也没有（要硬件时间戳） |

对照 [14 Ch9](../14-hft-engineering/chapter-09-latency-measurement-benchmarking/README.md)。Rust 用 `std::time::Instant` 做进程内直方图；生产再上 TSC / PTP。

`cargo run --release` 才有参考意义。debug 带溢出检查和没优化的 `BTreeMap`，p99 会被自己吓到。即便 release，WSL 上的数字也只是「本机相对值」，不是交易所 T2T。

直方图实现：demo 把每个 tick 的纳秒推进 `Vec`，结束时排序取分位。热路径里 `push` 已 `with_capacity`，不要在每个 tick `clone` 去排序——排序只在打印报表时做一次。

---

## 日志

热路径 **不准** `println!` / `log::info!`。格式化字符串会分配。  
做法：把固定布局的二进制记录推进日志环，冷线程再写成行。P10 还没做异步日志，但原则在 [14 §9.4](../14-hft-engineering/chapter-09-latency-measurement-benchmarking/9.4-异步日志.md)。

demo 只在 **回放结束** 打一份报表，这可以。不要在 `handle()` 里 `println!`。

监控面板（仓位、拒单数、是否 kill）走冷路径 HTTP，挂了也不许回头阻塞引擎。

---

## 调优顺序

1. 正确性测试（FIFO、STP、风控）——`cargo test`  
2. 去掉热路径分配（看自己是否还在 `handle` 里 `clone` Event）  
3. 再谈 CPU 亲和、大页、无锁环  

不要一上来就 `unsafe` SIMD。P8 Phase 5 的 perf/bpftrace 仍然适用，语言无关。

---

## 卡住翻哪篇

| 卡住了… | 翻这里 |
|---------|--------|
| 运维监控 | [14 Ch11](../14-hft-engineering/chapter-11-production-deployment-ops/README.md) |
| 为什么 P10 的 queue >> compute | [P10 design §6](../projects/P10-hft-prototype/docs/design.md) |
