## 1. 本章定位

> **《深入浅出 DPDK》Ch 3 并行计算** — 主频触顶后，靠 **并行度** 换吞吐

---

### 一、本章讲什么

高速包处理不能无限 **提主频**（功耗墙）— DPDK 靠两条线突破：

| 维度 | 内容 |
|------|------|
| **多核并发** | Gustafson、NUMA、超线程、cgroup |
| **微架构并行** | 超标量乱序、**SIMD**、`rte_memcpy` |

**承上启下：** Ch2 解决 **内存/Cache**；本章解决 **如何用满多核与 CPU 流水线**；**Ch4 同步互斥** 解决多核引入的 **锁与无锁** 问题。

---

### 二、小节导航

| 节 | 主题 |
|----|------|
| [2](./section-2-多核性能与可扩展性.md) | Amdahl/Gustafson、NUMA、HT、cgroup |
| [3](./section-3-指令级并发.md) | 超标量、乱序、IPC |
| [4](./section-4-数据并行与SIMD.md) | SSE/AVX、`rte_memcpy` |
| [5](./section-5-小结与索引.md) | 分治法代价 → 无锁预告 |

---

### 三、HFT 视角

- **吞吐型** 行情网关：追求 **PPS 随核数近线性扩展** — Gustafson 思维  
- **延迟型** 单 tick 路径：Amdahl **串行段**（锁、跨核队列）决定 tail latency  
- **SIMD**：批量 parse / checksum / memcpy — 与 **向量化解码** 同构  

---

← [Ch 3 导读](../README.md) · 下一节 [2. 多核扩展](./section-2-多核性能与可扩展性.md)
