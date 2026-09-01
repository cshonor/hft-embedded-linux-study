## ③ 虚拟内存区域 · VMA · `vm_area_struct`

地址空间不是 **均匀** 的 — 内核按 **段** 管理，每段是一个 **VMA**，带 **独立权限、后备存储、操作回调**。

> **本篇分工**：实体书已讲 VMA 是什么。本篇只做三件事：
> ① 用 v6.6 源码**逐字列出 `vm_area_struct` 的真实字段**（书上的字段表已经过期）；
> ② **订正两个必错的说法** —— "`vma->vm_flags` 可以直接改"和"匿名 VMA 就是 `vm_file == NULL`"；
> ③ 讲清**一个 VMA 同时挂在三棵不同的树/链表上**（这是理解反向映射的钥匙）。

---

## 1. v6.6 的 `vm_area_struct` 逐字实证

```c
/* include/linux/mm_types.h:569 */
struct vm_area_struct {
	/* The first cache line has the info for VMA tree walking. */

	union {
		struct {
			/* VMA covers [vm_start; vm_end) addresses within mm */
			unsigned long vm_start;
			unsigned long vm_end;
		};
		struct rcu_head vm_rcu;		/* CONFIG_PER_VMA_LOCK：延迟释放用 */
	};

	struct mm_struct *vm_mm;		/* 所属地址空间 */
	pgprot_t vm_page_prot;			/* Access permissions of this VMA. */

	/*
	 * Flags, see mm.h.
	 * To modify use vm_flags_{init|reset|set|clear|mod} functions.
	 */
	union {
		const vm_flags_t vm_flags;	/* ★ const！不能直接写 */
		vm_flags_t __private __vm_flags;
	};

	/* ---- CONFIG_PER_VMA_LOCK（v6.4+）---- */
	int vm_lock_seq;
	struct vma_lock *vm_lock;		/* 指向自己那把 rwsem */
	bool detached;				/* 已从 mm_mt 摘下（munmap 进行中） */

	/*
	 * For areas with an address space and backing store,
	 * linkage into the address_space->i_mmap interval tree.
	 */
	struct {
		struct rb_node rb;
		unsigned long rb_subtree_last;
	} shared;

	struct list_head anon_vma_chain;	/* Serialized by mmap_lock & page_table_lock */
	struct anon_vma *anon_vma;		/* Serialized by page_table_lock */

	const struct vm_operations_struct *vm_ops;	/* ★ 回调表 */

	unsigned long vm_pgoff;			/* 文件内偏移，单位是 PAGE_SIZE */
	struct file * vm_file;			/* 后备文件（可为 NULL） */
	void * vm_private_data;

	struct anon_vma_name *anon_name;	/* CONFIG_ANON_VMA_NAME：VMA 名字 */
	atomic_long_t swap_readahead_info;
	struct vm_region *vm_region;		/* NOMMU only */
	struct mempolicy *vm_policy;		/* CONFIG_NUMA */
	struct vma_numab_state *numab_state;	/* NUMA Balancing */
	struct vm_userfaultfd_ctx vm_userfaultfd_ctx;
} __randomize_layout;				/* ★ 布局随机化 */
```

### 1.1 第一个 cache line 是给树遍历用的

源码注释直说：**`vm_start` / `vm_end` 放在第一个 cache line**，因为 maple tree 遍历时每一层都要比较这两个值。

更有意思的是这个 **union**：

```c
	union {
		struct {
			unsigned long vm_start;
			unsigned long vm_end;
		};
		struct rcu_head vm_rcu;	/* Used for deferred freeing. */
	};
```

`vm_rcu` 与 `vm_start/vm_end` **复用同一块内存**（`struct rcu_head` 正好是两个指针 = 16 字节）。
原因是：VMA 一旦从 `mm_mt` 摘下，就没人能再通过地址找到它了，
此时 `vm_start/vm_end` 已经没用，可以安全地借给 RCU 做延迟释放的链表指针，**省 16 字节**。

### 1.2 ⚠️ 订正一：`vm_flags` 已经是 `const`，不能直写

```c
	union {
		const vm_flags_t vm_flags;
		vm_flags_t __private __vm_flags;
	};
```

**版本断崖**（多版本 `include/linux/mm_types.h` diff）：

