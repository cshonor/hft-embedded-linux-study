## 4. DPDK 底层方法论

> 四条 **指导思想** — 解释「为何 DPDK 这样设计」

---

### 一、专用负载下的针对性优化

- 网络负载 **可预测、可批处理**  
- **榨干** 平台特性 — 大页、DDIO、SIMD、无分支热路径  
- 非通用 OS 路径，而是 **为包处理定制**

---

### 二、追求可水平扩展的性能

| 目标 | 手段 |
|------|------|
| 吞吐随 **核数线性增长** | 多 lcore **并行** 收发包 |
| 少跨核共享 | **每核独立** 队列、mempool — 减锁与 false sharing |

→ 无锁环、per-core 队列 — [15 HFT ch07](../../../15-HFT-Low-Latency-Practice/)

---

### 三、向 Cache 索求极致

> **娴熟驾驭 Cache** 是极致性能的关键。

DPDK 大量实现优化 **围绕 Cache**：

- mbuf / mempool **预分配、对齐、局部性**  
- 数据结构 **紧凑、顺序访问**  
- 避免 false sharing（cache line 争用）

→ [CSAPP Ch6](../../../01-CSAPP-3rd/chapter-06-memory-hierarchy/) · [chapter-02-Cache与内存](../chapter-02-cache-and-memory/)

---

### 四、理论分析 + 实践推导

**螺旋上升：**

```
分析 → 推测 → 原型 → 跑数据 → 再分析 → …
```

与 [03 SysPerf](../../../13-Systems-Performance-2nd/) 方法论一致 — **用 perf/BPF 验证** 而非凭直觉。

---

← [3. 最佳实践](./section-3-性能最佳实践.md) · 下一节 [5. 应用潜力](./section-5-应用潜力.md)
