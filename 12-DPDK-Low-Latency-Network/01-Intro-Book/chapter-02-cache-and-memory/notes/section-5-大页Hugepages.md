## 5. 突破 TLB 瓶颈：大页 (Hugepages)

> **4KB 小页** → 页表项爆炸 → **TLB miss** 频发

---

### 一、问题

大内存应用（DPDK mbuf 池、大 ring）若用 **4KB 页**：

- 需要 **海量 PTE**  
- **TLB 装不下** → 频繁 miss → 遍历 **多级页表**  

→ TLB：[section-2](./section-2-阶梯式Cache系统.md) · [ULK Ch2](../../../04-Understanding-Linux-Kernel/chapter-02-memory-addressing/)

---

### 二、DPDK 的大页

| 页大小 | 典型用途 |
|--------|----------|
| **2MB** | 常用 hugepage |
| **1GB** | 更大池、减 TLB 压力 |

**效果：** 同样物理内存 → **页表项数量 ÷512（2MB）或更多** → TLB **命中率高**。

---

### 三、如何使用

1. 系统配置 **hugetlbfs** / `sysctl` 预留大页  
2. **`rte_eal_init()`** 申请并映射大页（EAL 内部 **`mmap`** hugetlbfs）  
3. **mbuf / mempool / ring** 从大页堆分配  

→ EAL：[Ch1 HelloWorld](../chapter-01-dpdk-intro/notes/section-6-编程实例入门.md) · mbuf：[chapter-02-mbuf](../chapter-02-mbuf与内存池.md)

**HFT 检查清单：** `grep Huge /proc/meminfo` · NUMA 节点上 **分别** 预留大页。

---

← [4. 一致性](./section-4-Cache一致性与无锁设计.md) · 下一节 [6. DDIO/NUMA](./section-6-DDIO与NUMA.md)