| 版本 | 声明 |
|------|------|
| v6.0 / v6.1 / v6.2 | `vm_flags_t vm_flags;` ← 随便改 |
| **v6.3 起** | `const vm_flags_t vm_flags;` + `vm_flags_t __private __vm_flags;` |

`const` 让编译器挡掉所有 `vma->vm_flags |= X` 的直接赋值；
`__private` 让静态检查工具（sparse）挡掉所有绕过 `ACCESS_PRIVATE()` 的写法。
**必须走访问器**：

```c
/* include/linux/mm.h:840 */
static inline void vm_flags_set(struct vm_area_struct *vma, vm_flags_t flags)
{
	vma_start_write(vma);                       /* ★ 顺带拿 VMA 写锁 */
	ACCESS_PRIVATE(vma, __vm_flags) |= flags;
}
```

| 访问器 | 用途 |
|--------|------|
| `vm_flags_init(vma, flags)` | 初始化（VMA 还没进树） |
| `vm_flags_set(vma, flags)` | 置位 |
| `vm_flags_clear(vma, flags)` | 清位 |
| `vm_flags_mod(vma, set, clear)` | 同时置位和清位 |
| `vm_flags_reset(vma, flags)` | 整体替换（**注释明确警告：不加 VMA 锁**） |
| `vm_flags_reset_once(vma, flags)` | 只替换一次 |

> 为什么这么麻烦？因为 **per-VMA lock 的读者靠 `vm_flags` 裸读**（`lock_vma_under_rcu()` 里 `READ_ONCE`），
> 修改必须成对走 `vma_start_write()`，否则读者会看到撕裂的中间状态。

### 1.3 per-VMA lock 三件套（v6.4+）

```c
/* include/linux/mm_types.h:549 */
struct vma_lock {
	struct rw_semaphore lock;	/* 每个 VMA 一把读写信号量 */
};
```

```c
	int vm_lock_seq;		/* 与 mm->mm_lock_seq 配对 */
	struct vma_lock *vm_lock;	/* 自己那把锁（单独分配） */
	bool detached;			/* 已从 mm_mt 摘下 */
```

读锁的实现**不是**简单 `down_read`，而是配了一个**全局失效序列号**：

```c
/* include/linux/mm.h:653 */
static inline bool vma_start_read(struct vm_area_struct *vma)
{
	if (READ_ONCE(vma->vm_lock_seq) == READ_ONCE(vma->vm_mm->mm_lock_seq))
		return false;                          /* 快检：已被全局失效 */
	if (unlikely(down_read_trylock(&vma->vm_lock->lock) == 0))
		return false;                          /* 有人持有写锁 */
	if (unlikely(vma->vm_lock_seq == smp_load_acquire(&vma->vm_mm->mm_lock_seq))) {
		up_read(&vma->vm_lock->lock);
		return false;                          /* 拿到锁之后才发现刚被失效 */
	}
	return true;
}
```

设计要点：
- `mm->mm_lock_seq` 是**全局失效计数器**。`mmap_write_lock` 的持有者递增它，
  一次性作废**所有** VMA 读锁 → 不用逐个去抢。
- `smp_load_acquire` 与 `vma_end_write_all()` 的 RELEASE 配对。
- **溢出是允许的**（注释明说）：只会导致偶尔多走一次慢路径，不会出错。

---

## 2. ⚠️ 订正二：匿名 VMA 的判断是 `!vm_ops`，不是 `!vm_file`

常见误解："匿名映射 = `vm_file == NULL`"。**源码里不是这么判的**：

```c
/* include/linux/mm.h:875 */
static inline void vma_set_anonymous(struct vm_area_struct *vma)
{
	vma->vm_ops = NULL;
}

static inline bool vma_is_anonymous(struct vm_area_struct *vma)
{
	return !vma->vm_ops;
}
```

**为什么不是 `vm_file`？** 因为存在"**有文件、但没回调**"和"**没文件、但有回调**"两种情形：

