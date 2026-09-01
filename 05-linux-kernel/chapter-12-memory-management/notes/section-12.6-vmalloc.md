## ⑥ vmalloc() · 虚拟连续分配

**虚拟连续、物理可不连续** — 用 **改页表** 把零散物理页拼成 **连续 VA 区间**，适合 **大块**、**非 DMA**、**非性能关键** 的内核分配。

```
kmalloc 路径:
  [ 物理连续 4KB×N ] ──direct map──► 连续 VA  （快，页表早已建好）

vmalloc 路径:
  物理页 A   物理页 C   物理页 F  （分散，各自 order-0 从 buddy 取）
     │          │          │
     └──── 改页表拼接 ──► [ VA: v .. v+size )  连续，但 PTE 逐页新填
```

#### 保证与代价

| 保证 | 不保证 |
|------|--------|
| **内核 VA 连续** | **物理连续** |
| 可分配 **很大** 区域（仅受 VA 空间限制） | **低延迟** |
| | **适合 DMA** |
| | **可安全在原子上下文分配** |

| 代价 | 原因 |
|------|------|
| **比 kmalloc 慢** | 需 **逐页建页表项**（`vmap_pages_range`），不是简单的 direct-map 偏移 |
| **TLB 压力** | 每页一项 — 4KB 粒度，**miss 多**；释放还要 flush 内核 TLB |
| **更碎片化物理** | 从 buddy **逐页**凑，拿不到高阶块 |

> **LKD 只说了"要不要用"，没说"为什么慢"。** 慢在三件互不相干的事上：
> ① **找 VA 空隙**（红黑树，见下节）；② **逐页建 PTE**（页表层数 × 页数）；
> ③ **页表同步**（x86_64 上其他进程的顶层页表靠缺页异常按需回抄，见第 7 节）。
> 这三项都不在 `kmalloc` 的路径上 —— `kmalloc` 返回的是 **早已映射好的 direct map 地址**，只差 slab 记账。

---

#### 【v6.6 实证】三层数据结构

vmalloc 的"元数据"和"地址区间"是**分开管理的两个对象**，这点 LKD 没讲：

```c
/* include/linux/vmalloc.h:49 —— 描述符，回答"这块是什么" */
struct vm_struct {
	struct vm_struct	*next;
	void			*addr;
	unsigned long		size;
	unsigned long		flags;
	struct page		**pages;	/* 物理页指针数组 */
#ifdef CONFIG_HAVE_ARCH_HUGE_VMALLOC
	unsigned int		page_order;	/* 用大页时记录阶数 */
#endif
	unsigned int		nr_pages;
	phys_addr_t		phys_addr;
	const void		*caller;	/* 分配点 —— /proc/vmallocinfo 的来源 */
};
```

```c
/* include/linux/vmalloc.h:63 —— 区间，回答"这块在哪" */
struct vmap_area {
	unsigned long va_start;
	unsigned long va_end;

	struct rb_node   rb_node;   /* 按地址排序的红黑树节点 */
	struct list_head list;      /* 按地址排序的双向链表节点 */

	/*
	 * 这两个变量可以共用内存，因为一个 vmap_area 只能处于两种状态之一：
	 *    1) 在 free 树里（根是 free_vmap_area_root）
	 *    2) 或在 busy 树里（根是 vmap_area_root）
	 */
	union {
		unsigned long subtree_max_size; /* 在 free 树中：子树最大空闲块 */
		struct vm_struct *vm;           /* 在 busy 树中：指向描述符 */
	};
	unsigned long flags;
};
```

> **注意那个 `union`**：同一块内存，在空闲树上存"我子树里最大能给你多少"，在忙树上存"我属于哪个 vm_struct"。
> 因为一个区间不可能同时又空闲又忙碌，内核把两个字段**叠在一起**省 8 字节。
> 这是内核里典型的"用生命周期互斥换内存"手法 —— 和 [19.4](../../chapter-19-portability/notes/section-19.4-数据对齐和结构体填充.md) 的结构体布局优化同源。

**为什么要两套索引（树 + 链表）？**

```
free_vmap_area_root  (红黑树)        free_vmap_area_list  (双向链表)
   —— 查找 O(log n)                    —— 合并 O(1)

   [va 1000-2000]                    A ⇄ B ⇄ C ⇄ D
      ╱        ╲                     ↑ 插入/删除要动邻居
  [100-500]  [3000-4000]             树里找邻居是 O(log n)，
                                     链表里 prev/next 是 O(1)
```

`mm/vmalloc.c:750` 的注释原文：

