## 1.6 存储设备形成层次结构

### 存储层次（金字塔）

从快到慢、从贵到便宜、从小到大的 **层次结构 (memory hierarchy)**：

```
寄存器  →  L1/L2/L3 cache  →  主存 DRAM  →  本地磁盘 (SSD/HDD)  →  远程存储/磁带
  ↑ 更快、更贵、更小                                                          ↓ 更慢、更便宜、更大
```

**设计思想：** 上层是下层的 cache — 磁盘缓存页在内存，内存缓存行在 L3，…

| 层级 | 典型用途 | HFT |
|------|----------|-----|
| 寄存器/cache | 热路径计算、order book | 算法与布局主战场 |
| DRAM | 工作集、mmap 行情缓冲 | NUMA 绑定、THP/大页策略 |
| SSD | 日志、回放、配置 | 顺序写、避免热路径 sync |
| 网络存储 | 备份、研究数据 | **不在 tick 路径** |

### 与 1.5 的关系

- **1.5** 强调 CPU–cache–DRAM 路径上的延迟
- **1.6** 把 **磁盘 I/O** 纳入同一图景 — I/O 慢几个数量级，必须 **异步 / 批量 / 摊销**

**HFT 实践：**

- 热路径：**零磁盘**（内存 ring、预分配）
- 冷路径：批量落盘、单独线程 + 专用核
- 基准：区分 **memory-bound vs I/O-bound**（→ [13-Systems-Performance Ch 12 基准测试](../../../13-Systems-Performance-2nd/chapter-12-benchmarking/)）

→ [Ch 6](../../chapter-06-memory-hierarchy/) · [Ch 10 系统级 I/O](../../chapter-10-system-io/)

---

← [本章导读](../README.md)
