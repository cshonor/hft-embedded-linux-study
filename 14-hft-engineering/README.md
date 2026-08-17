# HFT Low-Latency Practice — 交易系统工程实践

**文件夹 16** · [返回总清单](../READING-LIST.md#与-14-hft-engineering-章节映射)

> **前置：** `03` TLPI → `05` LKD → `12`–`15` 网络栈  
> 全链路 → [README.md](../README.md)

## 与网络板块的分界

| | `12`–`15` 网络技术栈 | 本文件夹（`18`） |
|---|--------------------------|-----------------|
| 维度 | PNP、UNP、协议、内核、DPDK | 交易系统整机工程 |

网络能力从 `12`→`15` 获取；本文件夹负责**整合落地**。

---

## 从零构建 HFT：路线图

| 步骤 | 内容 | 章节 |
|------|------|------|
| 1 | **架构** Gateway / Book / Strategy / OMS | [Ch1](./chapter-01-hft-fundamentals-ecosystem/README.md) · [Ch8](./chapter-08-ultra-low-latency-engine-dev/README.md) |
| 2 | **硬件/OS** 绑核 · BIOS · Hugepage · Bypass | [Ch4 原理](./chapter-04-hardware-selection-server-config/README.md) · [Ch5 实操](./chapter-05-os-kernel-tuning/README.md) |
| 3 | **IPC** 无锁 Ring · 内存池 | [Ch7 无锁/内存（原书 Ch6§2–3）](./chapter-07-lockless-data-structures-memory-layout/README.md) |
| 4 | **语言** C++ 关键路径 | [Ch8（原书 Ch8）](./chapter-08-ultra-low-latency-engine-dev/README.md) · [Ch1 §5 语言选择](./chapter-01-hft-fundamentals-ecosystem/1.5-编程语言选择.md) |
| 5 | **网络** 交换机 · TCP/UDP · 包路径 · PTP | [Ch6 动态网络](./chapter-06-low-latency-network-protocol/README.md) |
| 6 | **FPGA / Crypto** ns 级 · 云端共址 | [Ch13（原书 Ch11）](./chapter-13-fpga-crypto-hft/README.md) · [Ch4 §4 硬件选型速查](./chapter-04-hardware-selection-server-config/4.4-硬件选型速查.md) |
| 7 | **测量** T2T 分段 · 异步日志 · Bypass 总纲 | [Ch9 日志/测量（原书 Ch7）](./chapter-09-latency-measurement-benchmarking/README.md) |

**入门实操：** [Ch1 实战启动建议](./chapter-01-hft-fundamentals-ecosystem/1.8-实战启动建议.md)

---

## 章节（13 章）

| 章 | 笔记 | 状态 |
|----|------|------|
| 1 | [chapter-01 基础与生态](./chapter-01-hft-fundamentals-ecosystem/README.md) | ✅ 总览 |
| 2 | [chapter-02 关键组件](./chapter-02-exchange-architecture-matching/README.md) | ✅ 要点 |
| 3 | [chapter-03 交易所动态与 LOB](./chapter-03-orderbook-depth-market-data/README.md) | ✅ 要点 |
| 4 | [chapter-04 硬件到 OS](./chapter-04-hardware-selection-server-config/README.md) | ✅ 要点 |
| 5 | [chapter-05 OS 调优 · 上下文切换（原书 Ch6§1）](./chapter-05-os-kernel-tuning/README.md) | ✅ 要点 |
| 6 | [chapter-06 动态网络（原书 Ch5）](./chapter-06-low-latency-network-protocol/README.md) | ✅ 要点 |
| 7 | [chapter-07 无锁与内存池（原书 Ch6§2–3）](./chapter-07-lockless-data-structures-memory-layout/README.md) | ✅ 要点 |
| 8 | [chapter-08 C++ 微秒征途（原书 Ch8）](./chapter-08-ultra-low-latency-engine-dev/README.md) | ✅ 要点 |
| 9 | [chapter-09 日志与 TTT 测量（原书 Ch7）](./chapter-09-latency-measurement-benchmarking/README.md) | ✅ 要点 |
| 10 | [chapter-10 风控合规](./chapter-10-risk-compliance-slippage/README.md) | ✅ 要点 |
| 11 | [chapter-11 实盘运维](./chapter-11-production-deployment-ops/README.md) | ✅ 要点 |
| 12 | [chapter-12 做市与套利（本仓库扩展）](./chapter-12-market-making-arbitrage/README.md) | ✅ 要点 |
| 13 | [chapter-13 FPGA 与 Crypto（原书 Ch11）](./chapter-13-fpga-crypto-hft/README.md) | ✅ 要点 |

---

## 交叉阅读

- [03-linux-userspace-api](../03-linux-userspace-api/) · [12-PNP](../04-cpp/M2-cpp-network-programming/)
- [13-DPDK](../13-dpdk/) · [19-markets-microstructure](../19-markets-microstructure/)
- [18-Rust](../18-rust-quant/) · [projects/P9-os-from-scratch](../projects/P9-os-from-scratch/)
