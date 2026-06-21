# HFT Low-Latency Practice — 交易系统工程实践

**文件夹 11** · 低延迟工程落地 · [返回总清单](../READING-LIST.md#与-11-hft-low-latency-practice-章节映射)

> **定位：** **动手实现**（C++ 工程）— 整机调优、交易所对接、架构、压测。  
> **前置：** `01` CSAPP → `02` SysPerf → `03` BPF（`05`–`10` 可并行补）  
> 全链路 → [LEARNING-CHAIN.md](../LEARNING-CHAIN.md)

## 与网络板块的分界

| | `07`–`10` 网络技术栈 | 本文件夹（`11`） |
|---|--------------------------|-----------------|
| 维度 | 报文、协议、网卡、收发模型 | 交易系统整机工程 |
| 内容 | TCP/IP、Socket、内核栈、DPDK | 对接规范、架构、压测、运维 |
| C++ HFT 书 | — | 仅作**工程案例参考** |

网络能力从 `07→08→09→10` 获取；本文件夹负责**整合落地**。

## 章节（12 章）

| 章 | 笔记 |
|----|------|
| 1 高频交易基础与生态 | [chapter-01](./chapter-01-高频交易基础与生态.md) |
| 2 交易所架构与撮合原理 | [chapter-02](./chapter-02-交易所架构与撮合原理.md) |
| 3 订单簿深度与行情解析 | [chapter-03](./chapter-03-订单簿深度与行情解析.md) |
| 4 硬件选型与服务器配置 | [chapter-04](./chapter-04-硬件选型与服务器配置.md) |
| 5 操作系统内核极致调优 | [chapter-05](./chapter-05-操作系统内核极致调优.md) |
| 6 低延迟网络与协议优化 | [chapter-06](./chapter-06-低延迟网络与协议优化.md) |
| 7 无锁数据结构与内存布局 | [chapter-07](./chapter-07-无锁数据结构与内存布局.md) |
| 8 超低延迟核心引擎开发 | [chapter-08](./chapter-08-超低延迟核心引擎开发.md) |
| 9 高频做市与套利策略 | [chapter-09](./chapter-09-高频做市与套利策略.md) |
| 10 延迟测量与基准压测 | [chapter-10](./chapter-10-延迟测量与基准压测.md) |
| 11 风控合规与滑点控制 | [chapter-11](./chapter-11-风控合规与滑点控制.md) |
| 12 实盘上线与运维进阶 | [chapter-12](./chapter-12-实盘上线与运维进阶.md) |

## 参考书映射

详见 [READING-LIST.md §与 10 映射](../READING-LIST.md#与-11-hft-low-latency-practice-章节映射)

## 交叉阅读

- 网络技术栈 → [04](../07-TCP-IP-Illustrated-Vol1/) · [05](../08-UNP-Vol1/) · [06](../09-Linux-Kernel-Networking/) · [12](../10-DPDK-Low-Latency-Network/)
- 跨模块总览 → [CROSS-MODULE-GUIDE.md](../CROSS-MODULE-GUIDE.md)
- Rust 工程 → [12-Rust-Quant-Trading-Guide](../12-Rust-Quant-Trading-Guide/)
