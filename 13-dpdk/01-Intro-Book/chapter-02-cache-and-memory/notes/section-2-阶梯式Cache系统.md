## 2. 阶梯式 Cache 系统

> 弥补 **CPU 与内存** 之间的速度鸿沟

---

### 一、L1 / L2 / L3

| 级别 | 延迟 (cycle) | 延迟 (ns @ 3GHz) | 容量 (典型) | 特点 |
|------|:---:|:---:|------|------|
| **L1 D** | 4 | ~1.3 | 32-48 KB | **最快、最小** — 分 **指令 Cache (I)** 与 **数据 Cache (D)**；**每核独占** |
| **L2** | 12-14 | ~4 | 256 KB-1.25 MB | 稍慢、容量更大 — 通常 **每核独占** |
| **L3 (LLC)** | 30-40 | ~10-13 | 16-64 MB | **最大** — **所有核心共享** Last Level Cache |
| **DRAM** | 150-300 | ~50-100 | GB 级 | **极慢** — cache miss 的代价 |

包处理热路径：**命中 L1/L2** = 纳秒级；**落内存** = 数百 cycle。HFT tick-to-trade 每微秒都珍贵 — 一次 L3 miss 可能吃掉 **3% 的延迟预算**（假设 100ns budget）。

 [02-CSAPP Ch6](../../../../02-computer-systems/chapter-06-memory-hierarchy/) · [15-computer-architecture Ch2](../../../../15-computer-architecture/chapter-02-memory-hierarchy-design/)

---

### 二、Cache Line（缓存行）

- CPU 与内存交换的 **基本单位** — x86 通常 **64 字节**
- 读 1 字节也可能 **整行载入** — 结构体布局影响 **false sharing**（→ [section-4](./section-4-Cache一致性与无锁设计.md)）
- 相邻数据 **免费** 命中 — 数组顺序访问远快于指针追逐

**DPDK 中的 cache line 对齐：**

```c
#include <rte_common.h>

/* 关键结构体按 cache line 对齐 */
struct port_statistics {
    uint64_t rx_packets    __rte_cache_aligned;  /* 独占 cache line */
    uint64_t tx_packets    __rte_cache_aligned;
    uint64_t rx_dropped    __rte_cache_aligned;
} __rte_cache_aligned;

/* RTE_CACHE_LINE_SIZE = 64 (x86) 或 128 (部分 ARM) */
```

**HFT：** 热数据结构 **≤ 64B 或按行对齐拆分**；避免无关字段与高频计数器 **同行**。订单簿的 price level 结构体尤其需要注意 — 如果 bid 和 ask 落在同一 cache line，更新 bid 会导致 ask 所在行 invalidate。

---

### 三、TLB Cache

| 作用 | 缓存 **虚拟地址 → 物理地址** 的页表项 |
|------|----------------------------------------|
| **L1 dTLB** | ~64-72 条目（4KB 页），~12 cycle |
| **L2 TLB** | ~1024-2048 条目（共享），~20-30 cycle |
| **TLB miss** | CPU 需 **遍历多级页表** 访存 — 4 级页表 = 4 次内存访问，**极贵** |

**x86-64 四级页表（4KB 页）：**

```
虚拟地址 (48 bit)
┌─────────┬─────────┬─────────┬─────────┬──────────┐
│ PML4(9) │ PDPT(9) │ PD(9)   │ PT(9)   │ Offset(12)│
└────┬────┴────┬────┴────┬────┴────┬────┴──────────┘
     │         │         │         │
  CR3→PML4 → PDPT → PD → PT → 物理页帧
     ↓         ↓         ↓         ↓
   L1 miss  L1 miss  L1 miss  L1 miss  = 4× ~100ns
```

一次 TLB miss 最坏情况需要 **4 次内存访问** — 这就是大页存在的理由（→ [section-5](./section-5-大页Hugepages.md)）。

 [ULK Ch2 页表](../../../../16-linux-kernel-deep/chapter-02-memory-addressing/) · 大页缓解：[section-5](./section-5-大页Hugepages.md)

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. 预取](./section-3-Cache预取.md)
