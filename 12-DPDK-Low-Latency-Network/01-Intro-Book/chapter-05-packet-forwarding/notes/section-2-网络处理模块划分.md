## 2. 网络处理模块划分

---

### 一、报文生命周期

完整处理链大致分为 **硬件** 与 **软件** 两部分：

```
输入 → 粗预处理 → 细分类 → 入队 → 调度
  → 硬件加速（加解密等）→ 出队调度 → 后处理 → 输出
```

| 阶段 | 典型职责 |
|------|----------|
| **输入 / 输出** | NIC RX/TX、DMA |
| **粗 / 细分类** | 包类型、五元组、ACL |
| **队列 / 调度** | QoS、优先级、多核分发 |
| **硬件加速** | checksum、TSO、IPsec offload |
| **后处理** | 统计、镜像、封装 |

---

### 二、性能原则

| 优先级 | 策略 |
|--------|------|
| **1. 硬件卸载** | 能用网卡/NIC feature 的 **尽量 offload** — 减 CPU、减访存 |
| **2. 软件算法** | Hash / LPM / ACL **空间换时间**、SIMD 加速（→ §4） |
| **3. 并行** | 多核 RTC 或 Pipeline + [Ch3 并行](../chapter-03-parallel-computing/) |

→ 网卡能力对照 [Ch8 offload](../chapter-08-flow-classification-multiqueue/notes/section-3-硬件流分类.md) · 内核路径 [13-LKN](../../../11-Linux-Kernel-Networking/)

---

### 三、与 DPDK 组件映射

| 模块 | DPDK 侧 |
|------|---------|
| 收发包 | **PMD** — [chapter-03-PMD](../chapter-03-PMD与轮询模式.md) |
| 缓冲 | **mbuf / mempool** — [chapter-02-mbuf](../chapter-02-mbuf与内存池.md) |
| 核间队列 | **rte_ring** — [Ch4 无锁](../chapter-04-synchronization/notes/section-5-无锁机制.md) |
| 查表 | **rte_hash / rte_lpm / rte_acl** — §4 |

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. 转发框架](./section-3-转发框架模型.md)