| 情形 | `vm_file` | `vm_ops` | `vma_is_anonymous()` |
|------|-----------|----------|---------------------|
| 普通匿名映射（`MAP_ANONYMOUS`） | NULL | NULL | ✅ true |
| **匿名 `MAP_SHARED`** | NULL | **非 NULL**（`shmem_vm_ops`） | ❌ **false** |
| 文件私有映射（`MAP_PRIVATE`） | 非 NULL | 通常 NULL（走 generic） | 视 fs 而定 |
| 设备 `mmap`（如 `/dev/mem`、DRM） | 非 NULL | 非 NULL | ❌ false |
| hugetlbfs | 非 NULL | `hugetlb_vm_ops` | ❌ false |

关键例子：**`mmap(MAP_SHARED|MAP_ANONYMOUS)` 不是"匿名 VMA"**。
内核给它挂了 `shmem_vm_ops` 并用 `shmem_zero_setup()` 建了一个 tmpfs 的 inode 做后备
——所以它能跨 `fork`/进程共享，也能被 swap 出去。

```c
/* mm/mmap.c:2812 —— mmap_region 里的分支 */
	} else if (vm_flags & VM_SHARED) {
		error = shmem_zero_setup(vma);      /* 匿名共享 → 建 tmpfs 后备 */
		if (error)
			goto free_vma;
	} else {
		vma_set_anonymous(vma);             /* 匿名私有 → vm_ops = NULL */
	}
```

> **HFT 推论**：用 `MAP_SHARED|MAP_ANONYMOUS` 做跨进程行情环，
> 数据页是 **tmpfs/shmem 页**，走**页缓存**路径，可以 swap、受 `NR_SHMEM` 统计、
> 在 `/proc/pid/maps` 里显示为 `/dev/zero (deleted)` 或 `[shmem]`（取决于版本）。
> 想要"纯粹不落盘"的共享内存，用 **hugetlbfs + `MAP_SHARED`** 或 **`memfd_create` + `mlock`**。

---

## 3. `vm_operations_struct`：完整的 14 个回调

```c
/* include/linux/mm.h（节选） */
struct vm_operations_struct {
	void (*open)(struct vm_area_struct * area);
	void (*close)(struct vm_area_struct * area);
	int (*may_split)(struct vm_area_struct *area, unsigned long addr);
	int (*mremap)(struct vm_area_struct *area);
	int (*mprotect)(struct vm_area_struct *vma, unsigned long start,
			unsigned long end, unsigned long newflags);
	vm_fault_t (*fault)(struct vm_fault *vmf);
	vm_fault_t (*huge_fault)(struct vm_fault *vmf, unsigned int order);
	vm_fault_t (*map_pages)(struct vm_fault *vmf,
			pgoff_t start_pgoff, pgoff_t end_pgoff);
	unsigned long (*pagesize)(struct vm_area_struct * area);
	vm_fault_t (*page_mkwrite)(struct vm_fault *vmf);
	vm_fault_t (*pfn_mkwrite)(struct vm_fault *vmf);
	int (*access)(struct vm_area_struct *vma, unsigned long addr,
		      void *buf, int len, int write);
	const char *(*name)(struct vm_area_struct *vma);
	/* CONFIG_NUMA */
	int (*set_policy)(struct vm_area_struct *vma, struct mempolicy *new);
	struct mempolicy *(*get_policy)(struct vm_area_struct *vma, unsigned long addr);
	struct page *(*find_special_page)(struct vm_area_struct *vma, unsigned long addr);
};
```

| 回调 | 何时调用 | 典型实现 |
|------|----------|----------|
| **`open`** | VMA 创建后（`fork` 复制、`mremap`、`split` 产生新 VMA） | 驱动增加设备引用 |
| **`close`** | VMA 被移除（**注释明写：May sleep，调用者持 mmap_lock**） | 驱动释放资源 |
| **`may_split`** | `split_vma()` 之前，**决定允不允许劈开** | hugetlb 用它拒绝劈开大页 VMA |
| **`mremap`** | `mremap()` 之后通知驱动地址变了 | DRM / RDMA 重编程硬件 |
| **`mprotect`** | `mprotect()` 落地前做**驱动自己的权限检查** | 拒绝把 MMIO 映射改成可执行 |
| **`fault`** | 缺页（单页） | 文件映射从页缓存取页；设备映射返回 `pfn` |
| **`map_pages`** | 缺页时**批量预映射周围页**（fault-around） | `filemap_map_pages()` —— 顺带把相邻页也填进 PTE |
| **`huge_fault`** | 大页缺页 | hugetlbfs / DAX |
| **`pagesize`** | 该 VMA 的"自然页大小" | hugetlbfs 返回 2MB |
| **`page_mkwrite`** | **写一个已存在的只读 PTE**（即 COW 或共享可写文件的写通知） | 文件系统做 `update_mmu_cache` 前的记账 |
| **`pfn_mkwrite`** | 同 `page_mkwrite`，但用于 `VM_PFNMAP`/`VM_MIXEDMAP` | — |
| **`access`** | `access_process_vm()`（ptrace/gdb 读内存）失败后的兜底 | `generic_access_phys()` |
| **`name`** | `/proc/pid/maps` 问这个 VMA 该显示什么名字 | 返回 `"[drm_mm]"` 之类 |
| **`find_special_page`** | `vm_normal_page()` 遇特殊 PTE 时找 `struct page` | — |

