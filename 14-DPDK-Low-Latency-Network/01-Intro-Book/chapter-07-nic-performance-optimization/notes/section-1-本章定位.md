## 1. 本章定位

> **《深入浅出 DPDK》Ch 7 网卡性能优化** — 从 **网卡 + 软件路径** 榨干包处理性能

---

### 一、本章讲什么

前几章覆盖 **CPU、内存、并发、PCIe、mbuf**；本章聚焦 **网卡层** 的软件设计、硬件选型与参数配置：

| 维度 | 内容 |
|------|------|
| **收包模式** | 纯轮询 vs **混合中断轮询**（`l3fwd-power`） |
| **微架构优化** | Burst、时延隐藏、Cache Line 冲突、SIMD 描述符 |
| **平台调优** | Extended Tag、NUMA 就近、`isolcpus`、测试流量 |
| **队列参数** | RX 描述符环长度权衡 |

**承上启下：** [Ch6 PCIe/I/O](../chapter-06-pcie-packet-io/) 讲 DMA 与 MMIO；本章讲 **PMD 收发包路径如何调优**；[Ch8 多队列](../chapter-08-flow-classification-multiqueue/) 讲 **RSS/硬件分流**。

---

### 二、小节导航

| 节 | 主题 |
|----|------|
| [2](./section-2-轮询与混合中断模式.md) | 中断 vs 轮询、UIO/VFIO + epoll |
| [3](./section-3-IO性能深度优化.md) | Burst、批量、Cache Line、SIMD |
| [4](./section-4-平台优化与配置调优.md) | PCIe Tag、NUMA、isolcpus |
| [5](./section-5-队列长度及阈值设置.md) | 128 vs 512/1024 |
| [6](./section-6-小结与索引.md) | 交叉索引 |

---

### 三、HFT 视角

| 场景 | 倾向 |
|------|------|
| **极低延迟 tick** | **纯轮询** + 独占核 — 不用混合中断（首包延迟） |
| **共置 / 省 CPU** | 混合模式可试 — 需评估 **唤醒延迟** |
| **线速测试** | Extended Tag + NUMA + **随机多流** 测 RSS |

→ [15 HFT 低延迟工程](../../../15-HFT-Low-Latency-Practice/)

---

← [Ch 7 导读](../README.md) · 下一节 [2. 轮询与混合中断](./section-2-轮询与混合中断模式.md)
