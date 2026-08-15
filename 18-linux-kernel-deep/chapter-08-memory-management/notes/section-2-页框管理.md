## 2. 页框管理 (Page Frame Management)

---

### 一、页描述符 `struct page`

内核用 **`struct page`** 跟踪 **每一个物理页框**：

- 是否空闲  
- 引用计数  
- 所属 zone、node 等  

这是物理内存管理的 **基本单元**。

---

### 二、NUMA 节点 vs UMA

| 架构 | 组织方式 |
|------|----------|
| **NUMA** | 物理内存分多个 **节点** `pg_data_t`；CPU 访问本地/远程节点延迟不同 |
| **UMA（典型 80x86）** | 所有 RAM 逻辑上 **单一节点** |

HFT 绑核 + **NUMA 本地内存** 分配 → 降低跨节点访问延迟。

---

### 三、内存管理区 (Memory Zones)

因硬件限制，节点再划分为 **Zone**：

| Zone | 用途 |
|------|------|
| **`ZONE_DMA`** | **< 16MB** 页框 — 旧 ISA DMA 只能寻址低端内存 |
| **`ZONE_NORMAL`** | 内核可 **直接线性映射** 的常规页 |
| **`ZONE_HIGHMEM`** | 无法直接映射进内核线性地址空间（32 位第四 GB 限制）的 **高端内存** |

访问高端内存：

- **永久内核映射** — `kmap`  
- **临时内核映射** — `kmap_atomic`（中断上下文等）

→ 3G/1G 划分：[Ch 2](../../chapter-02-memory-addressing/notes/section-6-内存布局与TLB.md)

---

### 四、伙伴系统 (Buddy System)

**问题：** **外碎片** — 物理内存有空闲页，但没有足够大的 **连续** 块。

**做法：** 11 条链表，分别管理大小为 **1, 2, 4, …, 1024** 个连续页框的块。

| 操作 | 行为 |
|------|------|
| **分配** | 找 ≥ 请求大小的最小块；过大则 **拆分** |
| **释放** | 尝试与 **物理相邻、同大小** 的 **伙伴块合并** |

满足大块连续分配，摊还效率好。

---

### 五、每 CPU 页框高速缓存

- 每个 CPU 维护 **热 / 冷** 页框缓存  
- **单页** 分配走本地缓存 → 减少 **全局 spinlock** 竞争  

→ 与 [Ch 5](../../chapter-05-kernel-synchronization/notes/section-3-基础同步原语.md) per-CPU 变量思想一致。

### 常见陷阱

1. 把 ULK 讲的 buddy system 当不变的事实——6.x 的 buddy 仍存在但 API 大改（`folio` 系列），且支持 MGLRU 回收
2. 混淆 `alloc_pages()` 和 `__get_free_pages()`——前者返回 `struct page*`，后者返回虚拟地址
3. 以为 `free_pages()` 和 `put_page()` 等价——`free_pages()` 是 buddy API，`put_page()` 是引用计数减一

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** buddy system 的核心思想和优缺点？

<details><summary>答案</summary>

核心：将空闲页按 2^n 大小分块（order 0 = 4KB, order 1 = 8KB, ..., order 10 = 4MB）。分配时找最小满足的块，大了就分裂。释放时检查 buddy（相邻同大小块）是否空闲，空闲则合并。优点：① 快速（O(log N)）。② 避免外碎片（大块需求能满足）。缺点：① 内碎片（请求 5KB 分配 8KB）。② 不适合小对象（用 slab/slob 补充）。

</details>

**Q2.** 现代内核 `struct folio` 相比 `struct page` 解决了什么问题？

<details><summary>答案</summary>

`struct page` 每个 4KB 页一个，64GB 内存有 16M 个 `struct page`（每个 64 字节 = 1GB 开销）。Folio 将多个连续页作为一个管理单元，减少 `struct page` 数量和遍历开销。API：`folio_alloc()` / `folio_free()` / `folio_address()`。对文件系统/页缓存收益最大（一个 folio 管理多页，减少锁竞争和树操作）。ULK 时代没有 folio 概念。

</details>

**Q3.** HFT 如何利用大页（huge page）减少 TLB miss？

<details><summary>答案</summary>

① `mmap(MAP_HUGETLB, ...)`：匿名大页（2MB/1GB），TLB 一个条目覆盖 512/262144 个 4KB 页。② `madvise(MADV_HUGEPAGE)`：让内核对普通匿名映射尝试合并为透明大页（THP）。③ `/sys/kernel/mm/transparent_hugepage/enabled=always`。注意：THP 可能在后台触发 `khugepaged` 整理内存，导致延迟尖峰。HFT 优先用显式 `MAP_HUGETLB`。

</details>

</details>

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. Slab 分配器](./section-3-Slab分配器.md)
> ↔ [LKD Ch12 §12.2 页](../../../05-linux-kernel/chapter-12-memory-management/notes/section-12.2-页.md)
