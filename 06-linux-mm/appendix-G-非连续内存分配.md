# 附录 G 非连续内存分配 · Noncontiguous Memory Allocation

> **Code Commentary** · Mel Gorman · **跳过** · 源码核验：Linux v6.6

概念总览 → [./chapter-07-noncontiguous-memory-allocation/](./chapter-07-noncontiguous-memory-allocation/)

---

## 本节走读什么

正文 Ch7 讲了「vmalloc 两阶段分配、双结构双索引、三棵树释放」。本附录走读 **`mm/vmalloc.c`（118KB）** 的代码组织——重点是「**先 reserve 虚址，再分配物理页**」的两阶段结构，以及释放时的「懒 TLB flush」。

---

## 1. 两阶段分配全景（mm/vmalloc.c）

```
__vmalloc_node_range(size, align, start, end, gfp, ...)   // vmalloc.c:3235
        │
        ├─ ① __get_vm_area_node(...)                       // :2570  reserve 虚拟地址
        │       └─ alloc_vmap_area(...)                    // :1582  从 free 树找洞
        │               └─ 红黑树 + 双链表双索引（Ch7 §1）
        │
        └─ ② __vmalloc_area_node(...)                      // :3101  分配物理页 + 建映射
                ├─ vm_area_alloc_pages(...)                // :2992  批量分配页
                │       └─ alloc_pages_bulk_array_node(...) // 每次最多 100 页（:3020）
                └─ vmap_pages_range(...)                   // :628   逐页建 PTE 映射
```

**走读要点**：两阶段的意义是**失败可回滚**——先 reserve 虚址（失败就直接返回），再分配物理页（失败则释放虚址）。不会出现「物理页分配了一半、虚址却没了」的中间态。这是 vmalloc 比 kmalloc 慢、但能「虚拟连续」的核心代价。

## 2. `alloc_vmap_area`：从 free 树找洞（:1582）

`struct vmap_area`（vmalloc.h:63）的 `union` 复用是精髓：

```
union {
    unsigned long subtree_max_size;   // 在 free 树里：该子树最大空闲洞
    struct vm_struct *vm;             // 在 busy 树里：指向区域描述符
};
```

- **free 树**按地址排序，`subtree_max_size` 加速「找第一个 ≥ size 的空洞」（从 O(n) 到 O(log n)）。
- **busy 树**按地址排序，`vm` 快速反查「这段地址属于哪个 vm_struct」。

## 3. 批量页分配：`vm_area_alloc_pages`（:2992）

```c
vm_area_alloc_pages(...)
        │  order>0 时 split_page 拆成 order-0，保证 pages[] 全 4K
        ▼
alloc_pages_bulk_array_node(...)        // 每次最多 100 页（:3020）
        │  减少 Buddy 锁 / 关抢占次数
```

**走读要点**：vmalloc 需要「物理不连续但各自 4K 对齐」的页，所以**批量**调 `alloc_pages_bulk_array_node`（一次 100 页）而不是逐页 `alloc_page`——后者每页都要抢一次 Buddy 锁，批量版本摊薄锁开销。

## 4. 释放路径 + 懒 TLB flush（:2807）

```
vfree(addr)                             // vmalloc.c:2807
        ├─ 中断上下文 → vfree_atomic    // :2773  lockless llist_add 入队 + workqueue 兜底
        └─ 进程上下文 ↓
remove_vm_area(addr)                    // :2684  从 busy 树摘除 + unmap
        │
free_unmap_vmap_area(va)                // :1847
        │
free_vmap_area_noflush(va)              // :1817  先不刷 TLB，攒进 purge 树
        │  攒到 lazy_max_pages() 才统一 flush_tlb_kernel_range
        ▼
lazy_max_pages()                        // :1700  = fls(CPU 数) × 8192 页
```

**走读要点**：释放后**不立即刷 TLB**，而是把已 unmap 的区间攒进 **purge 树**（第三棵树），超过 `lazy_max_pages()` 阈值才统一刷。这是「用少量陈旧 TLB 换大幅减少 flush 次数」的权衡——Ch7 §3 的三棵树模型（free/busy/purge）就在这里落地。

---

## 与正文对应

| 附录内容 | 正文落点 |
|----------|----------|
| 两阶段分配 | Ch7 §2（reserve VA → alloc pages） |
| `vmap_area` union 复用 | Ch7 §1（双索引 + union） |
| 批量 100 页/次 | Ch7 §2（`alloc_pages_bulk_array`） |
| 三棵树 + 懒 flush | Ch7 §3（free/busy/purge + `lazy_max_pages`） |

---

## HFT / 嵌入式关联

| 手段 | 落点 |
|------|------|
| 别用 vmalloc 做热路径 | vmalloc 每次都要改内核页表 + 可能 TLB flush，比 kmalloc 慢几个量级 |
| `kvmalloc` 智能回落 | 大块且不要求物理连续时，`kvmalloc` 先试 kmalloc 再回 vmalloc（Ch7 §4） |
| 懒 flush 的代价 | 陈旧 TLB 会让「刚释放又立刻重映射」的地址短暂不一致——内核态高频重映射场景要留意 |

---

## 相关章节

- 上一章：[appendix-F-物理页分配.md](./appendix-F-物理页分配.md)
- 下一章：[appendix-H-Slab分配器.md](./appendix-H-Slab分配器.md)

---

<details>
<summary>自测 5 问（点开看答案）</summary>

**Q1：vmalloc 的两阶段分配是哪两步？为什么分两阶段？**

① `__get_vm_area_node` reserve 虚拟地址；② `__vmalloc_area_node` 分配物理页 + 建映射。分两阶段是为了**失败可整体回滚**，不出现「物理页分了一半、虚址没了」的中间态。

**Q2：`struct vmap_area` 的 union 复用怎么用？**

free 树里 `subtree_max_size` 存「子树最大空闲洞」加速找洞；busy 树里 `vm` 指向区域描述符快速反查。

**Q3：`vm_area_alloc_pages` 为什么批量分配、每次最多 100 页？**

减少 Buddy 锁/关抢占次数（`alloc_pages_bulk_array_node`，:3020）；order>0 时先 `split_page` 拆成 order-0 保证 pages[] 全 4K。

**Q4：`vfree` 在中断上下文怎么处理？**

转 `vfree_atomic`（:2773），用 lockless `llist_add` 入队 + workqueue 兜底，不在中断里做可能睡眠的 unmap。

**Q5：懒 TLB flush 的阈值 `lazy_max_pages` 怎么算？**

`fls(CPU 数) × 8192 页`（:1700）——CPU 越多、攒的越多才刷一次，摊薄 flush 开销。

</details>