**书上没有的两个**：

- **`map_pages` / fault-around**：缺页时内核会顺手把相邻的若干页一起填进 PTE。
  对文件映射由 `filemap_map_pages()` 完成，默认 fault-around 半径是 **16 页**（64KB）。
- **`may_split`**：v6.x 新增。`split_vma()` 之前先问驱动，hugetlb 用它阻止把大页 VMA 切成 4KB 粒度。

---

## 4. `vm_flags` 实证表（书上没有的标 ★）

| 标志 | 值 | 含义 |
|------|-----|------|
| `VM_READ` | `0x00000001` | 可读 |
| `VM_WRITE` | `0x00000002` | 可写 |
| `VM_EXEC` | `0x00000004` | 可执行 |
| `VM_SHARED` | `0x00000008` | `MAP_SHARED` |
| `VM_MAYREAD/WRITE/EXEC/SHARE` | `0x10/0x20/0x40/0x80` | **`mprotect()` 的上限**（`mprotect` 不能授予 `MAY` 之外的权限） |
| `VM_GROWSDOWN` | `0x00000100` | 向下增长（栈） |
| `VM_UFFD_MISSING` | `0x00000200` | userfaultfd 缺页跟踪 |
| `VM_PFNMAP` | `0x00000400` | 纯 PFN 映射，无 `struct page` |
| `VM_UFFD_WP` | `0x00001000` | userfaultfd 写保护跟踪 |
| **`VM_LOCKED`** | `0x00002000` | `mlock` 范围，计入 `locked_vm` |
| `VM_IO` | `0x00004000` | MMIO 映射，非 RAM |
| `VM_SEQ_READ` / `VM_RAND_READ` | `0x8000` / `0x10000` | 预读策略（`madvise` 设置） |
| **`VM_DONTCOPY`** | `0x00020000` | **`fork` 时不复制**（`MADV_DONTFORK`） |
| **`VM_DONTEXPAND`** | `0x00040000` | 禁止 `mremap` 扩展 |
| ★ **`VM_LOCKONFAULT`** | `0x00080000` | **只在缺页时才锁**（`mlock2(MLOCK_ONFAULT)`） |
| `VM_ACCOUNT` | `0x00100000` | 参与 overcommit 记账 |
| `VM_NORESERVE` | `0x00200000` | 不预留 swap/account（`MAP_NORESERVE`） |
| **`VM_HUGETLB`** | `0x00400000` | hugetlbfs / `MAP_HUGETLB` |
| ★ `VM_SYNC` | `0x00800000` | `MAP_SYNC`（持久化内存的同步映射） |
| ★ **`VM_WIPEONFORK`** | `0x02000000` | **`fork` 时子进程内容清零**（`MADV_WIPEONFORK`） |
| **`VM_DONTDUMP`** | `0x04000000` | 不包含在 core dump（`MADV_DONTDUMP`） |
| `VM_MIXEDMAP` | `0x10000000` | 同时含 `struct page` 和纯 PFN |
| ★ **`VM_HUGEPAGE`** | `0x20000000` | `MADV_HUGEPAGE` 标记（THP 候选） |
| ★ **`VM_NOHUGEPAGE`** | `0x40000000` | `MADV_NOHUGEPAGE` 标记（THP 禁区） |
| `VM_MERGEABLE` | `0x80000000` | KSM 可合并 |

组合掩码（源码定义）：

