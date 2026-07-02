## 1. 本章定位

> **《深入浅出 DPDK》Ch 5 报文转发** — 报文如何在系统中 **走完生命周期** 并被 **高速匹配/分发**

---

### 一、本章讲什么

| 维度 | 内容 |
|------|------|
| **处理流水线** | 硬件卸载 + 软件各阶段（收包→分类→队列→加速→发包） |
| **框架模型** | **Run to Completion** vs **Pipeline (Packet Framework)** |
| **转发算法** | 精确匹配 Hash、**LPM**、**ACL** |
| **多核分发** | **Packet Distributor** — 同流同 Worker、保序 |

**承上启下：** [Ch4 同步互斥](../chapter-04-synchronization/) 提供 ring/LPM/ACL 的 **锁与无锁** 原语；本章讲 **如何用这些组件搭转发路径**；[Ch8 流分类](../chapter-08-flow-classification-multiqueue/) 讲 **硬件侧** 如何把流分给多核。

---

### 二、小节导航

| 节 | 主题 |
|----|------|
| [2](./section-2-网络处理模块划分.md) | 硬件/软件模块、卸载优先 |
| [3](./section-3-转发框架模型.md) | RTC vs Pipeline |
| [4](./section-4-核心转发算法.md) | Hash、LPM tbl24/tbl8、ACL Tier |
| [5](./section-5-报文分发机制.md) | Distributor、stream 亲和 |
| [6](./section-6-小结与索引.md) | 框架选型 · 交叉索引 |

---

### 三、HFT 视角

| 场景 | 典型框架 |
|------|----------|
| **单 tick 低延迟网关** | **Run to Completion** — 一核 RX→parse→策略→TX，逻辑简单 |
| **行情解析 + 多 stage 过滤** | **Pipeline** — parse / 查表 / action 分核，stage 间 **rte_ring** |
| **会话保序** | Distributor 或 **RSS 对称哈希**（Ch8）— 同流同核 |

→ [15 HFT 低延迟工程](../../../15-HFT-Low-Latency-Practice/)

---

← [Ch 5 导读](../README.md) · 下一节 [2. 处理模块划分](./section-2-网络处理模块划分.md)