```c
/* mm/vmalloc.c:750 */
/*
 * This linked list is used in pair with free_vmap_area_root.
 * It gives O(1) access to prev/next to perform fast coalescing.
 */
static LIST_HEAD(free_vmap_area_list);
```

#### VA 空隙查找：augmented rbtree

普通红黑树只能回答"有没有"，不能高效回答"**哪块够大**"。v6.6 的空闲树是 **增强红黑树**：

```c
/* mm/vmalloc.c:756 */
/*
 * This augment red-black tree represents the free vmap space.
 * All vmap_area objects in this tree are sorted by va->va_start
 * address. It is used for allocation and merging when a vmap
 * object is released.
 *
 * Each vmap_area node contains a maximum available free block
 * of its sub-tree, right or left. Therefore it is possible to
 * find a lowest match of free area.
 */
static struct rb_root free_vmap_area_root = RB_ROOT;
```

```c
/* mm/vmalloc.c:790 */
RB_DECLARE_CALLBACKS_MAX(static, free_vmap_area_rb_augment_cb,
	struct vmap_area, rb_node, unsigned long, subtree_max_size, va_size)
```

```
每个节点额外维护 subtree_max_size = max(自身大小, 左子树最大, 右子树最大)

要 1MB：从根往下走，若左子树的 max < 1MB 就根本不进左子树
        → 一次下降就能定位"最低地址且够大"的空隙，O(log n)
```

> **版本断崖（实测）**：这个结构 **不是一直有的**。
> 我抓了五个版本的 `mm/vmalloc.c` 数 `subtree_max_size` 出现次数：
>
> | 版本 | `subtree_max_size` 次数 | `free_vmap_area_root` | 找空隙复杂度 |
> |------|------------------------|----------------------|--------------|
> | v4.19 | 0 | 无 | **O(n)** 沿树线性扫 |
> | v4.20 | 0 | 无 | O(n) |
> | v5.0 | 0 | 无 | O(n) |
> | v5.1 | 0 | 无 | O(n) |
> | **v5.2** | **23** | **有** | **O(log n)** |
> | v5.10 | 19 | 有 | O(log n) |
> | v6.6 | — | 有 | O(log n) |
>
> **即 v5.2 是分界线**（commit 系列把 free 与 busy 拆成两棵树、引入增强回调）。
> 老资料里"vmalloc 分配是线性扫描、区域多时慢"的说法，在 v5.2 之后**已不成立**。
> 同一批改动还带来 `ne_fit_preload_node`（`mm/vmalloc.c:773`）：
> per-CPU 预载一个 `vmap_area` 对象，专门服务于"正好劈开一个空闲块"的 no-edge 分裂情形，
> 目的写在注释里 —— *"get rid of allocations from the atomic context, thus to use more permissive allocation masks"*。

---

#### API 全表（v6.6 行号锚点）

| API | 文件:行 | 说明 |
|-----|---------|------|
| **`vmalloc(size)`** | `mm/vmalloc.c:3416` | 底层固定 `GFP_KERNEL` |
| **`vzalloc(size)`** | `:3456` | `vmalloc \| __GFP_ZERO` |
| **`vmalloc_user(size)`** | `:3472` | 清零 + `VM_USERMAP`，可 `remap_vmalloc_range` 给用户态 |
| **`vmalloc_node(size, node)`** | `:3494` | 指定 NUMA 节点 |
| **`__vmalloc(size, gfp)`** | `:3397` | 自定义 gfp |
| **`vmalloc_huge(size, gfp)`** | **`:3435`** | **v6.6 新增，允许用大页** |
| **`vmap(pages[], n, flags, prot)`** | `:2894` | 把**已有**的页数组映射成连续 VA |
| **`vmap_pfn(pfns[], n, prot)`** | `:2968` | 按 **PFN 数组**映射（不要求有 `struct page`） |
| **`vm_map_ram(pages[], n, node)`** | `include/linux/vmalloc.h:131` | 小批量、不走 vm_struct 的轻量映射 |
| **`vfree(addr)`** | `:2807` | 释放；**中断上下文自动降级** |
| **`vfree_atomic(addr)`** | `:2773` | 显式走延迟释放，**NMI 里仍不可用** |
| **`vcalloc(n, size)`** | `include/linux/vmalloc.h:159` | 带溢出检查的数组版 `vzalloc` |
| **`vmalloc_array(n, size)`** | `include/linux/vmalloc.h:157` | 带溢出检查的数组版 |

`vm_struct->flags` 位（v6.6 `include/linux/vmalloc.h:19`）：

