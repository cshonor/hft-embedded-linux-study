# HFT Low-Latency Practice — 交易系统工程实践

**文件夹 15** · [返回总清单](../READING-LIST.md#与-15-hft-low-latency-practice-章节映射)

> **前置：** `08` TLPI → `09` 自制 OS → `09`–`14` 网络栈  
> 全链路 → [LEARNING-CHAIN.md](../LEARNING-CHAIN.md)

## 与网络板块的分界

| | `09`–`14` 网络技术栈 | 本文件夹（`15`） |
|---|--------------------------|-----------------|
| 维度 | PNP、UNP、协议、内核、DPDK | 交易系统整机工程 |

网络能力从 `09`→`14` 获取；本文件夹负责**整合落地**。

---

## 从零构建 HFT：路线图

| 步骤 | 内容 | 章节 |
|------|------|------|
| 1 | **架构** Gateway / Book / Strategy / OMS | [Ch1](./chapter-01-高频交易基础与生态.md) · [Ch8](./chapter-08-超低延迟核心引擎开发.md) |
| 2 | **硬件/OS** 绑核 · BIOS · Hugepage · Bypass | [Ch4](./chapter-04-硬件选型与服务器配置.md) · [Ch5](./chapter-05-操作系统内核极致调优.md) |
| 3 | **IPC** 无锁 Ring · 共享内存 | [Ch7](./chapter-07-无锁数据结构与内存布局.md) |
| 4 | **语言** C++ 热点 · Java/Python 边界 | [Ch8 §5](./chapter-08-超低延迟核心引擎开发.md#5-c-引擎编码规范热点路径) |
| 5 | **网络** 二进制协议 · 微波 | [Ch6](./chapter-06-低延迟网络与协议优化.md) |
| 6 | **FPGA** ns 级 | [Ch4 §3](./chapter-04-硬件选型与服务器配置.md#3-fpga-加速) |
| 7 | **测量** T2T · p99 | [Ch10](./chapter-10-延迟测量与基准压测.md) |

**入门实操：** [Ch1 实战启动建议](./chapter-01-高频交易基础与生态.md#实战启动建议)

---

## 章节（12 章）

| 章 | 笔记 | 状态 |
|----|------|------|
| 1 | [chapter-01 基础与生态](./chapter-01-高频交易基础与生态.md) | ✅ 总览 |
| 2 | [chapter-02 交易所与撮合](./chapter-02-交易所架构与撮合原理.md) | ✅ 要点 |
| 3 | [chapter-03 订单簿与行情](./chapter-03-订单簿深度与行情解析.md) | ✅ 要点 |
| 4 | [chapter-04 硬件与 FPGA](./chapter-04-硬件选型与服务器配置.md) | ✅ 要点 |
| 5 | [chapter-05 OS 调优](./chapter-05-操作系统内核极致调优.md) | ✅ 要点 |
| 6 | [chapter-06 网络与协议](./chapter-06-低延迟网络与协议优化.md) | ✅ 要点 |
| 7 | [chapter-07 无锁与内存](./chapter-07-无锁数据结构与内存布局.md) | ✅ 要点 |
| 8 | [chapter-08 核心引擎](./chapter-08-超低延迟核心引擎开发.md) | ✅ 要点 |
| 9 | [chapter-09 做市与套利](./chapter-09-高频做市与套利策略.md) | 待补充 |
| 10 | [chapter-10 延迟测量](./chapter-10-延迟测量与基准压测.md) | ✅ 要点 |
| 11 | [chapter-11 风控合规](./chapter-11-风控合规与滑点控制.md) | 待补充 |
| 12 | [chapter-12 实盘运维](./chapter-12-实盘上线与运维进阶.md) | 待补充 |

---

## 交叉阅读

- [08-TLPI](../08-The-Linux-Programming-Interface/) · [10-PNP](../10-Practical-Network-Programming/)
- [14-DPDK](../14-DPDK-Low-Latency-Network/) · [00-Trading-and-Exchanges](../00-Trading-and-Exchanges/)
- [16-Rust](../16-Rust-Quant-Trading-Guide/) · [09-system-low-level-hands-on](../09-system-low-level-hands-on/)
