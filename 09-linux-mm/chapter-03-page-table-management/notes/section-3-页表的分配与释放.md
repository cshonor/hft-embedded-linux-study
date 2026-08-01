# Ch 3 §3 页表的分配与释放 (Quicklists)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**

### 3. 页表的分配与释放 (Quicklists)

分配 **物理页** 作页表本身（PGD/PMD/PTE 表占用的页）**昂贵** — 常需 **关中断 / 拿锁**。

**2.4/2.6 快速缓存 (Quicklists)：**  per-CPU 或全局 **LIFO 链表** 缓存刚释放的页表页：

| Quicklist | 缓存对象 |
|-----------|----------|
| **`pgd_quicklist`** | PGD 级表页 |
| **`pmd_quicklist`** | PMD 级表页 |
| **`pte_quicklist`** | PTE 级表页 |

**LIFO** → 刚释放的页更可能 **仍在 cache 热**，加速 **fork / munmap / 缺页** 路径。

> **现代内核：** quicklist 概念演化为 **专用 slab / kmem_cache**（如 `pgtable_cache`）；**思想不变** — **不要每次 walk 到 buddy 分配器**。

---