| 标志 | 值 | 含义 |
|------|-----|------|
| `VM_IOREMAP` | `0x01` | `ioremap()` 及其变体 |
| `VM_ALLOC` | `0x02` | `vmalloc()` — **可 `vfree` 释放物理页** |
| `VM_MAP` | `0x04` | `vmap()` |
| `VM_USERMAP` | `0x08` | 可 `remap_vmalloc_range` |
| `VM_NO_GUARD` | `0x40` | `***DANGEROUS***` 不加保护页 |
| **`VM_FLUSH_RESET_PERMS`** | `0x100` | 解除映射时要重置 direct map 权限 + flush TLB，**不能在原子上下文释放** |
| **`VM_ALLOW_HUGE_VMAP`** | `0x400` | 允许用大页（见下节） |

> **保护页细节**：默认 vmalloc 区域**尾部多带一个 guard page**，
> `get_vm_area_size()`（`:196`）算可用大小时要 `area->size - PAGE_SIZE`。
> 所以越界写一个字节不会立刻踩到别人的数据 —— 但**只有下溢方向**有保护。

#### 分配路径五步

```c
/* mm/vmalloc.c:3235 */
void *__vmalloc_node_range(unsigned long size, unsigned long align,
			unsigned long start, unsigned long end, gfp_t gfp_mask,
			pgprot_t prot, unsigned long vm_flags, int node,
			const void *caller)
{
	/* ① 硬上限：请求的页数不能超过系统总页数 */
	if ((size >> PAGE_SHIFT) > totalram_pages()) {     /* :3250 */
		warn_alloc(gfp_mask, NULL,
			"vmalloc error: size %lu, exceeds total pages", real_size);
		return NULL;
	}
	...
again:
	/* ② 在 [start,end) 里找 VA 空隙 —— 这里会 might_sleep() */
	area = __get_vm_area_node(real_size, align, shift, VM_ALLOC |
				  VM_UNINITIALIZED | vm_flags, start, end, node,
				  gfp_mask, caller);                      /* :3280 */
	...
	/* ③ 逐页从 buddy 取页 + ④ 填页表 + ⑤ KASAN unpoison */
}
```

第 ② 步内部（`mm/vmalloc.c:1600`）是**上下文约束的真正来源**：

```c
/* mm/vmalloc.c:1600 */
	might_sleep();
	gfp_mask = gfp_mask & GFP_RECLAIM_MASK;

	va = kmem_cache_alloc_node(vmap_area_cachep, gfp_mask, node);
```

> 两行都是硬约束：
> - `might_sleep()` —— 原子上下文调用会触发 `CONFIG_DEBUG_ATOMIC_SLEEP` 告警；
> - `gfp_mask & GFP_RECLAIM_MASK` —— **剥掉 `__GFP_HIGH` / `__GFP_KSWAPD_RECLAIM` 等回收位**。
>   也就是说**即使你传 `GFP_ATOMIC` 给 `__vmalloc()`，也会被这里洗成可睡眠的**。
>   想"传个原子标志就能在中断里 vmalloc"是**行不通的**。

---

#### 【断崖】`vmalloc_huge()`：vmalloc 也能用大页了

LKD 时代（2.6 / 3.x）的定论是"**vmalloc 一定是 4KB 粒度，TLB 压力天然大**"。
**v6.6 已经打破了这个定论**：

```c
/* mm/vmalloc.c:3423 —— v6.6 原文 */
/**
 * vmalloc_huge - allocate virtually contiguous memory, allow huge pages
 * @size:      allocation size
 * @gfp_mask:  flags for the page level allocator
 *
 * Allocate enough pages to cover @size from the page level
 * allocator and map them into contiguous kernel virtual space.
 * If @size is greater than or equal to PMD_SIZE, allow using
 * huge pages for the memory
 */
void *vmalloc_huge(unsigned long size, gfp_t gfp_mask)
{
	return __vmalloc_node_range(size, 1, VMALLOC_START, VMALLOC_END,
				    gfp_mask, PAGE_KERNEL, VM_ALLOW_HUGE_VMAP,
				    NUMA_NO_NODE, __builtin_return_address(0));
}
EXPORT_SYMBOL_GPL(vmalloc_huge);
```

生效条件（`mm/vmalloc.c:3257`）：

```c
	if (vmap_allow_huge && (vm_flags & VM_ALLOW_HUGE_VMAP)) {
		size_per_node = size;
		if (node == NUMA_NO_NODE)
			size_per_node /= num_online_nodes();     /* NUMA 上按节点数摊 */
		if (arch_vmap_pmd_supported(prot) && size_per_node >= PMD_SIZE)
			shift = PMD_SHIFT;                        /* 2MB 大页 */
		else
			shift = arch_vmap_pte_supported_shift(size_per_node);

		align = max(real_align, 1UL << shift);
		size = ALIGN(real_size, 1UL << shift);
	}
```

