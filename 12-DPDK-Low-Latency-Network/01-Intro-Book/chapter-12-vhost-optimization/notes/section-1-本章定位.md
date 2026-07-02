## 1. 本章定位

> **《深入浅出 DPDK》Ch 12 加速包处理的 vhost 优化方案** — Virtio **后端** 的用户态极致路径

---

### 一、与 Ch11 的配对

| | [Ch11 Virtio 前端](../chapter-11-virtio-paravirtualization/) | **Ch12 vhost 后端（本章）** |
|---|--------------------------------------------------------------|----------------------------|
| 位置 | **客户机** Guest 驱动 | **宿主机** 处理 virtqueue |
| 对象 | `net_virtio` PMD | **vhost-user** / vhost lib / vhost PMD |
| 优化 | 固定环表、Indirect desc | **零拷贝映射**、用户态轮询 |

**本书结语：** 虚拟化网络性能需 **前后端双管齐下** — 仅优化一端不够。

---

### 二、本章讲什么

| 主题 | 要点 |
|------|------|
| **演进** | Qemu+Tap → **vhost-net** → **vhost-user** |
| **DPDK 设计** | Unix socket 控制面、**mem_table 映射** |
| **编程接口** | **vhost lib** vs **vhost PMD (Ethdev)** |
| **实战** | `vhost-switch` + **VMDQ** |

---

### 三、HFT 视角

- **NFV / 云交换** — 本章是 **宿主机侧** 标准答案  
- **HFT 裸金属 / SR-IOV** — **选读**；理解 vhost 有助于对比 **为何 VF 更快**  
- OVS-DPDK、vhost-user 容器网络 — 与本章同族

→ [15 HFT 部署](../../../15-HFT-Low-Latency-Practice/)

---

← [Ch 12 导读](../README.md) · 下一节 [2. vhost 演进](./section-2-vhost演进与原理.md)
