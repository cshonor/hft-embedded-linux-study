# Ch 7 §2 分配非连续区域 (Allocating)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**
> 源码核验：Linux **v6.6**（`mm/vmalloc.c`）

---

## 本节讲什么

本节回答：**`vmalloc()` 是怎么一步步把「物理上散页」拼成「虚拟上连续」的？**

原书（2.6）已经确立了两阶段思想：先 reserve 虚拟区间、再分配物理页 + 建页表。v6.6 里这条骨架没变，但两个环节都**现代化**了——物理页分配从「逐个 `alloc_page`」升级成「批量 `alloc_pages_bulk_array`」，页表映射从「按 4K 逐页」升级成「可选 PMD 大页」。本节沿完整调用链走一遍。

---

## 1. API 家族

| 函数 | 说明 | 备注 |
|------|------|------|
| `vmalloc(size)` | 通用非连续分配，`GFP_KERNEL` 默认 | `mm/vmalloc.c:3416` |
| `vzalloc(size)` | `vmalloc` + **清零** | 等价 `__vmalloc(size, GFP_KERNEL \| __GFP_ZERO)` |
| `vmalloc_user(size)` | 可映射到用户态（`VM_USERMAP`） | 配 `remap_vmalloc_range` |
| `vmalloc_node(size, node)` | 指定 NUMA 节点 | 物理页优先从该 node 取 |
| `vmalloc_32(size)` | 物理页落在 **32 位可寻址**范围 | 老设备 DMA 约束 |
| `vmalloc_huge(size, gfp)` | 强制尝试大页映射 | `:3435`，仅 `HAVE_ARCH_HUGE_VMALLOC` |
| `__vmalloc(size, gfp)` | 自定义 `gfp_mask` 的底层入口 | `:3397` |
| `vmap(pages, count, flags, prot)` | 把调用者给好的页**映射**成虚拟连续（不分配页） | `:2894` |
| `vmalloc_array(n, size)` / `vcalloc(n, size)` | 数组分配（带溢出检查） | `vmalloc.h:156-159` |

> ⚠️ **版本断崖**：原书提到的 `vmalloc_dma()` 在 v6.6 **已删除**。现代做法是用 `__vmalloc_node_range()` 传 `GFP_DMA`/`GFP_DMA32` 约束页分配器（或直接用 DMA API，见 09-DMA Ch12）。不要在新代码里找 `vmalloc_dma`。

---

## 2. 两阶段分配总览

```
vmalloc(size)
  └─ __vmalloc(size, GFP_KERNEL)                     :3397
       └─ __vmalloc_node(size, 1, gfp, NUMA_NO_NODE, caller)  :3382
            └─ __vmalloc_node_range(...)             :3235  ★ 核心
                 │
                 ├─ 阶段① __get_vm_area_node()       :2570
                 │        reserve 一段虚拟区间
                 │        (alloc_vmap_area 从 free 树找洞 :1582)
                 │        新建 vm_struct + vmap_area，挂 busy 树
                 │
                 └─ 阶段② __vmalloc_area_node()      :3101
                          ├─ vm_area_alloc_pages()   :2992  分配散页数组
                          │      (批量 alloc_pages_bulk_array)
                          └─ vmap_pages_range()      :628   建页表映射
                                 (walk pgd→pud→pmd→pte 填表)
```

**关键：先抢地址、再填物理。** 阶段①和阶段②分离，好处是：物理页分配失败时，只需把已 reserve 的虚拟区间归还（`vfree`/`free_vm_area`），不会留下「半映射」的脏状态。

---

## 3. 阶段①：`__get_vm_area_node` reserve 虚拟区间

```c
/* mm/vmalloc.c:2570 */
static struct vm_struct *__get_vm_area_node(unsigned long size, ...)
{
    area = kzalloc_node(sizeof(*area), ...);      /* :2588 先分配 vm_struct */
    if (!(flags & VM_NO_GUARD))
        size += PAGE_SIZE;                        /* :2592 默认 +1 页 guard */

    va = alloc_vmap_area(size, align, start, end, ...);  /* :2595 抢地址 */
    if (IS_ERR(va)) { kfree(area); return NULL; }

    setup_vmalloc_vm(area, va, flags, caller);    /* :2601 关联 vm_struct ↔ vmap_area */
    ...
    return area;
}
```

`alloc_vmap_area`（`:1582`）是**地址分配的真正引擎**：

```c
retry:
    addr = __alloc_vmap_area(&free_vmap_area_root, &free_vmap_area_list,
                             size, align, vstart, vend);   /* :1615 红黑树找洞 */
    ...
    va->va_start = addr;                          /* :1628 */
    va->va_end   = addr + size;
    va->vm       = NULL;                          /* :1630 尚未关联 */

    insert_vmap_area(va, &vmap_area_root, &vmap_area_list);  /* :1634 挂 busy 树 */
    ...
overflow:
    reclaim_and_purge_vmap_areas();               /* :1651 空间不足先回收懒释放 */
```

