## 1. 本章定位

> **《深入浅出 DPDK》Ch 10 X86 平台上的 I/O 虚拟化** — **第二部分「DPDK 虚拟化技术篇」** 开篇

---

### 一、篇章切换

| 第一部分（Ch1–9） | 第二部分（Ch10+） |
|-------------------|-------------------|
| 物理机上的 **CPU/内存/I/O/PMD** 优化 | **虚拟化环境** 中如何保持接近裸机 I/O 性能 |
| 裸金属 DPDK | **NFV / 云网** 中 DPDK in VM |

**本章核心：** Intel **硬件辅助虚拟化** + **I/O 透传（Pass-through）** — 让客户机内 DPDK **像物理机一样轮询硬件**。

---

### 二、本章讲什么

| 主题 | 要点 |
|------|------|
| **VT-x / EPT** | CPU、内存虚拟化基础 |
| **I/O 三种模型** | 全虚拟 / 半虚拟(virtio) / **透传** |
| **VT-d + SR-IOV** | DMA 重映射、PF/VF |
| **透传下 DMA** | EPT + IOTLB、**大页** |
| **配置陷阱** | BIOS、`intel_iommu=on` |

---

### 三、HFT 视角

| 场景 | 含义 |
|------|------|
| **共置裸金属** | 本章 **选读** — 直接 Ch1–9 物理机路径 |
| **托管 / 云 VM 收行情** | **必读** — 无 SR-IOV 透传则 **难达 HFT 延迟** |
| **SR-IOV VF** | 与 [Ch8 虚拟化实战](../chapter-08-flow-classification-multiqueue/notes/section-4-DPDK实战结合.md) 衔接 |

→ [15 HFT 部署形态](../../../15-HFT-Low-Latency-Practice/)

---

← [Ch 10 导读](../README.md) · 下一节 [2. X86 虚拟化概述](./section-2-X86虚拟化概述.md)
