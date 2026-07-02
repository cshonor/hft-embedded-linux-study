## 1. 本章定位

> **《深入浅出 DPDK》Ch 13 DPDK 与网络功能虚拟化** — **第三部分「应用篇」** 开篇

---

### 一、篇章切换

| 第一部分（Ch1–9） | 第二部分（Ch10–12） | **第三部分（Ch13+）** |
|-------------------|---------------------|----------------------|
| 裸金属 **数据面** 基础 | **虚拟化** I/O | **NFV / VNF 落地** |
| CPU/内存/I/O/PMD | 透传、Virtio、vhost | **电信云、运营商、SDN/NFV** |

**本章问题：** 传统专有网元 → **通用 X86 + 虚拟化** 后，DPDK 如何在 **NFVI** 层支撑 **VNF** 高性能？

---

### 二、本章讲什么

| 主题 | 要点 |
|------|------|
| **NFV 架构** | ETSI：NFVI、VNF、MANO |
| **OPNFV 生态** | OpenStack/KVM/OVS + **DPDK 数据面** |
| **扩展 API** | CryptoDev、Pattern Matching、Compression |
| **VNF 方法论** | I/O vs 计算密集、闭环调优 |
| **接口与 QoS** | IVSHMEM/Virtio/SR-IOV、CAT/CMT/MBM |
| **案例** | vBRAS Pipeline、Brocade vRouter |

---

### 三、HFT 视角

| 场景 | 读法 |
|------|------|
| **交易所共置 tick** | **选读** — 架构不同，但 **VNF 分型 / 闭环调优** 与 [03 SysPerf](../../../13-Systems-Performance-2nd/) 同构 |
| **云化行情/网关** | 理解 **Virtio vs VF**、**LLC 争用** — 与 tail latency 相关 |
| **安全 VNF（IPSec）** | **CryptoDev / QAT** — 与 [Ch9 offload](../chapter-09-hardware-offload/) 延伸 |

→ [15 HFT 工程](../../../15-HFT-Low-Latency-Practice/) · [02-Advanced-Book](../../02-Advanced-Book/)

---

← [Ch 13 导读](../README.md) · 下一节 [2. NFV 架构](./section-2-NFV起源与架构.md)