| 条件 | 不满足的后果 |
|------|-------------|
| `vmap_allow_huge` 为真（默认 true，可被 `nohugevmalloc` 启动参数关掉，`mm/vmalloc.c:66~75`） | 完全不尝试大页 |
| 调用者传 `VM_ALLOW_HUGE_VMAP` | 普通 `vmalloc()` **不会**用大页 |
| `prot` 支持 PMD 映射且 `size/nr_nodes >= PMD_SIZE`（2MB） | 退到 `arch_vmap_pte_supported_shift()`（x86 仍是 4KB） |

> **为什么普通 `vmalloc()` 不默认开大页？** 源码注释给了答案（`mm/vmalloc.c:3260`）：
> *"Only try for PAGE_KERNEL allocations, others like modules don't yet expect huge pages in their allocations due to `apply_to_page_range` not supporting them."*
> 即：**大页映射会让 `apply_to_page_range()` 一族失效**，模块加载等依赖逐页遍历的路径会炸。
> 所以是"按需开"，不是"默认开"。

#### x86_64 上 vmalloc 有多大

```c
/* arch/x86/include/asm/pgtable_64_types.h:124 */
#define __VMALLOC_BASE_L4	0xffffc90000000000UL
#define __VMALLOC_BASE_L5 	0xffa0000000000000UL

#define VMALLOC_SIZE_TB_L4	32UL        /* 4 级页表：32 TB */
#define VMALLOC_SIZE_TB_L5	12800UL     /* 5 级页表：12800 TB */
```

| 配置 | 起始地址 | 大小 |
|------|---------|------|
| 4 级页表（常见） | `0xffffc90000000000` | **32 TB** |
| 5 级页表 | `0xffa0000000000000` | **12800 TB** |
| 开 KMSAN | 同上起点 | 只取 **1/4**（`VMALLOC_QUARTER_SIZE`，`:169`） |

对比 `VMALLOC_TOTAL = VMALLOC_END - VMALLOC_START`（`include/linux/vmalloc.h:284`）——
**VA 几乎不是瓶颈，物理内存和 TLB 才是**。

---

#### 释放路径：lazy purge（v6.6 与教科书最大的差别）

教科书说"**vfree 要 flush 内核 TLB，很贵**"。v6.6 的实际情况是：**不立即 flush**。

```c
/* mm/vmalloc.c:1817 —— 解除映射后，只把区间挂进 purge 列表 */
static void free_vmap_area_noflush(struct vmap_area *va)
{
	nr_lazy = atomic_long_add_return((va->va_end - va->va_start) >>
				PAGE_SHIFT, &vmap_lazy_nr);

	spin_lock(&purge_vmap_area_lock);
	merge_or_add_vmap_area(va, &purge_vmap_area_root, &purge_vmap_area_list);
	spin_unlock(&purge_vmap_area_lock);
	...
}
```

攒够了才一次性 flush：

```c
/* mm/vmalloc.c:1700 */
static unsigned long lazy_max_pages(void)
{
	unsigned int log;

	log = fls(num_online_cpus());

	return log * (32UL * 1024 * 1024 / PAGE_SIZE);
}
```

```c
/* mm/vmalloc.c:1749 —— 真正的 flush 只在这里发生一次 */
	flush_tlb_kernel_range(start, end);
```

```
vfree(A); vfree(B); vfree(C);            ← 三次都只改页表 + 挂 purge 列表，不 flush
        │
        └─► vmap_lazy_nr 累加，超过 lazy_max_pages()
                 │
                 └─► __purge_vmap_area_lazy():
                       flush_tlb_kernel_range(min_start, max_end)   ← 一次搞定
                       把区间真正还给 free 树
```

| CPU 数 | `fls(nr_cpus)` | `lazy_max_pages` | 折合 |
|--------|----------------|------------------|------|
| 1 | 1 | 8192 页 | 32 MB |
| 4 | 3 | 24576 页 | 96 MB |
| 8 | 4 | 32768 页 | 128 MB |
| 64 | 7 | 57344 页 | 224 MB |

> **注释里那句"less aggressive log scale"值得读懂**（`mm/vmalloc.c:1694`）：
> 阈值**按 CPU 数取对数缩放**而不是线性。理由很工程 —— 核数翻倍不代表 vmap 活跃度翻倍，
> 而且在大系统上一次 purge 太大会引入长延迟。**用对数换"可预测的尾延迟"**。
> 这个取向和 HFT 的取舍完全一致：**宁可平均成本高一点，也不要偶发的巨大停顿**。

