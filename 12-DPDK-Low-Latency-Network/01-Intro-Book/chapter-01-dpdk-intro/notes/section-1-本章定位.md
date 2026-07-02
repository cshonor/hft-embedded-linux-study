## 1. 本章定位

> **《深入浅出 DPDK》Ch 1 认识 DPDK** — 从宏观理解 **DPDK 是什么、为何快、用在哪**

---

### 一、本章讲什么

DPDK（**Data Plane Development Kit**）用 **软件** 在 **通用 IA 多核** 上把包处理性能推到新高度。本章不深入 API 细节，而是建立 **全景图**：

| 主题 | 要点 |
|------|------|
| **硬件背景** | 加速器 / NPU / 多核 — DPDK 选 **IA 多核 + 软件** |
| **最佳实践** | 轮询、用户态驱动、绑核、大页、NUMA、SIMD |
| **方法论** | 专用优化、水平扩展、Cache 极致、理论+实测螺旋 |
| **应用** | NFV、计算节点协议栈、SPDK 存储 |
| **实例** | HelloWorld → Skeleton → L3fwd |

---

### 二、小节导航

| 节 | 主题 |
|----|------|
| [2](./section-2-硬件平台与DPDK定位.md) | 三类硬件平台、DPDK 定位 |
| [3](./section-3-性能最佳实践.md) | 轮询、PMD、亲和性、大页、SIMD |
| [4](./section-4-底层方法论.md) | 四条指导思想 |
| [5](./section-5-应用潜力.md) | 网络 / 计算 / 存储节点 |
| [6](./section-6-编程实例入门.md) | HelloWorld、Skeleton、L3fwd |

---

### 三、在 HFT 学习链上的位置

```
09 Rosen（内核栈 — 知道「绕过谁」）
    ↓
12-DPDK Ch1 认识 DPDK（本章）
    ↓
chapter-02 mbuf · chapter-03 PMD · chapter-05 组播
    ↓
15 HFT ch06 低延迟网络
```

**前置：** 理解内核 **中断/NAPI/syscall** 路径 — [13-LKN](../../../11-Linux-Kernel-Networking/) · [ULK Ch4 中断](../../../04-Understanding-Linux-Kernel/chapter-04-interrupts-and-exceptions/)

---

← [Ch 1 导读](../README.md) · 下一节 [2. 硬件平台](./section-2-硬件平台与DPDK定位.md)
