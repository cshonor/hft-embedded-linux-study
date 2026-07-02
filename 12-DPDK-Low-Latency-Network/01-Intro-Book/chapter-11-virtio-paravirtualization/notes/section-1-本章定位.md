## 1. 本章定位

> **《深入浅出 DPDK》Ch 11 半虚拟化 Virtio** — 虚拟化篇：**透传之外** 的主流 I/O 路径

---

### 一、与 Ch10 的关系

| | [Ch10 I/O 透传](../chapter-10-x86-io-virtualization/) | **Ch11 Virtio（本章）** |
|---|------------------------------------------------------|-------------------------|
| 模型 | **半虚拟化** 的对立面 — 设备直通 | **半虚拟化** — 前后端协同 |
| 性能 | **最高**（近裸金属） | 吞吐/时延/抖动 **逊于透传** |
| 运维 | 难热迁、常需 SR-IOV | **标准化**、**可 live migration** |
| 拓扑 | VM 直控 NIC | 包可经 **宿主机**（防火墙/LB）再入 VM |

---

### 二、本章讲什么

| 主题 | 要点 |
|------|------|
| **Virtio 规范** | 前端（Guest）/ 后端（vhost 等）、特性协商 |
| **Virtqueue** | Descriptor / Available / Used 三环 |
| **驱动分层** | Linux 内核 vs **DPDK 用户态 PMD** |
| **DPDK 优化** | **固定可用环表**、**Indirect 描述符** |

---

### 三、HFT 视角

- **共置裸金属 / SR-IOV VF** → 优先 Ch10，本章 **选读**  
- **必须在 VM + virtio-net 收行情** → 本章 **必读** — 理解性能天花板与 vhost-user 等变体  
- DPDK `net_virtio` PMD 的优化对 **云 NFV** 更有代表性，对 **极致 tick** 通常 **非首选**

→ [15 HFT 部署](../../../15-HFT-Low-Latency-Practice/)

---

← [Ch 11 导读](../README.md) · 下一节 [2. Virtio 规范](./section-2-Virtio规范与使用场景.md)
