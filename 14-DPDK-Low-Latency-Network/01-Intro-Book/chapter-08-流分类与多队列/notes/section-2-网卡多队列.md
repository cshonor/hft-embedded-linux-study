## 2. 网卡多队列 (Multi-queue)

---

### 一、技术由来

| 驱动因素 | 说明 |
|----------|------|
| **多核 CPU** | 并行算力可用 |
| **高速 NIC** | 10G+ 单核 **跟不上** 线速 PPS |

**多队列：** 网卡提供 **多个硬件 RX/TX 队列** — 不同队列可由 **不同 CPU 核** 独立轮询处理。

→ 内核侧对照：[13-LKN RSS/NAPI](../../../13-Linux-Kernel-Networking/chapter-14-advanced-topics/)

---

### 二、DPDK 与多队列的天然契合

DPDK Packet I/O **原生多队列** — 建立 **核 / 内存 / 网卡队列** 三角 **亲和性**：

| 模型 | 做法 |
|------|------|
| **Run to Completion** | 一 lcore ↔ **专属 RX + TX 队列** — 只处理该队列报文 |
| **收益** | 高 **Cache 命中**；**无** 多线程争用同一队列的 **锁** |

→ [Ch2 per-core · NUMA](../chapter-02-Cache与内存/notes/section-4-Cache一致性与无锁设计.md) · [Ch1 绑核](../chapter-01-认识DPDK/notes/section-3-性能最佳实践.md)

---

### 三、配置要点

- **`rte_eth_dev_configure()`** — `nb_rx_queue` / `nb_tx_queue`  
- **队列 i** 绑定 **lcore j** + **NUMA socket** 与大页一致  
- **PMD** `rx_burst` / `tx_burst` ** per-queue 调用**

→ [chapter-03 PMD](../chapter-03-PMD与轮询模式.md)

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. 流分类](./section-3-硬件流分类.md)
