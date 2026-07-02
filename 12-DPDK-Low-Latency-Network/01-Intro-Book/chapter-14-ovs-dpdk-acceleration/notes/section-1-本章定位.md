## 1. 本章定位

> **《深入浅出 DPDK》Ch 14 Open vSwitch 中的 DPDK 性能加速** — **应用篇** 续

---

### 一、与 Ch13 的衔接

| Ch13 NFV | **Ch14 OVS** |
|----------|--------------|
| NFVI 整体架构、VNF 方法论 | **NFVI 内虚拟交换机** 这一关键软件层 |
| OPNFV 生态提及 OVS | **OVS 数据通路** 如何用 DPDK 落地 |
| 接口选型（Virtio / SR-IOV…） | **netdev-dpdk** 统一抽象 PHY / vhost / dpdkr |

**本章问题：** NFV 中 VM 互联、路由、OpenFlow 流表 — **OVS 性能瓶颈在哪？DPDK 如何改造快路径？**

---

### 二、本章讲什么

| 主题 | 要点 |
|------|------|
| **传统 OVS** | ovs-vswitchd（慢路径）+ openvswitch.ko（快路径） |
| **瓶颈** | 内核转发、内核↔用户态切换 |
| **DPDK OVS** | OVS 2.4+：数据通路 **完全用户态** |
| **核心模块** | `dpif-netdev`、`netdev-dpdk` |
| **接口** | 物理 PMD、vhost-user/cuse、dpdkr（IVSHMEM） |
| **量化** | Intel 测试：PHY-PHY **11.4×**、PHY-VM-PHY 最高 **12.9×** |

---

### 三、HFT 视角

| 场景 | 读法 |
|------|------|
| **共置 tick 直连** | **选读** — 通常不用 OVS；理解 **虚拟交换机开销** 有助于云化行情网关选型 |
| **云 / NFV 行情分发** | OVS+DPDK 是 **多租户 VM 互联** 常见栈 — 与 [Ch13 VNF 调优](../chapter-13-dpdk-nfv/notes/section-4-VNF评估与性能分析.md) 同构 |
| **OpenFlow / SDN** | 慢路径流表学习 + 快路径转发 — 类比 **Match+Action** [Ch5](../chapter-05-packet-forwarding/) |

→ [15 HFT 工程](../../../15-HFT-Low-Latency-Practice/) · [02-Advanced-Book](../../02-Advanced-Book/)

---

← [Ch 14 导读](../README.md) · 下一节 [2. 传统 OVS](./section-2-传统OVS架构与瓶颈.md)