```c
#define VM_ACCESS_FLAGS (VM_READ | VM_WRITE | VM_EXEC)
#define VM_SPECIAL      (VM_IO | VM_DONTEXPAND | VM_PFNMAP | VM_MIXEDMAP)
#define VM_NO_KHUGEPAGED (VM_SPECIAL | VM_HUGETLB)
#define VM_LOCKED_MASK  (VM_LOCKED | VM_LOCKONFAULT)
#define VM_STACK        VM_GROWSDOWN        /* x86_64 上是这个 */
#define VM_STACK_FLAGS  (VM_STACK | VM_STACK_DEFAULT_FLAGS | VM_ACCOUNT)
```

---

## 5. 一个 VMA 同时挂在**三**个索引上

这是理解"反向映射"（Ch17 回收、Ch 16 页缓存）的钥匙。VMA 不只是"我这段 VA 是什么"，
它还回答"**谁映射了这个物理页**"：

```
                    ┌─────────────────────────┐
                    │  struct vm_area_struct   │
                    └───────┬─────┬───────┬───┘
                            │     │       │
        ┌───────────────────┘     │       └──────────────────┐
        ▼                         ▼                          ▼
 ① mm->mm_mt               ② file->f_mapping->i_mmap    ③ anon_vma_chain
   (maple tree)              (interval tree)              (双向链表 + rb)
   按【地址】索引             按【文件区间】索引            按【匿名页】索引
   回答：这个 VA 归谁        回答：这个文件的这段          回答：这个匿名页
                                  被哪些进程映射了             被哪些 VMA 映射了
   find_vma()               用于：truncate / msync /      用于：swap 回收时找
                                  page_mkwrite            到所有 PTE 去 unmap
   vma-> 无额外字段          vma->shared.rb               vma->anon_vma_chain
                                                          vma->anon_vma
```

```c
/* include/linux/rmap.h:82 */
struct anon_vma_chain {
	struct vm_area_struct *vma;
	struct anon_vma *anon_vma;
	struct list_head same_vma;	/* locked by mmap_lock & page_table_lock */
	struct rb_node rb;		/* locked by anon_vma->rwsem */
	unsigned long rb_subtree_last;
};
```

| 索引 | 什么时候挂上去 | 谁用它 |
|------|---------------|--------|
| ① `mm->mm_mt` | `mmap_region()` → `vma_iter_store()` | `find_vma()`、`/proc/pid/maps`、缺页 |
| ② `i_mmap` interval tree | `vma_interval_tree_insert()`（有 `vm_file` 且 `VM_SHARED` 或可写私有） | `unmap_mapping_range()`（`truncate`/`ftruncate`/`munmap` 文件映射）、`page_mkwrite` |
| ③ `anon_vma_chain` | `anon_vma_chain_link()`（缺页分配匿名页时惰性建立） | `try_to_unmap()`（回收/迁移匿名页）、`fork` 时的 COW 共享 |

> **HFT 推论**：`mlock` 之后页面不在 LRU 上、不参与回收 → **③ 这条链不再被遍历**，
> 也就没有 `try_to_unmap()` 的 IPI 和 rmap 锁竞争。这是 `mlockall` 除"不换出"之外的
> **第二个收益**：顺带消掉了回收路径的锁开销。

---

## 6. VMA 的生命周期（含 RCU 释放）

```
创建：mmap() → mmap_region()
      vm_area_alloc(mm)                 /* 从 slab 分配（vm_area_cachep） */
      → vm_flags_init(vma, vm_flags)
      → vma->vm_page_prot = vm_get_page_prot(vm_flags)
      → vma_iter_prealloc(&vmi, vma)    /* ★ 先给 maple 节点预分配内存 */
      → vma_start_write(vma)
      → vma_iter_store(&vmi, vma)       /* 插入 mm_mt */
      → mm->map_count++

销毁：munmap() → __do_munmap()
      vma_mark_detached(vma, true)      /* 先标记，per-VMA lock 读者看到会 retry */
      → vma_iter_clear_gfp(vmi, ...)    /* 从 mm_mt 摘掉 */
      → unmap_region()                  /* 拆 PTE + TLB flush */
      → remove_mt() → vm_area_free()    /* 释放 */
```

⚠️ **`vma_iter_prealloc()` 必须在 `vma_iter_store()` 之前**——
maple tree 插入时可能需要分配新节点，如果插入途中分配失败，树会处于半更新状态。
所以内核改成**先备好内存，再一次性插入**，让插入路径**不可能失败**。