#### `vfree()` 的上下文降级

```c
/* mm/vmalloc.c:2807 —— v6.6 原文 */
void vfree(const void *addr)
{
	struct vm_struct *vm;
	int i;

	if (unlikely(in_interrupt())) {
		vfree_atomic(addr);
		return;
	}

	BUG_ON(in_nmi());
	kmemleak_free(addr);
	might_sleep();
	...
```

```c
/* mm/vmalloc.c:2773 */
void vfree_atomic(const void *addr)
{
	struct vfree_deferred *p = raw_cpu_ptr(&vfree_deferred);

	BUG_ON(in_nmi());
	kmemleak_free(addr);

	/*
	 * Use raw_cpu_ptr() because this can be called from preemptible
	 * context. Preemption is absolutely fine here, because the llist_add()
	 * implementation is lockless, so it works even if we are adding to
	 * another cpu's list. schedule_work() should be fine with this too.
	 */
	if (addr && llist_add((struct llist_node *)addr, &p->list))
		schedule_work(&p->wq);
}
```

| 事实 | 说明 |
|------|------|
| `vfree()` **在中断里可以直接调** | 内部 `in_interrupt()` 时自动转 `vfree_atomic` |
| 这个降级 **不是新特性** | 实测 v4.19 `:1590` 就已经是 `if (unlikely(in_interrupt())) __vfree_deferred(addr);` |
| 降级机制 **换代了** | v4.19 用**全局** `vmap_purge_list`（单个 llist，多核竞争）；v6.6 用 **per-CPU `vfree_deferred` + per-CPU llist + `schedule_work`**，靠 `llist_add` 的 lockless 特性免锁 |
| **NMI 里绝对不行** | `BUG_ON(in_nmi())` —— NMI 不能 `schedule_work` |
| **带 `VM_FLUSH_RESET_PERMS` 的区域不能在原子上下文释放** | 见 `include/linux/vmalloc.h:28` 注释原文 |

> **易错点**：很多人记成"vfree 和 vmalloc 一样不能进中断"。**反了**。
> 准确说法是：**分配不能进原子上下文，释放可以（会自动延迟）**。
> 但释放被延迟意味着 —— **中断里 `vfree` 之后，那块地址可能还没真正解除映射**，
> 页表和 TLB 的清理要等工作队列跑。这在审计"释放后立刻重新分配同地址"的代码时是个坑。

---

#### 上下文约束速查

| 场景 | `kmalloc(GFP_KERNEL)` | `kmalloc(GFP_ATOMIC)` | `vmalloc()` | `vfree()` |
|------|----------------------|----------------------|-------------|-----------|
| 进程上下文，可睡眠 | ✅ | ✅ | ✅ | ✅ |
| 持有 `spinlock` | ❌ | ✅ | ❌ | ✅（延迟） |
| 硬中断 handler | ❌ | ✅ | ❌ | ✅（延迟） |
| NMI | ❌ | ✅（仅部分架构） | ❌ | ❌ `BUG_ON` |
| 可能直接回收内存 | ✅ | ❌ | ✅ | ✅ |

> `vmalloc` 那一列全是 ❌ 的根本原因就在 `mm/vmalloc.c:1600` 的 `might_sleep()` + `GFP_RECLAIM_MASK` 掩码。

#### 决策：什么时候选谁

| 需求 | 选 | 理由 |
|------|-----|------|
| **< 8 KB**，高频 | **`kmalloc`** | slab 缓存，无页表操作（v6.6 `KMALLOC_MAX_CACHE_SIZE = 8KB`） |
| **8 KB ~ 4 MB**，要物理连续 | **`kmalloc` / `alloc_pages`** | 走 buddy 高阶块，`MAX_ORDER=10` 上限 4MB |
| **> 4 MB**，或不要物理连续 | **`vmalloc`** | buddy 给不了，`vmalloc` 能拼 |
| 大块 + **热访问** | **`vmalloc_huge`** 或 hugetlb | 2MB 大页降 TLB 压力 |
| **DMA** | **`dma_alloc_coherent`** | 需要设备可见的物理地址 |
| **MMIO 寄存器** | **`ioremap`** | 非 RAM，不能走 `vmalloc` |
| 已有 `struct page[]` 想拼成连续 VA | **`vmap`** | 不重新分配物理页 |

#### 典型用途

