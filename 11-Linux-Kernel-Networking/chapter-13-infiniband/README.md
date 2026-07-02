# Ch 13 InfiniBand 网络 · InfiniBand

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**（客座：Dotan Barak, Mellanox）

> **RDMA** — 远程 **直接内存访问**：低延迟、高带宽、**CPU 卸载、零拷贝**。Linux 统一 **RDMA 栈** 覆盖 **InfiniBand、RoCE、iWARP**。共置 **🟡 选读** — 与 **TCP/UDP（Ch 11）**、**DPDK（旁路）** 对照；部分 **行情/存储** 场景用 **RoCE**。

---

## 本章概述

| | 内容 |
|---|------|
| **本章** | RDMA 基础 · 寻址 · 包格式 · PD/MR/QP/CQ · WR 异常 · ib_/ibv_ API |
| **前置** | [Ch 11 L4](../chapter-11-layer-4-protocols/) · [Ch 7 邻居/L2](../chapter-07-neighbouring-subsystem/) |
| **HFT 读法** | **RoCEv2** 在 **同机房 RDMA 行情/分布式缓存** 可能出现；发单仍以 **TCP/UDP 以太** 为主 |

---

## 小节笔记

| 节 | 笔记 | HFT |
|----|------|-----|
| 1. RDMA 与 InfiniBand 基础 | [notes/section-1-RDMA与InfiniBand基础.md](./notes/section-1-RDMA与InfiniBand基础.md) | 🟡 |
| 2. 硬件组件与寻址 | [notes/section-2-硬件组件与寻址.md](./notes/section-2-硬件组件与寻址.md) | 🟡 |
| 3. 数据包与管理实体 | [notes/section-3-数据包结构与管理实体.md](./notes/section-3-数据包结构与管理实体.md) | 🟡 |
| 4. RDMA 核心资源 | [notes/section-4-RDMA核心资源与数据结构.md](./notes/section-4-RDMA核心资源与数据结构.md) | 🟡 |
| 5. 工作请求与异常流 | [notes/section-5-工作请求处理与异常流.md](./notes/section-5-工作请求处理与异常流.md) | 🟡 |
| 6. 内核与用户空间 API | [notes/section-6-内核与用户空间API区别.md](./notes/section-6-内核与用户空间API区别.md) | 🟡 |

---

## 相关章节

- 上一章：[../chapter-12-wireless-in-linux/](../chapter-12-wireless-in-linux/)
- 下一章：[../chapter-14-advanced-topics/](../chapter-14-advanced-topics/)
- 用户态旁路：[12-DPDK-Low-Latency-Network](../../12-DPDK-Low-Latency-Network/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