释放走 RCU（`CONFIG_PER_VMA_LOCK` 时），因为可能有读者还在 RCU 读锁下持有这个 VMA 的指针
——这正是 §1.1 里 `vm_rcu` 与 `vm_start/vm_end` 共享内存的原因。

合并：**VMA 合并是常态，不是优化**。`mmap` 相邻同属性区间时，
`vma_merge()` / `vma_expand()` 会直接把已有 VMA 扩展，而不是新建。
所以 `/proc/pid/maps` 里看到的行数**远少于** `mmap` 调用次数。

⚠️ **反过来：合并失败会导致 VMA 数量爆炸**。
不同 `vm_flags`（比如有的 `MADV_HUGEPAGE` 有的没有）→ **不合并** → `map_count` 逼近
`sysctl_max_map_count`（默认 **65530**）→ `mmap` 开始返回 `-ENOMEM`。

---

## 7. 命名 VMA（v5.17+，`CONFIG_ANON_VMA_NAME`）

```c
#define PR_SET_VMA		0x53564d41
# define PR_SET_VMA_ANON_NAME		0
```

```c
/* 给一块匿名内存起名字 */
prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, ring, RING_SIZE, "orderbook");
```

产出（`/proc/pid/maps`）：
```
7f2b4c000000-7f2b4c200000 rw-p 00000000 00:00 0 [anon:orderbook]
```

> **HFT 上线审计必备**：不用再靠地址猜哪块内存是什么。
> 名字存在 `vma->anon_name`（`struct anon_vma_name *`），由 `mmap_lock` 保护。

---

## 8. HFT

每个 **策略缓冲** 应对应 **独立 VMA** — 便于 **`/proc/maps` 审计** 与 **`mlock` 精确范围**。
`MAP_SHARED` **行情 ring** 与 **私有 stack/heap** 分离 — **权限最小化**（ring **RW- 无 X**）。

v6.6 补充三条：

| 做法 | 理由 |
|------|------|
| **每线程/每用途一个独立 VMA** | 独立 VMA = 独立 per-VMA lock → 并发缺页不互斥（见 15.2 §6） |
| **`prctl(PR_SET_VMA_ANON_NAME)` 命名** | `/proc/pid/maps` 直接看出每块是什么，出问题时不用猜 |
| **监控 `map_count`** | 合并失败会让 VMA 数暴涨，逼近 65530 上限后 `mmap` 直接失败 |
| **`MAP_SHARED|MAP_ANONYMOUS` ≠ 纯匿名** | 它走 shmem/tmpfs 后备，**能 swap**（除非 mlock），且 `vma_is_anonymous()` 为 **false** |
| **`madvise(MADV_DONTFORK)`** | 置 `VM_DONTCOPY`，防 `fork` 时子进程复制（见 15.6） |

---

→ [Ch 15.6 mmap 创建](./section-15.6-创建与删除地址区间.md) · [Ch 15.8 缺页](./section-15.8-从访问到缺页概念.md) · [Ch 15.2 mm_struct](./section-15.2-内存描述符.md) · [06 Gorman VMA](../../../06-linux-mm/chapter-04-process-address-space/notes/section-3-内存区域.md)


> ↔ [ULK Ch9 §3 内存区VMA](../../../16-linux-kernel-deep/chapter-09-process-address-space/notes/section-3-内存区VMA.md)


<details>
<summary>自测题（点击展开）</summary>

**Q1.** VMA 的作用是什么？text/data/heap/stack 各是什么 VMA？

<details><summary>答案</summary>

VMA（vm_area_struct）描述一段连续虚拟地址区间的属性（起止地址/权限/映射方式/后备存储）。text=只读可执行、data=读写、heap=读写可扩展（brk）、stack=读写向下扩展。mmap 创建新 VMA。`cat /proc/pid/maps` 可看到进程所有 VMA。HFT mmap 共享内存会在 maps 中显示为独立 VMA。

**按 v6.6 补充**：
- VMA 除了"属性"还有第三重身份：**它是反向映射的索引节点**。
  一个 VMA 同时挂在 `mm->mm_mt`（按地址）、`file->f_mapping->i_mmap`（按文件区间）、
  `anon_vma_chain`（按匿名页）三个索引上，回收器靠它们找到"谁映射了这个物理页"。
