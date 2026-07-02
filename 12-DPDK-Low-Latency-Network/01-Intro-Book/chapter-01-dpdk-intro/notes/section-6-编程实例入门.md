## 6. 编程实例入门

> 三个 **由小到大** 的经典示例 — 建立 DPDK 程序直觉

---

### 一、HelloWorld

| 要点 | 说明 |
|------|------|
| **最简入门** | 理解 DPDK 程序 **骨架** |
| **`rte_eal_init()`** | 初始化 **EAL**（Environment Abstraction Layer）— 大页、lcore、PCI 等 |
| **多核启动** | 各 lcore 执行 `rte_eal_remote_launch` 或等价 worker 入口 |

→ EAL 深潜：官方 [Programmer's Guide · EAL](https://doc.dpdk.org/guides/prog_guide/env_abstraction_layer.html) · 后续 [chapter-02 mbuf](../chapter-02-mbuf与内存池.md)

---

### 二、Skeleton

| 要点 | 说明 |
|------|------|
| **单核最小骨架** | 收包 → **不处理** → 转发 |
| **核心 API** | `rte_eth_rx_burst()` / `rte_eth_tx_burst()` |
| **目的** | 看清 **PMD 收发环** 最小闭环 |

→ [chapter-03 PMD](../chapter-03-PMD与轮询模式.md)

---

### 三、L3fwd

DPDK **最流行** 示例之一 — **三层转发**：

| 能力 | 说明 |
|------|------|
| 结合 HelloWorld + Skeleton | 多核 + 真实转发逻辑 |
| **Exact Match (Hash)** | 精确匹配转发 |
| **LPM** | 最长前缀匹配 — 路由表 |

**HFT 类比：** 行情 **UDP 五元组过滤**、简单 **ACL 转发** 与 L3fwd 结构类似（查表 → 选端口/out queue）。

→ 官方：[L3 Forwarding Sample](https://doc.dpdk.org/guides/sample_app_ug/l3_forward.html)

---

### 四、后续章节索引

| Ch1 主题 | 继续读 |
|----------|--------|
| Cache / 大页 / NUMA | [chapter-02-Cache与内存](../chapter-02-cache-and-memory/) 🔴 |
| mbuf / mempool | [chapter-02-mbuf](../chapter-02-mbuf与内存池.md) 🔴 |
| PMD / 轮询 | [chapter-03](../chapter-03-PMD与轮询模式.md) 🔴 |
| 零拷贝旁路 | [chapter-04](../chapter-04-零拷贝与用户态旁路.md) 🔴 |
| 组播行情 | [chapter-05](../chapter-05-组播行情接入.md) 🔴 |
| 内核栈对照 | [13-LKN](../../../11-Linux-Kernel-Networking/) |
| XDP / RDMA 选型 | [02-Advanced-Book](../../02-Advanced-Book/) |
| 工程落地 | [15 HFT ch06](../../../15-HFT-Low-Latency-Practice/) |

---

← [5. 应用潜力](./section-5-应用潜力.md) · 下一章 [Ch2 Cache与内存](../chapter-02-cache-and-memory/)
