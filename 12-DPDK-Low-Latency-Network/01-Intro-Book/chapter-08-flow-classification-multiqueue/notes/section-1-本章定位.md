## 1. 本章定位

> **《深入浅出 DPDK》Ch 8 流分类与多队列** — 海量报文如何 **分给多核**

---

### 一、本章讲什么

万兆以上速率，**单核** 不足以处理网卡流量。本章讲 **硬件多队列 + 流分类** 如何与 DPDK **多 lcore** 配合：

| 主题 | 要点 |
|------|------|
| **Multi-queue** | 队列 ↔ 核 绑定，Run-to-Completion |
| **Flow Classification** | RSS、Flow Director、QoS、包类型 offload |
| **实战** | 转发核 + 控制核分离、VF/Cloud Filter |
| **RMT** | Match+Action 统一抽象 |

---

### 二、小节导航

| 节 | 主题 |
|----|------|
| [2](./section-2-网卡多队列.md) | 多队列由来、DPDK 亲和性 |
| [3](./section-3-硬件流分类.md) | RSS、FD、QoS、描述符包类型 |
| [4](./section-4-DPDK实战结合.md) | 分而治之、虚拟化 VF |
| [5](./section-5-RMT抽象模型.md) | SDN RMT、Match+Action |
| [6](./section-6-小结与索引.md) | 总结与交叉阅读 |

---

### 三、HFT 场景

| 场景 | 典型用法 |
|------|----------|
| **UDP 组播行情** | RSS 按 **(sip,dip, sport,dport)** 散列到多核 **或** 单队列 + 单核（低 PPS） |
| **行情 + 控制通道** | RSS 负载均衡数据面；**Flow Director** 精准导向 **会话/控制** 队列 |
| **对称哈希** | 双向流同核 — 若做 **有状态** 过滤/会话表 |

→ [chapter-05 组播](../chapter-05-组播行情接入.md) · [15 HFT ch06](../../../15-HFT-Low-Latency-Practice/)

---

← [Ch 8 导读](../README.md) · 下一节 [2. 多队列](./section-2-网卡多队列.md)
