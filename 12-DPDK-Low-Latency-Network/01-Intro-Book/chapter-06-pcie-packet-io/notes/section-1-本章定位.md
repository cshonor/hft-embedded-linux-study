## 1. 本章定位

> **《深入浅出 DPDK》Ch 6 PCIe 与包处理 I/O** — 从 **CPU 优化** 转向 **网卡 I/O 与 DMA**

---

### 一、本章讲什么

前几章聚焦 **多核、Cache、同步、转发算法**；本章把视线移到 **PCIe 总线** 与 **包缓冲数据结构**：

| 维度 | 内容 |
|------|------|
| **PCIe 带宽** | TLP、Gen1/2/3 编码、有效带宽 vs 理论峰值 |
| **DMA 描述符环** | Head/Tail、RX/TX 基本操作 |
| **I/O 调优** | 减 MMIO、Cache Line 对齐、批量描述符 |
| **带宽量化** | 小包场景 PCIe **协议开销** |
| **mbuf / mempool** | 元数据布局、无锁池、**Core Cache** |

**承上启下：** [Ch5 报文转发](../chapter-05-packet-forwarding/) 讲 **软件框架**；本章讲 **I/O 如何搬包 + 缓冲如何组织**；[PMD 轮询](../chapter-03-PMD与轮询模式.md) 在驱动层 **消费** 描述符环。

---

### 二、小节导航

| 节 | 主题 |
|----|------|
| [2](./section-2-PCIe事务与带宽.md) | TLP、MRd/MWr、有效带宽 |
| [3](./section-3-DMA描述符环形队列.md) | 描述符环、DD 位、Tail 通知 |
| [4](./section-4-CPU与IO协奏优化.md) | MMIO 批量、Cache Line 合并 |
| [5](./section-5-PCIe净荷带宽计算.md) | 64B 小包开销示例 |
| [6](./section-6-Mbuf与Mempool.md) | head room、双 ring 池、本地缓存 |
| [7](./section-7-小结与索引.md) | 交叉索引 |

---

### 三、HFT 视角

- **线速小包** 瓶颈常在 **PCIe + 描述符/MMIO**，不在 CPU 算力  
- **mbuf 预分配 + per-core cache** — 热路径 **零 malloc**  
- **NUMA + 大页**（Ch2）与 **mempool 通道对齐** 叠加 — 同 Node 收发包  

→ [15 HFT 低延迟工程](../../../15-HFT-Low-Latency-Practice/)

---

← [Ch 6 导读](../README.md) · 下一节 [2. PCIe 事务与带宽](./section-2-PCIe事务与带宽.md)