| 场景 | 为何 vmalloc |
|------|--------------|
| **内核模块加载** | 代码/数据 **大块**、加载期一次 —— 但注意模块**不开**大页 |
| **大表 / debug** | `/proc` 大缓冲、驱动 **big config table** |
| **filesystem 元数据** | 非热路径大结构 |
| **BPF map 的大值区** | 远超 4MB 时只能在 VA 层面拼 |

#### 何时 **不要** vmalloc

| 场景 | 改用 |
|------|------|
| **网络包处理热路径** | **`kmalloc` / 预分配池** |
| **DMA 缓冲** | **`dma_alloc_coherent`** / **`alloc_pages`** |
| **中断上下文频繁 alloc** | **Slab cache 预建**（初始化时 `kmem_cache_create`，运行时只 `kmem_cache_alloc`） |
| **持自旋锁期间** | 提前在锁外分配好，或改 `GFP_ATOMIC` 的 `kmalloc` |
| **延迟有硬上限的路径** | 预分配 + 复用，任何分配都不做 |

**HFT：** 用户态 **`mmap` 大块匿名区** 与 **`vmalloc`** 思想类似 —— **虚连续**。
但 **策略 ring** 还要 **`mlock` + hugepage** 保证 **物理稳定 + TLB 友好**。
普通 **`vmalloc` 无 hugepage 语义**（`vmalloc_huge` 才有），热路径 **禁用**。

> **给策略引擎的三条硬规则**：
> ① 热路径**一次都不分配** —— 用初始化阶段建好的 pool / slab cache；
> ② 非用大块不可时，用 **`vmalloc_huge` + 一次性分配**，绝不放进 per-packet 路径；
> ③ 警惕 **lazy purge 的长尾** —— 内核里如果有人高频 `vfree`，
> 达到 `lazy_max_pages` 时会来一次 `flush_tlb_kernel_range()`，
> 这是**内核态的全局 TLB shootdown**，同一时刻其他核上的策略线程也会被 IPI 打断。
> "我们没调 vmalloc"不代表你能躲开——**同机器的内核模块在调就行**。

---

#### 观测与调试

| 接口 | 能看到什么 |
|------|-----------|
| **`/proc/vmallocinfo`** | 每条 `vm_struct`：**VA 范围 / 大小 / 调用者（`caller` 字段）/ flags / 物理页** |
| **`/proc/meminfo` → `VmallocUsed`** | 已用 vmalloc 页数（来自 `nr_vmalloc_pages` / `vmap_lazy_nr`） |
| **`VMALLOC_TOTAL`** | VA 总量（`include/linux/vmalloc.h:284`） |
| **`/proc/vmallocinfo \| sort -k2 -n`** | 按大小排序找"谁在吃 VA" |
| **`kasan` + `CONFIG_KASAN_VMALLOC`** | vmalloc 越界检测（v6.6 里 `__vmalloc_node_range` 有 `kasan_unpoison_vmalloc` 分支） |
| **tracepoint** | `trace_purge_vmap_area_lazy`（`:1782`）、`trace_alloc_vmap_area`（`:1619`）、`trace_free_vmap_area_noflush`（`:1837`） |

```bash
# 谁在吃 vmalloc 空间，按大小倒序
awk '{print $2, $3, $0}' /proc/vmallocinfo | sort -rn | head -20

# 观察 lazy purge 是否被触发（阈值 = fls(nr_cpu) * 32MB）
perf trace -e vmm:purge_vmap_area_lazy
```

---

→ [06 Gorman Ch7 非连续分配](../../../06-linux-mm/chapter-07-noncontiguous-memory-allocation/) · [Ch 15 mmap 用户视角](../../chapter-15-process-address-space/) · [01 CSAPP Ch9](../../../02-computer-systems/chapter-09-virtual-memory/) · [12.4 获得页](./section-12.4-获得页.md) · [12.5 kmalloc](./section-12.5-kmalloc-与-kfree.md)


> ↔ [ULK Ch8 §4 非连续内存与vmalloc](../../../16-linux-kernel-deep/chapter-08-memory-management/notes/section-4-非连续内存与vmalloc.md)


<details>
<summary>自测题（点击展开）</summary>

**Q1.** vmalloc 为什么比 kmalloc 慢？

<details><summary>答案</summary>

vmalloc 需要：1) 从 buddy 分配零散物理页；2) 修改页表把散页映射到连续虚拟地址；3) flush TLB（其他 CPU 上的 TLB 也要 IPI flush）。kmalloc 从 slab 直接返回已映射的连续物理内存。vmalloc 的 TLB flush 是主要开销，在多核系统上尤其昂贵。