**要点**：
- 用 **free 树** + `subtree_max_size` 剪枝，O(log n) 找到满足 `size` + `align` 的洞（`__alloc_vmap_area`，`:1489`）。
- 地址耗尽时走 `overflow` 分支：先 `reclaim_and_purge_vmap_areas()` 把攒着的懒释放区间真正回收，再 `goto retry` 重试。
- 找到后 `insert_vmap_area` 把节点从 free 树挪到 **busy 树**。

---

## 4. 阶段②：`__vmalloc_area_node` 分配物理页 + 建页表

```c
/* mm/vmalloc.c:3101 */
static void *__vmalloc_area_node(struct vm_struct *area, gfp_t gfp_mask, ...)
{
    /* 1. 先分配 pages 指针数组本身 */
    array_size = nr_small_pages * sizeof(struct page *);
    if (array_size > PAGE_SIZE)
        area->pages = __vmalloc_node(array_size, ...);   /* :3122 数组大，递归 vmalloc */
    else
        area->pages = kmalloc_node(array_size, ...);     /* :3125 数组小，直接 kmalloc */

    /* 2. 批量分配物理散页 */
    area->nr_pages = vm_area_alloc_pages(gfp_mask | __GFP_NOWARN,
                        node, page_order, nr_small_pages, area->pages);  /* :3139 */

    /* 3. 建立页表，映射到 vmalloc 区 */
    ret = vmap_pages_range(addr, addr + size, prot, area->pages, page_shift);  /* :3182 */

    return area->addr;
fail:
    vfree(area->addr);                            /* :3203 失败回滚 */
    return NULL;
}
```

三个步骤，两个现代升级点：

### 4.1 批量页分配：`vm_area_alloc_pages`（`:2992`）

原书是「walk 页表到 PTE 时**逐个** `alloc_page`」；v6.6 用**批量分配器**：

```c
if (!order) {                                     /* :3007 order-0 走批量 */
    while (nr_allocated < nr_pages) {
        nr_pages_request = min(100U, nr_pages - nr_allocated);  /* :3020 每次最多 100 页 */
        nr = alloc_pages_bulk_array_node(bulk_gfp, nid,
                                         nr_pages_request, pages + nr_allocated); /* :3033 */
        nr_allocated += nr;
        cond_resched();                           /* :3038 */
        if (nr != nr_pages_request)
            break;                                /* :3044 没拿满，退化为单页分配 */
    }
}
```

| 要点 | 说明 |
|------|------|
| **批量 100 页/次** | `alloc_pages_bulk_array` 一次拿一串物理页，比逐个 `alloc_page` **减少 Buddy 加锁/关闭抢占的次数**（上限硬编码 100，防长时间 preemption off） |
| **NUMA 策略** | `nid == NUMA_NO_NODE` 时走 `alloc_pages_bulk_array_mempolicy`（`:3028`），尊重进程 mempolicy（如 interleave），避免「全落到最近节点」 |
| **失败回退** | 批量没拿满 → 退到 `alloc_pages` 单页路径（`:3058` 循环），更宽松 |
| **大页拆分** | `order > 0` 时 `split_page(page, order)`（`:3084`），把高阶页拆成 order-0 逐页填入数组——**保证 `vm_struct->pages[]` 里永远是 4K 页指针**，上层 API 不感知大页 |

### 4.2 大页映射：huge vmalloc（`vmap_allow_huge`）

`__vmalloc_node_range`（`:3235`）开头有段 huge 判断：

```c
if (vmap_allow_huge && (vm_flags & VM_ALLOW_HUGE_VMAP)) {   /* :3257 */
    if (arch_vmap_pmd_supported(prot) && size_per_node >= PMD_SIZE)
        shift = PMD_SHIFT;                          /* :3271 用 2MB 大页映射 */
    else
        shift = arch_vmap_pte_supported_shift(size_per_node);
    align = max(real_align, 1UL << shift);
    size  = ALIGN(real_size, 1UL << shift);
}
```

- `vmap_allow_huge` 默认 true，可用内核参数 `nohugevmalloc` 关闭（`:70`）。
- 满足条件时 `shift` 从 `PAGE_SHIFT` 升到 `PMD_SHIFT`，`vmap_pages_range` 会**直接映射 PMD 级大页**，而非一页页 PTE。
- **失败回退**（`:3352`）：若大页路径失败，`shift = PAGE_SHIFT` 后 `goto again`，用普通 4K 页重试——保证「能大则大，大不了退 4K」。

