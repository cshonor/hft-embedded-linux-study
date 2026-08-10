# SLUB vs SLAB 性能对比

> **原文:** [SLUB vs SLAB performance](https://lwn.net/Articles/229160/) (LWN, 2007)
> **对标旧书:** ULK3 Ch8 SLAB 性能分析

---

## 核心观点

LWN 对比测试了 SLUB 和 SLAB 在不同负载下的性能表现。

### 性能对比

| 指标 | SLAB | SLUB | 差异 |
|------|------|------|------|
| 单线程 kmalloc | 基准 | +5-10% | freelist 更快 |
| 多线程 (8 CPU) | 基准 | +15-20% | per-CPU 锁竞争更少 |
| NUMA 跨节点 | 基准 | +30% | partial slab 本地化 |
| 内存开销 | 基准 | -30% | 无 array_cache 元数据 |
| 大对象 (>8KB) | 相似 | 相似 | 都退化到页分配器 |

### SLUB 优势场景

1. **高频小对象分配**：网络栈 sk_buff（~256B）、文件系统 inode（~500B）—— per-CPU freelist 快路径优势最大
2. **NUMA 系统**：partial slab 优先本地节点，减少跨节点分配
3. **内存受限系统**：元数据开销减少 30%

### SLAB 仍有优势的场景

1. **特殊对齐需求**：SLAB 的 `kmem_cache_create()` 支持更细粒度对齐
2. **确定性回收**：SLAB 的 `kmem_cache_shrink()` 行为更可预测

---

## 与旧书差异

| ULK3 讲的 | 现代实现 |
|-----------|---------|
| SLAB 是默认 | SLUB 是默认 (2.6.23+) |
| 无性能对比数据 | SLUB 在多线程/NUMA 场景显著优于 SLAB |

---

## HFT 关联

HFT 的网络包处理路径每秒数百万次 kmalloc/kfree sk_buff。SLUB 的 per-CPU 快路径（~20ns）比 SLAB 的 array_cache（~40ns）快 2 倍。在 10Gbps 网卡满载时，这减少 ~5% CPU 占用。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** SLUB 在 NUMA 系统上比 SLAB 快 30% 的原因是什么？

> SLAB 的 per-node shared array_cache 需要跨 CPU 协调，锁竞争和 cache line bouncing 严重。SLUB 的 per-CPU partial slab 优先从本地节点的 partial 链表补充，跨节点分配只在本地 partial 耗尽时才发生。NUMA 距离导致的延迟（~100-300ns）被最小化。

**Q2:** 为什么 SLUB 的元数据开销比 SLAB 少 30%？

> SLAB 每个 per-CPU array_cache 是一个对象指针数组（如 120 个指针 = 960B/CPU）。加上 per-node 的 3 条 shared/shared_alien 链表头，元数据总量可观。SLUB 的 per-CPU 状态只有一个 freelist 指针 + 一个 partial 链表头（~24B/CPU），空闲对象的 next 指针嵌在对象本身中。

</details>