> **按 v6.6 精确化（补充，不推翻原答案）**：
> ① **分配侧**的页表开销确实存在（逐页 `vmap_pages_range`），但"每次都 flush TLB"只在**释放侧**，
> 而且释放走 **lazy purge** —— 攒到 `lazy_max_pages() = fls(nr_cpus) × 32MB` 才 flush 一次（`mm/vmalloc.c:1749`）。
> 所以"慢"的主因更准确说是：**逐页建 PTE + TLB miss 率高（4KB 粒度）**，而不是"每次都全局 flush"。
> ② `kmalloc` 也不是"从 slab 直接返回"那么简单 —— **> 8KB 会走 `kmalloc_large()` 直连 buddy**
> （`include/linux/slab.h:595`）。"slab 快"只对 ≤ 8KB 成立。
> ③ x86_64 上其他进程的顶层页表**不主动同步**，靠缺页异常 `vmalloc_fault()` 从 reference pgd 回抄
> （`arch/x86/mm/fault.c:231`），这又是一笔按需成本。

</details>

**Q2.** vmalloc 分配的内存能用于 DMA 吗？为什么？

<details><summary>答案</summary>

不能直接用于 DMA。vmalloc 内存物理不连续，DMA 设备需要物理连续地址（或需要 scatter-gather 支持）。如果要用 vmalloc 内存做 DMA，需要逐页映射：`for each page: dma_map_page()`，性能差且复杂。DMA buffer 应该用 kmalloc 或 alloc_pages。

> **按 v6.6 补充**：`struct vm_struct` 里有 `struct page **pages`（`include/linux/vmalloc.h:54`），
> 所以技术上**可以**遍历出来逐页 `dma_map_page()`。但注意：
> - 若这块是用 `vmalloc_huge()` 分的，`pages[]` 里存的是**复合页的头页**，
>   要用 `page_order` 判断实际阶数，否则 scatter-gather 列表会算错长度；
> - 更根本的问题是 **`vmalloc` 出来的页不保证来自 `ZONE_DMA/DMA32`**，
>   设备的寻址位宽限制（32 位设备）同样过不去。
> 结论不变：**DMA 用 `dma_alloc_coherent()`**。

</details>

**Q3.** 驱动在 `spin_lock` 保护的数据路径里想分配 2MB 的临时缓冲，用 `vmalloc(2*1024*1024)` 会怎样？正确做法是什么？

<details><summary>答案</summary>

**会出问题。** `vmalloc` 走到 `__get_vm_area_node()`（`mm/vmalloc.c:1600`）时有 `might_sleep()`；
开了 `CONFIG_DEBUG_ATOMIC_SLEEP` 会在持锁期间打印 "BUG: sleeping function called from invalid context"。
即使没开调试选项，`kmem_cache_alloc_node(vmap_area_cachep, ...)` 也可能真的睡眠 ——
**持自旋锁睡眠是死锁风险**（别的 CPU 等着这把锁，唤醒者可能正是等锁者）。

**更隐蔽的一点**：传 `GFP_ATOMIC` 给 `__vmalloc()` 也救不了你 —— `:1601` 的
`gfp_mask = gfp_mask & GFP_RECLAIM_MASK` 会把 `__GFP_HIGH`/`__GFP_KSWAPD_RECLAIM` 等回收位**全部剥掉**，
`might_sleep()` 依然在。

**正确做法**（按优先级）：
1. **初始化阶段就分好** —— probe/初始化时 `vmalloc` 或 `kmalloc` 一块，运行时只复用；
2. 非要运行时分配且必须持锁 → 改成 **`kmem_cache_alloc(..., GFP_ATOMIC)`**（≤8KB）或
   **`alloc_pages(GFP_ATOMIC, order)`**；
3. 2MB 已超过 buddy 单次上限的一半，且 `GFP_ATOMIC` 拿 2MB 连续页成功率很低 ——
   最稳的是**启动时预留 + 自己管池子**，这正是 HFT/网络驱动的通行做法。

</details>

**Q4.** 中断 handler 里 `vfree(ptr)` 之后，立刻检查这段地址是否已经解除映射，会看到什么？

<details><summary>答案</summary>

**映射可能还在。** `vfree()` 在 `in_interrupt()` 为真时**自动降级**为 `vfree_atomic()`（`mm/vmalloc.c:2812`）：

```c
	if (unlikely(in_interrupt())) {
		vfree_atomic(addr);
		return;
	}
```

`vfree_atomic()` 只做两件事：把地址 `llist_add` 进本 CPU 的 `vfree_deferred.list`，
然后 `schedule_work()`。真正的解除映射 + 归还页表要等**工作队列在进程上下文跑起来**之后。