- `/proc/pid/maps` 的行数**远少于** `mmap` 调用次数——`vma_merge()` 会合并相邻同属性区间。
- 上限：`sysctl_max_map_count = DEFAULT_MAX_MAP_COUNT = USHRT_MAX - 5 = 65530`
  （`include/linux/mm.h:194`），超了 `mmap` 直接 `-ENOMEM`。

</details>


**Q2.** VMA 的权限如何影响内存访问？

<details><summary>答案</summary>

VMA 权限位 VM_READ/VM_WRITE/VM_EXEC 控制用户态访问权限。写只读 VMA → page fault → SIGSEGV。内核态不受 VMA 权限限制（可写任何物理页）。VMA 权限 + PTE 权限双重检查：VMA 是粗粒度（段级），PTE 是细粒度（页级，如 COW 页标为只读）。

**按 v6.6 修订/补充**：

1. **"内核态不受 VMA 权限限制"这句话要收窄**。内核访问**用户**地址必须走
   `copy_{to,from}_user()` / `get_user()` / `put_user()`，它们带
   `access_ok()` 检查（落在用户范围），且缺页被 `__get_user` 的异常表捕获返回 `-EFAULT`。
   直接解引用用户指针会被 `CONFIG_HARDENED_USERCOPY` / SMAP 拦下（SMAP 直接在硬件层拦）。

2. **`mprotect` 的第三层限制：`VM_MAY*`**。`mprotect(PROT_EXEC)` 想成功，
   VMA 必须已经带了 `VM_MAYEXEC`。`do_mmap()` 里这一行决定了上限：
   ```c
   vm_flags |= calc_vm_prot_bits(prot, pkey) | calc_vm_flag_bits(flags) |
               mm->def_flags | VM_MAYREAD | VM_MAYWRITE | VM_MAYEXEC;
   ```
   如果 `mmap` 时没要 `PROT_READ`（很多 `mmap(PROT_WRITE)` 的写法），
   后面 `mprotect(PROT_READ)` 会**失败**——这是常见的踩坑点。

3. **v6.3+ 起还有一个进程级开关**：`prctl(PR_SET_MDWE, PR_MDWE_REFUSE_EXEC_GAIN)`
   声明后，本进程**不允许出现 W+X 映射，也不允许把非可执行 VMA 变成可执行**
   （`map_deny_write_exec()` 返回 `-EACCES`）。

</details>


**Q3.** 匿名 VMA 就是 `vm_file == NULL` 的 VMA 吗？`MAP_SHARED|MAP_ANONYMOUS` 是匿名 VMA 吗？

<details><summary>答案</summary>

**都不是。** 内核的判据是 `!vm_ops`，不是 `!vm_file`：

```c
/* include/linux/mm.h:880 */
static inline bool vma_is_anonymous(struct vm_area_struct *vma)
{
	return !vma->vm_ops;
}
```

| 映射方式 | `vm_file` | `vm_ops` | `vma_is_anonymous()` |
|---------|-----------|----------|---------------------|
| `MAP_PRIVATE|MAP_ANONYMOUS` | NULL | NULL | ✅ **true** |
| **`MAP_SHARED|MAP_ANONYMOUS`** | NULL | **`shmem_vm_ops`** | ❌ **false** |
| 文件 `MAP_SHARED` | 非 NULL | `generic_file_vm_ops` 之类 | ❌ false |
| hugetlbfs | 非 NULL | `hugetlb_vm_ops` | ❌ false |

原因（`mm/mmap.c:2812` 的分支）：
```c
	} else if (vm_flags & VM_SHARED) {
		error = shmem_zero_setup(vma);   /* 匿名共享 → 建 tmpfs/shmem 后备 */
	} else {
		vma_set_anonymous(vma);          /* 匿名私有 → vm_ops = NULL */
	}
```

**后果**：
- 匿名共享映射的数据页是 **shmem/tmpfs 页**，走**页缓存**路径；
- 它们**可以被 swap 出去**（除非 `mlock`），会被计入 `Shmem` 而不是 `AnonPages`；
- `/proc/pid/status` 里会出现在 `RssShmem` 而不是 `RssAnon`；
- `vma_is_anonymous()` 为 false，所以走 THP 的路径也不同（shmem THP 走 `CONFIG_SHMEM` 分支）。

