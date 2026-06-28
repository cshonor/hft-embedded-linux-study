## 1. 本章定位

> **《深入浅出 DPDK》Ch 9 硬件加速与功能卸载** — 把 **简单重复** 的包处理下放给网卡

---

### 一、本章讲什么

软件侧优化（并行、内存、PCIe、PMD）之后，本章转向 **网卡硬件 offload**：

| 维度 | 内容 |
|------|------|
| **理念** | 控制面软件、数据面 **机械计算** 硬件化 |
| **三大类** | **计算/更新**、**分片 (TSO)**、**组包 (RSC)** |
| **DPDK 协同** | `rte_mbuf`、`ol_flags`、`l2_len` / `l3_len` |

**承上启下：** [Ch7 网卡调优](../chapter-07-nic-performance-optimization/) 讲 **软件路径**；[Ch8 流分类](../chapter-08-flow-classification-multiqueue/) 讲 **RSS/FD 分流**；本章讲 **Checksum/VLAN/TSO 等 NIC feature**。

---

### 二、小节导航

| 节 | 主题 |
|----|------|
| [2](./section-2-硬件卸载简介与演进.md) | 数据面/控制面、三类 offload |
| [3](./section-3-计算及更新功能卸载.md) | VLAN、PTP、Checksum、Tunnel |
| [4](./section-4-分片与组包卸载.md) | TSO、RSC |
| [5](./section-5-DPDK软件接口与协同.md) | ol_flags、TX 上下文描述符 |
| [6](./section-6-小结与索引.md) | 交叉索引 |

---

### 三、HFT 视角

| offload | 行情/ tick 典型 |
|---------|----------------|
| **RX/TX Checksum** | UDP 行情常用 — **减 CPU**，注意 **ol_flags 与能力位** |
| **VLAN 剥离** | 若接入带 Tag — 硬件剥 Tag **少拷贝** |
| **IEEE1588 PTP** | **交易所级时间戳** — 硬件戳 near-PHY |
| **TSO/RSC** | **TCP 发单** 更有用；纯 UDP tick 常 **不开** |

→ [15 HFT 低延迟工程](../../../15-HFT-Low-Latency-Practice/)

---

← [Ch 9 导读](../README.md) · 下一节 [2. 硬件卸载简介](./section-2-硬件卸载简介与演进.md)
