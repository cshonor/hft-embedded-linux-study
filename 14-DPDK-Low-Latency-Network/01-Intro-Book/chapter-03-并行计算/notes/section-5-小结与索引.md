## 5. 小结与后续索引

---

### 一、本章总结

**多核 = 分治法：**

| 收益 | 代价 |
|------|------|
| 单位时间 **更多任务**、CPU 利用率 ↑ | **上下文切换**（DPDK 轮询可减） |
| 吞吐 **近线性扩展**（理想） | **多核同步**、**Cache 一致性**、**伪共享** |

DPDK 应对：**资源局部化 + 无锁（Lockless）数据结构** — 实体书 **第四章「同步互斥机制」** 展开（ring、原子操作、RCU 类思想）。

```
Ch3 并行计算（本章）— 为何要并行、如何用满 CPU
    ↓
Ch4 同步互斥 — 如何不用锁也能安全共享
    ↓
Ch8 流分类与多队列 — 硬件+软件分核
```

---

### 二、后续章节索引

| Ch3 主题 | 继续读 |
|----------|--------|
| Cache / NUMA / per-core | [chapter-02-Cache与内存](../chapter-02-Cache与内存/) 🔴 |
| 多队列 / RSS | [chapter-08-流分类与多队列](../chapter-08-流分类与多队列/) 🔴 |
| mbuf / ring | [chapter-02-mbuf](../chapter-02-mbuf与内存池.md) 🔴 |
| 无锁 / 同步 | [chapter-04-同步互斥机制](../chapter-04-同步互斥机制/) 🔴 · [ULK Ch5 RCU](../../../06-Understanding-Linux-Kernel/chapter-05-kernel-synchronization/) |
| 体系结构 | [02-Hennessy](../../../02-Computer-Architecture-6th/) · [01-CSAPP Ch5](../../../01-CSAPP-3rd/chapter-05-optimization/) |
| HFT 绑核 / 扩展 | [15 HFT 工程](../../../15-HFT-Low-Latency-Practice/) |

---

← [4. SIMD](./section-4-数据并行与SIMD.md) · 下一章 [chapter-04-同步互斥](../chapter-04-同步互斥机制/) · [Ch2 Cache](../chapter-02-Cache与内存/)
