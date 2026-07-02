## 1. 本章定位

> **《深入浅出 DPDK》Ch 2 Cache 和内存** — 包处理为何 **卡在内存**

---

### 一、本章讲什么

高速包处理中，**CPU 算力 >> 内存带宽/延迟** — 内存访问常是 **第一瓶颈**。本章从 **硬件体系结构** 讲 DPDK 如何把性能推到极致：

| 主题 | 要点 |
|------|------|
| **多级 Cache** | L1/L2/L3、Cache Line、TLB |
| **预取** | 硬件 vs **软件预取**（`_mm_prefetch`） |
| **一致性** | MESI、伪共享、`__rte_cache_aligned`、per-core |
| **大页** | hugetlbfs、↓ TLB miss |
| **DDIO** | 网卡 ↔ **LLC** 直连 |
| **NUMA** | 本地设备、本地内存、本地核 |

---

### 二、小节导航

| 节 | 主题 |
|----|------|
| [2](./section-2-阶梯式Cache系统.md) | L1/L2/L3、64B Cache Line、TLB |
| [3](./section-3-Cache预取.md) | PREFETCH0、隐藏访存延迟 |
| [4](./section-4-Cache一致性与无锁设计.md) | MESI、对齐、per-core 队列 |
| [5](./section-5-大页Hugepages.md) | 2MB/1GB 页、mmap hugetlbfs |
| [6](./section-6-DDIO与NUMA.md) | Data Direct I/O、就近原则 |

---

### 三、与 Ch1 / mbuf 的关系

```
Ch1 认识 DPDK（轮询、绑核、大页概念）
    ↓
Ch2 Cache 与内存（本章 — 为何这样设计）
    ↓
mbuf / mempool（在正确内存上预分配对象）
    ↓
PMD 收发包
```

→ [Ch1](../chapter-01-dpdk-intro/) · [chapter-02 mbuf](../chapter-02-mbuf与内存池.md)

---

← [Ch 2 导读](../README.md) · 下一节 [2. Cache 层次](./section-2-阶梯式Cache系统.md)