**HFT 推论**：想要"跨进程共享 + 绝不落盘"，
`MAP_SHARED|MAP_ANONYMOUS` **必须配 `mlock`**，否则行情环在内存压力下会被换出。
更干净的做法是 `memfd_create(MFD_HUGETLB)` 或 hugetlbfs + `MAP_SHARED`。

</details>


**Q4.** v6.6 里能直接写 `vma->vm_flags |= VM_LOCKED` 吗？为什么？

<details><summary>答案</summary>

**不能。** v6.3 起 `vm_flags` 声明成 `const`：

```c
	union {
		const vm_flags_t vm_flags;
		vm_flags_t __private __vm_flags;
	};
```

多版本对照：

| 版本 | 声明 |
|------|------|
| v6.0 / v6.1 / v6.2 | `vm_flags_t vm_flags;` |
| **v6.3 起** | `const vm_flags_t vm_flags;` + `__private __vm_flags` |

两层拦截：
1. `const` → 编译器报错；
2. `__private` → sparse 静态检查报错（绕过 `ACCESS_PRIVATE()` 的写法会被抓）。

**必须走访问器**：`vm_flags_init/set/clear/mod/reset/reset_once`，
而且这些访问器会**顺带拿 VMA 写锁**：

```c
static inline void vm_flags_set(struct vm_area_struct *vma, vm_flags_t flags)
{
	vma_start_write(vma);                        /* ← 关键 */
	ACCESS_PRIVATE(vma, __vm_flags) |= flags;
}
```

**为什么必须加锁**：v6.4+ 的 per-VMA lock 让读者（缺页快路径）
在 **RCU 读锁下裸读 `vm_flags`**（`lock_vma_under_rcu()` 里的 `READ_ONCE`）。
如果写者不通过 `vma_start_write()`，读者可能看到撕裂的中间状态。

⚠️ 例外：`vm_flags_reset()` 的注释明确说 "**do not lock the vma**"，
它用于 VMA 还没插进 `mm_mt`（还没人能看到它）的初始化阶段。用错阶段会漏锁。

</details>


**Q5.** 一个 VMA 为什么要同时挂在三棵索引上？各自解决什么问题？

<details><summary>答案</summary>

因为内核需要回答**三个不同方向的查询**：

| # | 索引 | 数据结构 | 回答的问题 | 典型用户 |
|---|------|----------|-----------|---------|
| ① | `mm->mm_mt` | **maple tree** | "地址 X 属于哪个 VMA？" | `find_vma()`、缺页、`/proc/pid/maps` |
| ② | `file->f_mapping->i_mmap` | **interval tree** | "这个文件的第 N 页被哪些 VMA 映射了？" | `unmap_mapping_range()`（`truncate`/`ftruncate`）、`page_mkwrite` |
| ③ | `anon_vma_chain` | 双向链表 + rb | "这个匿名页被哪些 VMA 映射了？" | `try_to_unmap()`（回收/迁移）、`fork` 的 COW |

```c
struct anon_vma_chain {
	struct vm_area_struct *vma;
	struct anon_vma *anon_vma;
	struct list_head same_vma;	/* 同一 VMA 的所有 AVC */
	struct rb_node rb;		/* 挂进 anon_vma 的 rb 树 */
	unsigned long rb_subtree_last;
};
```

**为什么必须分开**：
- ① 是**正向映射**（VA → VMA），范围查询，用树；
- ②③ 是**反向映射**（物理页 → 所有 PTE）。回收一个页时必须找到**所有**映射它的 PTE 去解除，
  否则进程还能通过旧 PTE 访问到已经被分配给他人的物理页 —— **这是内存安全的硬要求**。
  文件页和匿名页的反向映射数据结构不同（文件有 `address_space` 可以挂 interval tree，
  匿名页没有），所以拆成两套。

**HFT 推论**：`mlock` 之后页面**不在 LRU 上、不参与回收** → ③ 这条链不再被遍历，
没有 `try_to_unmap()` 的 rmap 锁竞争和 TLB shootdown IPI。
这是 `mlockall` 除"不换出"之外的**第二个、常被忽略的收益**。

</details>

</details>
---
