## 2. 多核性能与可扩展性

---

### 一、Amdahl vs Gustafson

| 定律 | 关注点 | 结论 |
|------|--------|------|
| **Amdahl** | **时延 / 加速比** | 串行部分 **封顶** 加速 — 优化 **关键路径** |
| **Gustafson** | **吞吐量** | 核数 ↑ 时 **放大并行部分** — 包处理 **更适用** |

DPDK 目标：**吞吐随核数线性增长** — 靠 **资源局部化、少跨核共享、小临界区**。

→ [Ch1 水平扩展](../chapter-01-认识DPDK/notes/section-4-底层方法论.md)

---

### 二、NUMA 再强调

现代 MP 多为 **NUMA** — **跨节点** 访存/PCIe **极贵**。

DPDK 调优：**网卡、lcore、大页、mempool 同 Node** — 详见 [Ch2](../chapter-02-Cache与内存/notes/section-6-DDIO与NUMA.md)。

---

### 三、超线程 (Hyper-Threading)

| 特点 | 对 DPDK 的含义 |
|------|----------------|
| 1 物理核 → 2 **逻辑线程**，共享流水线与 Cache | I/O 密集、**IPC 要求低于计算密集** |
| 轮询 + 等内存时，另一逻辑线程可利用 **空闲流水线** | 可能 **小幅** 提升；也可能 **争用** 执行单元 — **需实测** |

**HFT 常见做法：** 热路径 **独占物理核**（`isolcpus`、关 HT 绑核）— 求 **确定性** 而非峰值吞吐。

---

### 四、cgroup 与 pthread

- DPDK lcore 底层是 **普通 pthread**  
- 可用 **cgroup** 限制/分配 **CPU 配额** — 改善 I/O 核 **闲置** 与混部场景资源隔离  

生产共置：DPDK 核 **cpuset 隔离** + cgroup **防 noisy neighbor**。

→ [ULK Ch7 调度](../../../06-Understanding-Linux-Kernel/chapter-07-process-scheduling/)

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. ILP](./section-3-指令级并发.md)