---

## 5. 页表同步：`init_mm` vs 当前进程

**关键**：`vmap_pages_range` 更新的是**内核参考页表 `init_mm->pgd`**，不是每个进程自己的页表。

| 何时 | 发生什么 |
|------|----------|
| **vmalloc 返回后** | `init_mm` 里已有完整映射；**其他进程/内核线程的页表可能还没有这一项** |
| **首次访问该 VA** | 内核态缺页 → 判断地址落在 vmalloc 区 → 从 `init_mm` 把对应 PTE **复制**到当前页表视图 |

**直觉**：vmalloc 区是**内核全局共享**资源，所以只维护一份「权威」页表（`init_mm`），其余进程**懒同步**。这与用户态 `mmap` 的 demand fault 同思路，但走**内核专属 fault 分支**（`vmalloc_fault`），不是 `do_anonymous_page`。

---

## 6. HFT / 嵌入式关联

| 场景 | 关联 |
|------|------|
| **批量分配的意义** | HFT 里「减少系统调用/锁开销」是核心课题；内核用 `alloc_pages_bulk_array` 把 N 次 Buddy 锁合并成 1 次，**同类优化思路** |
| **大页减少 TLB miss** | huge vmalloc 用 PMD 覆盖 2MB，让 vmalloc 区的**页表遍历层数少一级、TLB 覆盖更宽**——大页对性能的意义与用户态 THP/hugetlb 完全同源 |
| **两阶段的原子性** | 「先 reserve 再填页」保证失败可整体回滚——HFT 里资源分配也应遵循「先占位、后填充、失败回滚」的模式 |
| **懒同步的代价** | vmalloc 区**首访会触发内核 fault**，是隐蔽的延迟源；关键路径应避免「分配后立即大块读写」的冷启动抖动 |

---

## 7. 衔接

- 下节 [§3 释放非连续区域](./section-3-释放非连续区域.md)：`vfree` 怎么把这一整套对称拆掉
- 页表遍历：[Ch3 §2 遍历与使用页表](../../chapter-03-page-table-management/notes/section-2-遍历与使用页表.md)
- 页分配器：[Ch6 §2 页面分配](../../chapter-06-physical-page-allocation/notes/section-2-页面分配.md)（`alloc_pages` 的源头）

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：v6.6 的 vmalloc 两阶段和原书 2.6 相比，哪些环节现代化了？**
A：骨架没变（先 reserve VA 再填物理页），但两处升级：① 物理页分配从「逐个 `alloc_page`」变成 `alloc_pages_bulk_array`（每次最多 100 页批量，减少锁/关抢占次数）；② 页表映射从「仅 4K 逐页」变成可选 PMD 大页（`VM_ALLOW_HUGE_VMAP`，失败自动回退 4K）。另外地址索引从链表升级为红黑树（§1）。

**Q2：`vm_area_alloc_pages` 为什么硬编码「每次最多 100 页」？**
A：批量分配器 `alloc_pages_bulk_array` 在拿一串页时会**长时间关闭抢占**（`preemption off`）。若一次请求上千页，会让某个 CPU 长时间无法被抢占，影响调度延迟。100 页是「批量收益 vs 抢占延迟」的折中上限，注释里明确写了这一点（`:3015-3019`）。

**Q3：huge vmalloc 失败后怎么处理？会导致分配失败吗？**
A：不会直接失败。`__vmalloc_node_range` 的 `fail:` 标签（`:3352`）判断「若 `shift > PAGE_SHIFT`」就 `shift = PAGE_SHIFT` 后 `goto again`，用普通 4K 页重试。即「能大则大，大不了退 4K」——大页只是优化，不是必需。

**Q4：`area->pages` 数组本身是怎么分配的？为什么可能递归 vmalloc？**
A：数组大小 = `nr_small_pages * sizeof(struct page *)`。若它 ≤ 1 页（`PAGE_SIZE`），用 `kmalloc_node`；若 >1 页（比如几百 MB 的 vmalloc，页指针数组本身就好几 MB），用 `__vmalloc_node` **递归分配**。注释 `:3120` 强调「这个递归严格有界」——数组大小远小于分配大小，不会无限套娃。

**Q5：`vmalloc()` 返回后，为什么别的 CPU 可能「看不到」这个映射？**
A：因为映射只写进了 `init_mm->pgd`（内核参考页表），其他进程页表是**懒同步**的。它们首次访问该地址时触发内核 vmalloc fault，才把 `init_mm` 里的 PTE 复制过来。所以 vmalloc 区存在「首访延迟」，且需要内核在 fault 路径里识别「这是 vmalloc 区」而非普通缺页。

</details>