所以中断返回后立刻读这段地址：
- 页表大概率**还没清**，读写不会 fault（但这是 use-after-free，KASAN 会抓）；
- 物理页的释放同样被推迟；
- 如果这段带 `VM_FLUSH_RESET_PERMS`，那它**根本不该**在原子上下文释放（`include/linux/vmalloc.h:28`）。

**实践结论**：中断里可以 `vfree`，但"释放"是**异步**的。
任何依赖"释放即刻生效"的逻辑（比如释放后立刻重新分配并期望拿到同一地址）在中断路径上都是错的。

</details>

**Q5.** 一台 8 核机器上，内核模块每秒 `vmalloc/vfree` 几千次小区域（各 64KB）。为什么其他核上的延迟线程偶发尖刺？怎么定位？

<details><summary>答案</summary>

**根因是 lazy purge 的批量 TLB flush。**

`vfree` 不立即 flush，只累加 `vmap_lazy_nr`。当
`vmap_lazy_nr > lazy_max_pages() = fls(8) × 32MB = 4 × 32MB = 128MB`（折合 32768 页）时，
`__purge_vmap_area_lazy()` 触发一次 `flush_tlb_kernel_range(start, end)`（`mm/vmalloc.c:1749`）。

`flush_tlb_kernel_range` 是**跨核 TLB shootdown**：向所有 CPU 发 IPI，每个 CPU 在中断上下文
执行 `flush` 并回复，发起方等全部 ACK。8 核上一次几十微秒量级，**每个参与核都被打断**。
每秒几千次 × 64KB = 每秒几百 MB，意味着**每秒会触发好几次全局 shootdown** ——
延迟线程被自己没干过的事拖累，这就是"尖刺"的来源。

**定位步骤**：
```bash
# 1. 确认 purge 频率
perf trace -e vmm:purge_vmap_area_lazy      # tracepoint 在 mm/vmalloc.c:1782

# 2. 看 vmalloc 用量与调用者
awk '{print $2, $3}' /proc/vmallocinfo | sort -rn | head

# 3. 对比：尖刺时刻是否和 purge trace 对齐
perf record -e vmm:purge_vmap_area_lazy -a -- sleep 10
```

**缓解**：
- 让那个模块**复用**分配（池化），把 `vfree` 频率降下来；
- 或者改用 `kmalloc` / slab cache（≤8KB 场景），完全绕开 vmalloc 的页表与 flush 路径；
- 大块场景用 `vmalloc_huge()` —— 2MB 粒度让 PTE 数量除以 512，purge 时遍历的区间也少得多；
- 不要试图调 `lazy_max_pages` —— 它是 `static`，只为**减小** purge 的**频率**而设计，
  调大只会让单次停顿更长。

</details>

**Q6.** `vmalloc_huge()` 已经能用 2MB 大页了，为什么普通 `vmalloc()` 不默认开？

<details><summary>答案</summary>

源码注释直接给了理由（`mm/vmalloc.c:3260`）：

```c
		/*
		 * Try huge pages. Only try for PAGE_KERNEL allocations,
		 * others like modules don't yet expect huge pages in
		 * their allocations due to apply_to_page_range not
		 * supporting them.
		 */
```

三个层次的原因：

1. **`apply_to_page_range()` 一族不支持大页映射**。
   内核里大量代码靠它做"遍历 vmalloc 区域的每一页"（改权限、做 KASAN  poisoning、
   `set_memory_ro/rw/nx` 等）。一旦底层是 PMD 大页，这些遍历的语义就变了 ——
   老代码会静默出错。所以只能让**明确知道自己要什么**的调用者开。

2. **只有 `PAGE_KERNEL` 才考虑大页**（`:3261`）。`ioremap`、模块加载等用的是其他 `prot`，
   它们的映射语义和缓存属性不与大页兼容。

3. **大页会放大内部碎片**。`__vmalloc_node_range` 里 `size = ALIGN(real_size, 1UL << shift)`（`:3276`），
   申请 2MB + 1 字节会**对齐到 4MB**（两个 PMD）。对"就要一大块"的调用者无所谓，
   对"要几千个小块"的调用者是灾难。

所以设计成 **opt-in**：`vmalloc_huge()` 传 `VM_ALLOW_HUGE_VMAP`（`include/linux/vmalloc.h:30`），
且 `EXPORT_SYMBOL_GPL` —— **只有 GPL 模块能用**，进一步收窄爆炸半径。

</details>

</details>
---
