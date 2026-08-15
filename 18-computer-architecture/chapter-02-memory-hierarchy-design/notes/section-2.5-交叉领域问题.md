## 2.5 交叉领域问题

存储器系统与 **处理器、I/O、一致性模型** 深度耦合。

### 推测执行与内存异常

| 问题 | 要点 |
|------|------|
| **推测性加载** | 乱序 CPU 可能在分支解决前 **推测访问** 内存 |
| **异常精确性** | 若推测访问触发 **页故障/权限错**，需能 **撤销** 并精确报告 |
| **实现复杂度** | 微架构需缓冲、重放 — 影响性能与安全（侧信道另论） |

| HFT 视角 |
|----------|
| 热路径 **无分支依赖的指针访问** 更易被硬件预取/推测帮助 |
| 不可预测分支 + 指针追逐 → IPC 下降、预取失效 |

---

### I/O 与缓存一致性

| 问题 | 要点 |
|------|------|
| **DMA** | 设备直接写内存 — CPU cache 与内存可能 **不一致** |
| **一致性协议** | 需 flush/invalidate 或 **cache-coherent DMA**（如 IOMMU、snoop） |
| **写穿/写回** | 影响设备何时看到 CPU 写入 |

| HFT 视角 |
|----------|
| 网卡 **DMA 写 descriptor ring** — 与 CPU 读同一区域的 **内存序 + cache 一致性** 要用 barrier（→ [10 PNP](../../../04.5-network-sockets/)） |
| DPDK **mmap UIO/vfio** — 用户态轮询也要理解 **何时看到设备写入** |

---

### 与全书其他章的衔接

```
Ch2 层次与 cache  ←→  Ch3 ILP（非阻塞 cache 喂流水线）
                 ←→  Ch5 TLP（多核一致性、false sharing）
                 ←→  Ch7 DSA（HBM、近存计算）
```


### 常见陷阱

- 忽略 DMA 与 CPU cache 的一致性问题 — 网卡 DMA 写入内存后，CPU 可能仍读 **旧 cache 副本** → 数据错误；需 barrier 或 cache-coherent DMA
- 认为推测执行总是安全的 — 推测加载可能触发 **本不该执行的页故障/权限错**；Spectre/Meltdown 就是利用推测执行的侧信道
- DPDK 轮询模式下忘记内存序 — 即使用户态轮询，也需确保 **读到设备写入** 的正确时序（rmb/wmb 或 volatile + atomic）

### 自测题（点击展开）

<details>
<summary>Q1. 什么是 cache-coherent DMA？为什么网卡 DMA 需要 it？</summary>

DMA 设备直接写内存，CPU cache 可能仍有旧副本。cache-coherent DMA（通过 IOMMU/snoop）确保设备写入后，CPU cache 对应 line 被失效或更新。无一致性 → CPU 读到 **过期数据**。

</details>

<details>
<summary>Q2. 推测执行如何影响内存系统？对 HFT 代码有什么启示？</summary>

乱序 CPU 可能在分支解决前推测访问内存 → 可能触发本不该发生的缺页/权限异常。启示：热路径的 **指针访问如果无分支依赖** 更容易被推测/预取帮助；不可预测分支 + 指针追逐 → IPC 下降、预取失效。

</details>

<details>
<summary>Q3. DPDK 用户态轮询为什么仍需关心 cache 一致性？</summary>

DPDK mmap UIO/vfio → 网卡 DMA 写 descriptor ring → CPU 轮询读。即使无内核参与，CPU cache 和设备写入之间 **仍需内存序保证**（rmb 确保看到数据后才读 ring tail）。无 barrier → 可能读到半更新状态。

</details>
---
