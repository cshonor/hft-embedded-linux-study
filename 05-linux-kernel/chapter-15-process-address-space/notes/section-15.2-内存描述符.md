## ② 内存描述符 · `mm_struct`

**一个进程地址空间** 在内核里主要由 **`mm_struct`** 描述 — **同一地址空间的所有线程** 共享同一 **`mm`** 指针。

> **本篇分工**：实体书已讲字段含义。本篇只做三件事：
> ① 用 v6.6 源码**逐字列出 `mm_struct` 的真实字段**（书上的表已经过期了）；
> ② **订正一个全书级别的错误认知** —— `mmap` 链表和 `mm_rb` 红黑树**已经不存在了**；
> ③ 把 `mm_users` / `mm_count` / `active_mm` 讲成能用源码背下来的程度（这是全书最容易含混的一段）。

---

## 1. ⚠️ 先订正：`mmap` 链表与 `mm_rb` 红黑树已经没了

书的 Ch15.2 表格和网上大量资料都写着：

> ❌ `mmap` = VMA 单链表，`mm_rb` = VMA 红黑树，两者并存

**v6.6 里这两个字段都不存在。** 实证：

```bash
$ grep -n "mmap\|mm_rb\|maple_tree" include/linux/mm_types.h
587:		const vm_flags_t vm_flags;
588:		vm_flags_t __private __vm_flags;
690:		struct maple_tree mm_mt;
```

多版本对照（抓同名文件 diff，这是定位"断崖版本"的标准手法）：

| 版本 | VMA 容器 | 源码行 |
|------|----------|--------|
| **v6.0** | `struct vm_area_struct *mmap;` + `struct rb_root mm_rb;` | `mm_types.h:488-489` |
| **v6.1** | `struct maple_tree mm_mt;` | `mm_types.h:514` |
| **v6.6** | `struct maple_tree mm_mt;` | `mm_types.h:690` |

→ **断崖在 v6.1**：Liam Howlett 的 maple tree 一次性干掉了链表 + 红黑树。

### 为什么换？深度差一个数量级

```c
/* include/linux/maple_tree.h:29 —— 64 位下的节点容量 */
#define MAPLE_NODE_SLOTS	31	/* 256 bytes including ->parent */
#define MAPLE_RANGE64_SLOTS	16	/* 256 bytes */
#define MAPLE_ARANGE64_SLOTS	10	/* 240 bytes */
```

一个 maple 节点是 **256 字节 = 4 个 cache line**，64 位下装 **16 个槽位**（arange 节点 10 个，额外记录区间最大空隙）。
对比红黑树每下降一层就要 chase 一个指针（一次 cache miss）：

| VMA 数 | 红黑树深度（≈log₂N 次指针追踪） | maple tree 深度（≈log₁₆N） |
|--------|-------------------------------|---------------------------|
| 100 | ~7 | **2** |
| 1 000 | ~10 | **3** |
| 10 000 | ~14 | **4** |

而且 maple tree 带 `MT_FLAGS_USE_RCU`，**读路径不需要持 `mmap_lock`**——这正是后面
per-VMA lock（§6）能成立的前提。

**对 VMA 操作语义的影响（书上没有）**：

| 操作 | 红黑树时代 | maple tree 时代（v6.1+） |
|------|-----------|------------------------|
| 遍历所有 VMA | 走 `mm->mmap` 链表 | `for_each_vma()` = `mas_find()` 游标遍历树 |
| 找某地址的 VMA | `find_vma()` 走 `mm_rb` | `find_vma()` = `mt_find(&mm->mm_mt, &index, ULONG_MAX)` |
| 找空闲区间 | `mm->free_area_cache` 缓存上次位置 | **arange 节点里存了最大空隙**，直接查树 |
| 插入 | 链表 + 树两次维护 | **单次 `mas_store()`** |

```c
/* mm/mmap.c:1872 —— v6.6 的 find_vma()，就两行 */
struct vm_area_struct *find_vma(struct mm_struct *mm, unsigned long addr)
{
	unsigned long index = addr;
	mmap_assert_locked(mm);
	return mt_find(&mm->mm_mt, &index, ULONG_MAX);
}
```

---

## 2. v6.6 的 `mm_struct` 真实字段

```c
/* include/linux/mm_types.h:586（节选，去掉 #ifdef 分支） */
struct mm_struct {
	struct {
		struct {
			atomic_t mm_count;          /* ← 单独占一个 cacheline */
		} ____cacheline_aligned_in_smp;

		struct maple_tree mm_mt;            /* ★ VMA 索引树 */
		unsigned long (*get_unmapped_area)(struct file *filp, unsigned long addr,
					unsigned long len, unsigned long pgoff, unsigned long flags);
		unsigned long mmap_base;            /* mmap 区顶（top-down 起点） */
		unsigned long mmap_legacy_base;     /* bottom-up 起点 */
		unsigned long task_size;            /* 本进程的用户空间大小 */
		pgd_t * pgd;                        /* 页表根 */
		atomic_t membarrier_state;          /* 紧挨 pgd，与 switch_mm() 同 cacheline */
		atomic_t mm_users;                  /* ★ 用户线程计数 */
		atomic_long_t pgtables_bytes;       /* 页表占了多少字节 */
		int map_count;                      /* VMA 个数 */
		spinlock_t page_table_lock;
		struct rw_semaphore mmap_lock;      /* ★ 保护 mm_mt 与 VMA */
		struct list_head mmlist;            /* 挂到 init_mm.mmlist（可换出 mm 链表） */
		int mm_lock_seq;                    /* CONFIG_PER_VMA_LOCK 的序列号 */
		unsigned long hiwater_rss, hiwater_vm;
		unsigned long total_vm;             /* 已映射页数（含未兑现的） */
		unsigned long locked_vm;            /* mlock 的页数 */
		atomic64_t    pinned_vm;            /* FOLL_PIN 永久增引用 */
		unsigned long data_vm, exec_vm, stack_vm;
		unsigned long def_flags;            /* mmap 默认 flags */
		seqcount_t write_protect_seq;       /* fork 期间写保护 COW 用 */
		spinlock_t arg_lock;                /* 保护下面这几个 */
		unsigned long start_code, end_code, start_data, end_data;
		unsigned long start_brk, brk, start_stack;
		unsigned long arg_start, arg_end, env_start, env_end;
		unsigned long saved_auxv[AT_VECTOR_SIZE];
		struct percpu_counter rss_stat[NR_MM_COUNTERS];   /* ★ RSS 是 per-CPU 计数器 */
		struct linux_binfmt *binfmt;
		mm_context_t context;               /* 架构私有（x86 上是 LDT / ctx_id / tlb_gen） */
		unsigned long flags;                /* MMF_* 位，必须原子操作 */
		struct task_struct __rcu *owner;
		struct user_namespace *user_ns;
		struct file __rcu *exe_file;
		atomic_t tlb_flush_pending;
		struct uprobes_state uprobes_state;
		atomic_long_t hugetlb_usage;
		struct work_struct async_put_work;  /* mmdrop 的异步释放工作队列项 */
	} __randomize_layout;                   /* ★ 字段布局随机化（安全加固） */

	unsigned long cpu_bitmap[];             /* 动态大小，必须在最后 */
};
```

几个**书上没有但很重要**的点：

| 字段 | 要点 |
|------|------|
| `mm_count` | 被 `____cacheline_aligned_in_smp` 单独隔离到一个 cache line —— 它是**最热的原子变量** |
| `mmap_lock` | 是 `struct rw_semaphore`，不是 spinlock。`mmap`/`munmap` 持写锁，**缺页异常持读锁** |
| `rss_stat[]` | 类型是 `struct percpu_counter`，4 个计数项：`MM_FILEPAGES / MM_ANONPAGES / MM_SWAPENTS / MM_SHMEMPAGES` |
| `pgtables_bytes` | `atomic_long_t`，`/proc/pid/status` 的 `VmPTE` 就来自它 |
| `flags` | `MMF_*` 位（`include/linux/sched/coredump.h`），**必须用原子位操作** |
| `__randomize_layout` | 整个结构字段顺序**每次编译随机化**，防止内核漏洞利用时定位字段偏移 |
| `cpu_bitmap[]` | 柔性数组，大小 = `cpumask_size()`，必须放最后 |

### RSS 为什么是 per-CPU 计数器

```c
/* include/linux/mm_types_task.h:26 */
enum {
	MM_FILEPAGES,	/* Resident file mapping pages */
	MM_ANONPAGES,	/* Resident anonymous pages */
	MM_SWAPENTS,	/* Anonymous swap entries */
	MM_SHMEMPAGES,	/* Resident shared memory pages */
	NR_MM_COUNTERS
};
```

和 Ch12.10 的 `percpu_counter` 完全同一套机制：**加计数只改本 CPU 的副本，不加锁**；
读到 batch 阈值才结算到全局。代价是**读出来不准**（误差上界 ≈ `batch × nr_cpus`），
好处是**多线程 mmap/fault 时不打架**。

> **HFT 推论**：多线程程序频繁 `mmap`/`munmap` 时，RSS 更新的可扩展性靠这个 per-CPU 计数器兜底。
> 但**读 `/proc/pid/status` 的 `VmRSS` 会遍历所有 CPU 求和**——不要在热路径上周期性读它。

---

## 3. `mm_users` vs `mm_count`：两个计数的精确语义

这是全书最容易含糊的一段。**权威解释是 Linus 1999 年的一封邮件**，
v6.6 把它原样放在 `Documentation/mm/active_mm.rst` 里：

> Basically, the new setup is:
>
> - we have "real address spaces" and "anonymous address spaces". [...]
> - "tsk->mm" points to the "real address space". For an anonymous process,
>   tsk->mm will be NULL [...]
> - however, we obviously need to keep track of which address space we
>   "stole" for such an anonymous user. For that, we have "tsk->active_mm" [...]
>
> To support all that, the "struct mm_struct" now has two counters: a
> "mm_users" counter that is how many "real address space users" there are,
> and a "mm_count" counter that is the number of "lazy" users (ie anonymous
> users) plus one if there are any real users.
>
> — Linus Torvalds, 1999-07-30

翻成操作定义：

| 计数 | 含义 | 增减函数 | 谁在 bump |
|------|------|----------|-----------|
| **`mm_users`** | 有多少个"真正使用用户空间的实体"（= 多少线程把 `tsk->mm` 指向它） | `mmget()` / `mmput()` | `fork` 的 `CLONE_VM`（`copy_mm()` 里 `mmget(oldmm)`）、ptrace、`/proc` 读者 |
| **`mm_count`** | `mm_struct` **这块内存本身** 的引用计数（含 lazy 用户） | `mmgrab()` / `mmdrop()` | `get_task_mm()`、内核线程借 `active_mm`、core dump |

**关键关系（Linus 原话）**：`mm_count = lazy 用户数 + (有真实用户 ? 1 : 0)`。

```
                    ┌──────────────────────────────┐
线程 A ─┐           │  mm_struct                   │
线程 B ─┼─ tsk->mm ─┤  mm_users = 3                 │
线程 C ─┘           │  mm_count = 1 (+ lazy 借用数) │
                    └──────────────────────────────┘
        ↑ 每个线程退出：mmput() → mm_users--

mm_users 降到 0：
    → mmput() 拆掉所有 VMA、释放页表、再 mmdrop() 一次
    → 若此时还有内核线程在借 active_mm（lazy 用户），mm_count 仍 > 0
      → mm_struct 继续存活，但已经是"僵尸 mm"（只有页表还在，VMA 已空）
    → 最后一个 lazy 用户被调度走 → mm_count 归 0 → __mmdrop() → free_mm()
```

> **⚠️ v6.6 的新变化**：文档开头有一条警告——
> 「the mm_count refcount **may no longer include** the "lazy" users on kernels with
> `CONFIG_MMU_LAZY_TLB_REFCOUNT=n`」。
> 所以**不要直接 `mmgrab()` 记 lazy 引用**，必须用包装函数（见下节）。

---

## 4. `active_mm` 与内核线程：四象限（源码注释逐字）

内核线程 `tsk->mm == NULL`，但它运行时 CPU 上必须装着某个页表（否则一进内核半区就 #PF）。
办法是**借用上一个用户进程的 mm**。切换逻辑的完整四象限写在 `kernel/sched/core.c:5335`：

```c
	/*
	 * kernel -> kernel   lazy + transfer active
	 *   user -> kernel   lazy + mmgrab_lazy_tlb() active
	 *
	 * kernel ->   user   switch + mmdrop_lazy_tlb() active
	 *   user ->   user   switch
	 *
	 * switch_mm_cid() needs to be updated if the barriers provided
	 * by context_switch() are modified.
	 */
	if (!next->mm) {                                // to kernel
		enter_lazy_tlb(prev->active_mm, next);
		next->active_mm = prev->active_mm;
		if (prev->mm)                           // from user
			mmgrab_lazy_tlb(prev->active_mm);
		else
			prev->active_mm = NULL;
	} else {                                        // to user
		membarrier_switch_mm(rq, prev->active_mm, next->mm);
		switch_mm_irqs_off(prev->active_mm, next->mm, next);
		lru_gen_use_mm(next->mm);
		if (!prev->mm) {                        // from kernel
			/* will mmdrop_lazy_tlb() in finish_task_switch(). */
			rq->prev_mm = prev->active_mm;
			prev->active_mm = NULL;
		}
	}
```

| prev → next | 动作 | 是否换 CR3 |
|-------------|------|-----------|
| **user → user** | `switch_mm_irqs_off()` | ✅ 换（除非 `prev->mm == next->mm`，即同线程组） |
| **user → kernel** | `enter_lazy_tlb()` + `mmgrab_lazy_tlb()` | ❌ **不换**（借着用） |
| **kernel → user** | `switch_mm_irqs_off()` + 稍后 `mmdrop_lazy_tlb()` | ✅ 换 |
| **kernel → kernel** | `enter_lazy_tlb()` + 直接传递 `active_mm` | ❌ 不换 |

### lazy 引用的两种记账模式

```c
/* include/linux/sched/mm.h:87 */
static inline void mmgrab_lazy_tlb(struct mm_struct *mm)
{
	if (IS_ENABLED(CONFIG_MMU_LAZY_TLB_REFCOUNT))
		mmgrab(mm);
}

static inline void mmdrop_lazy_tlb(struct mm_struct *mm)
{
	if (IS_ENABLED(CONFIG_MMU_LAZY_TLB_REFCOUNT)) {
		mmdrop(mm);
	} else {
		/* mmdrop_lazy_tlb must provide a full memory barrier ... */
		smp_mb();
	}
}
```

| 模式 | 行为 | 取舍 |
|------|------|------|
| `CONFIG_MMU_LAZY_TLB_REFCOUNT=y`（**x86 走这条**） | lazy 用户也 `mmgrab()`，即计入 `mm_count` | **省一次 IPI**，但 mm_struct 可能多活一会儿 |
| `CONFIG_MMU_LAZY_TLB_SHOOTDOWN=y` | 不计数；`__mmdrop()` 前 **IPI 所有可能借用的 CPU**，让它们切到 `init_mm` | **及时释放**，但每次销毁 mm 都有 IPI 开销 |

```c
/* arch/Kconfig:492 */
config MMU_LAZY_TLB_REFCOUNT
	def_bool y
	depends on !MMU_LAZY_TLB_SHOOTDOWN
```

> **HFT 视角**：内核线程频繁唤醒（网卡驱动、workqueue）时，
> `user → kernel` 的 lazy 借用**避免了 TLB flush 和 CR3 重载**。
> 这正是"HFT 机器上启用 `threadirqs` 会把中断线程化"的一个隐藏收益：
> 中断线程是内核线程，切进来不用换页表。

---

## 5. `mm_struct` 自身的生与死

```c
/* kernel/fork.c:3269 */
mm_size = sizeof(struct mm_struct) + cpumask_size() + mm_cid_size();
mm_cachep = kmem_cache_create_usercopy("mm_struct",
		mm_size, ARCH_MIN_MMSTRUCT_ALIGN,
		SLAB_HWCACHE_ALIGN|SLAB_PANIC|SLAB_ACCOUNT,
		offsetof(struct mm_struct, saved_auxv),
		sizeof_field(struct mm_struct, saved_auxv),
		NULL);

#define allocate_mm()	(kmem_cache_alloc(mm_cachep, GFP_KERNEL))
#define free_mm(mm)	(kmem_cache_free(mm_cachep, (mm)))
```

注意 `kmem_cache_create_usercopy` 的最后两个参数：
**只允许把 `saved_auxv` 这个字段拷贝到用户空间**，其余字段的 `copy_to_user` 会被
`CONFIG_HARDENED_USERCOPY` 拦下。（`/proc/pid/auxv` 就靠这个白名单工作。）

销毁路径：

```c
/* kernel/fork.c:910 */
void __mmdrop(struct mm_struct *mm)
{
	BUG_ON(mm == &init_mm);
	cleanup_lazy_tlbs(mm);              /* ← 若 SHOOTDOWN 模式，这里发 IPI */
	mm_free_pgd(mm);                    /* 释放页表 */
	destroy_context(mm);
	mmu_notifier_subscriptions_destroy(mm);
	check_mm(mm);
	put_user_ns(mm->user_ns);
	mm_pasid_drop(mm);
	mm_destroy_cid(mm);
	percpu_counter_destroy_many(mm->rss_stat, NR_MM_COUNTERS);
	free_mm(mm);
}
```

还有一条**异步销毁**路径（PREEMPT_RT 上关键）：

```c
/* include/linux/sched/mm.h:73 */
static inline void mmdrop_sched(struct mm_struct *mm)
{
	if (atomic_dec_and_test(&mm->mm_count))
		call_rcu(&mm->delayed_drop, __mmdrop_delayed);   /* 不在调度器热路径上做销毁 */
}
```

> `mmdrop_sched` / `mmput_async` 把"拆页表 + 释放 mm"挪到工作队列或 RCU 回调里，
> 避免在持 `rq->lock` 的调度器热路径上做 IPI 和内存回收。**这是 PREEMPT_RT 降低调度延迟的一环。**

`fork` 时的分支（`kernel/fork.c:1708`）：

```c
	if (clone_flags & CLONE_VM) {
		mmget(oldmm);        /* 线程：mm_users++，共享同一个 mm */
		mm = oldmm;
	} else {
		mm = dup_mm(tsk, current->mm);   /* 进程：整套页表 + VMA 复制 */
		if (!mm)
			return -ENOMEM;
	}
```

---

## 6. per-VMA lock（v6.4+）：缺页不再抢 `mmap_lock`

这是 Ch15 相关最重要的现代改进，**直接决定多线程缺页延迟**。

`mmap_lock` 是一把**整个地址空间的读写信号量**。红黑树时代，
**任何一次缺页都必须先拿 `mmap_lock` 读锁**——多线程同时缺页时，
这把锁本身成为可扩展性瓶颈（一个进程 64 个线程同时 fault，就 64 个读者抢一把 rwsem 的 cacheline）。

v6.4 引入 `CONFIG_PER_VMA_LOCK`：给每个 VMA 配一把自己的读写锁（`vma->vm_lock`），
缺页时**只锁那一个 VMA**：

```c
/* arch/x86/mm/fault.c:1354 —— 缺页入口的快路径 */
	if (!(flags & FAULT_FLAG_USER))
		goto lock_mmap;

	vma = lock_vma_under_rcu(mm, address);          /* ★ 只锁这一个 VMA，不碰 mmap_lock */
	if (!vma)
		goto lock_mmap;                              /* 拿不到就退回老路 */

	if (unlikely(access_error(error_code, vma))) {
		vma_end_read(vma);
		goto lock_mmap;
	}
	fault = handle_mm_fault(vma, address, flags | FAULT_FLAG_VMA_LOCK, regs);
	if (!(fault & (VM_FAULT_RETRY | VM_FAULT_COMPLETED)))
		vma_end_read(vma);

	if (!(fault & VM_FAULT_RETRY)) {
		count_vm_vma_lock_event(VMA_LOCK_SUCCESS);
		goto done;
	}
	count_vm_vma_lock_event(VMA_LOCK_RETRY);
	/* ... 失败才降级到 lock_mmap 拿 mmap_lock ... */
```

```c
/* mm/memory.c:5431 —— 靠 maple tree 的 RCU 能力实现无锁查找 */
struct vm_area_struct *lock_vma_under_rcu(struct mm_struct *mm, unsigned long address)
{
	MA_STATE(mas, &mm->mm_mt, address, address);
	struct vm_area_struct *vma;

	rcu_read_lock();
retry:
	vma = mas_walk(&mas);                    /* RCU 保护的树查找 */
	if (!vma) goto inval;
	if (!vma_start_read(vma)) goto inval;    /* 拿这个 VMA 的读锁（seqcount 实现） */
	/* 匿名 VMA 且没有 anon_vma → 退回慢路径（并发 mremap 的竞态） */
	if (unlikely(vma_is_anonymous(vma) && !vma->anon_vma)) goto inval_end_read;
	/* 锁上之后再验一次地址范围，因为 vm_start/vm_end 可能变过 */
	if (unlikely(address < vma->vm_start || address >= vma->vm_end)) goto inval_end_read;
	/* VMA 已被隔离（detached）→ 重试 */
	if (vma->detached) { vma_end_read(vma); count_vm_vma_lock_event(VMA_LOCK_MISS); goto retry; }
	rcu_read_unlock();
	return vma;
	/* ... */
}
```

**能成立的前提**就是 maple tree 的两个标志：

```c
/* include/linux/mm_types.h:929 */
#define MM_MT_FLAGS	(MT_FLAGS_ALLOC_RANGE | MT_FLAGS_LOCK_EXTERN | MT_FLAGS_USE_RCU)

/* kernel/fork.c:1260 */
	mt_init_flags(&mm->mm_mt, MM_MT_FLAGS);
	mt_set_external_lock(&mm->mm_mt, &mm->mmap_lock);
```

`MT_FLAGS_USE_RCU` → 树可以在 RCU 读锁下查找；
`MT_FLAGS_LOCK_EXTERN` → 写操作仍由 `mmap_lock` 保证互斥。

> **HFT 推论（重要）**：
> 多线程 HFT 进程里，**每线程独立 `mmap` 出来的 buffer 是独立 VMA** → 并发首次访问
> （缺页）**互不阻塞**。
> 反之，如果所有线程共用**一个大 VMA 里切出来的块**，首次访问会全部串行化到同一把 VMA 锁上。
> → **按线程切分 VMA，不只是权限隔离的理由，也是缺页可扩展性的理由。**

观测（`/proc/vmstat` 里有一组 event 计数器）：

```bash
grep vma_lock /proc/vmstat
# vma_lock_success 4283
# vma_lock_retry   12
# vma_lock_miss    3
# vma_lock_abort   0
```

---

## 7. 观测手段（v6.6 实证）

| 路径 | 内容 | 数据来源 |
|------|------|----------|
| **`/proc/pid/maps`** | 所有 VMA 区间 | 遍历 `mm_mt` |
| **`/proc/pid/smaps`** | 每 VMA 的 `Rss/Pss/Pss_Dirty/Pss_Anon/Pss_File/Pss_Shmem/AnonHugePages/SwapPss/THPeligible/VmFlags` | `fs/proc/task_mmu.c` |
| **`/proc/pid/pagemap`** | PFN（需 root）— 验证是否 huge / 是否 swap | — |
| **`/proc/pid/status`** | 见下表 | `fs/proc/task_mmu.c:60-77` |

`/proc/pid/status` 的内存字段**与 `mm_struct` 字段一一对应**（源码逐字）：

```
VmPeak:	hiwater_vm
VmSize:	mm->total_vm
VmLck:	mm->locked_vm          ← mlock 的，上线前必查这一项
VmPin:	atomic64_read(&mm->pinned_vm)
VmHWM:	hiwater_rss
VmRSS:	total_rss
RssAnon / RssFile / RssShmem
VmData:	mm->data_vm
VmStk:	mm->stack_vm
VmExe / VmLib
VmPTE:	mm_pgtables_bytes(mm) >> 10   ← 页表开销，大页能显著压这一项
VmSwap:	swap
```

`PR_SET_THP_DISABLE` 会置 `MMF_DISABLE_THP` 位（`include/linux/sched/coredump.h:73`），
**只影响本进程**，不用动全局 sysfs —— 见 15.7。

---

## 8. 与调度 / 切换（Ch 4）

| 事件 | `mm` 行为 |
|------|-----------|
| **`context_switch` → user** | `switch_mm_irqs_off()` → 写 CR3 → 若开了 PCID 则不 flush 全部 TLB |
| **同线程组切换** | `prev->mm == next->mm`，`switch_mm()` 里会跳过 CR3 重载 |
| **→ kernel 线程** | **不换页表**，接 `prev->active_mm` |
| **绑核 + 单进程** | 切换少 → **HFT 友好** |

**HFT：** 上线前 **`smaps` 确认 locked**、**`pagemap` 确认 2MB huge**；**意外 `mm_users` 泄漏** 少见，但 **子进程 `fork` 复制地址空间** 会 **COW 尖刺** — 热路径 **posix_spawn / vfork 策略** 或 **启动后不再 fork**。

---

→ [Ch 4 context_switch](../../chapter-04-process-scheduling/notes/section-4.5-抢占与上下文切换.md) · [Ch 3 线程共享 mm](../../chapter-03-process-management/) · [Ch 15.4 VMA 的树](./section-15.4-内存区域的链表与树.md) · [06 Gorman mm_struct](../../../06-linux-mm/chapter-04-process-address-space/)


> ↔ [ULK Ch9 §2 内存描述符](../../../16-linux-kernel-deep/chapter-09-process-address-space/notes/section-2-内存描述符.md)


<details>
<summary>自测题（点击展开）</summary>

**Q1.** mm_struct 和 task_struct 的关系？线程间共享 mm 吗？

<details><summary>答案</summary>

task_struct 包含 mm 指针（指向 mm_struct）。同一进程的线程共享同一个 mm_struct（clone 时设 CLONE_VM）。不同进程的 mm_struct 不同。`current->mm` 访问当前进程地址空间。内核线程没有 mm_struct（mm=NULL），使用上一个用户进程的页表（lazy TLB）。HFT 多线程共享行情内存就是利用同 mm。

**按 v6.6 修订/补充**：
- 「clone 时设 `CLONE_VM`」的**代码后果**是 `copy_mm()` 里走 `mmget(oldmm)` 分支
  （`kernel/fork.c:1723`），即 **`mm_users++` 且 `mm` 指针原样复用**；
  不走 `CLONE_VM` 则 `dup_mm()` 复制整套 VMA 和页表（并写保护以便 COW）。
- 「内核线程 mm == NULL」的**完整说法**：`tsk->mm == NULL` 但 `tsk->active_mm != NULL`，
  `active_mm` 是从上一个用户进程**借**来的。切走时归还。
- 补充：v6.1 起 `mm_struct` 里**已经没有 `mmap` 链表和 `mm_rb` 红黑树**了，
  统一为 `struct maple_tree mm_mt`（见本篇 §1）。

</details>


**Q2.** `mm_users` 和 `mm_count` 到底差在哪？为什么需要两个？

<details><summary>答案</summary>

用 Linus 1999 年邮件（`Documentation/mm/active_mm.rst` 原文收录）的定义：

- `mm_users` = **有多少"真实用户"**（多少个线程的 `tsk->mm` 指向它）
- `mm_count` = **lazy 用户数 + （有真实用户 ? 1 : 0）**

两者生命周期不同：

| 事件 | `mm_users` | `mm_count` |
|------|-----------|-----------|
| 新进程 `fork` | 1 | 1 |
| 线程创建（`CLONE_VM`） | **+1**（`mmget()`） | 不变 |
| `get_task_mm()`（ptrace/`/proc` 读者） | 不变 | **+1**（`mmgrab()`） |
| 内核线程借用 `active_mm` | 不变 | **+1**（x86 上，走 `mmgrab_lazy_tlb()`） |
| 线程退出 | −1（`mmput()`） | 不变 |
| `mm_users` 归 0 | — | **`mmput()` 会再 `mmdrop()` 一次** |

**"僵尸 mm" 现象**：一个进程的最后一个线程退出了（`mm_users → 0`，VMA 和页表被拆），
但还有内核线程在借它的 `active_mm`（`mm_count > 0`）→ `mm_struct` 继续存在，
直到那个内核线程被调度走才真正释放。

⚠️ **v6.6 注意**：`CONFIG_MMU_LAZY_TLB_SHOOTDOWN=y` 的架构上，
lazy 用户**不计入 `mm_count`**（`mmgrab_lazy_tlb()` 是空操作），
`__mmdrop()` 改为发 IPI 强制各 CPU 切到 `init_mm`。
所以**记 lazy 引用必须用 `mmgrab_lazy_tlb()` 包装，不能直接 `mmgrab()`**。

</details>


**Q3.** 内核线程没有用户地址空间，为什么切到它时不用换页表？换页表的代价是什么？

<details><summary>答案</summary>

因为**内核半区的页表项在所有进程里是同一份**（PGD 项从 `swapper_pg_dir` 模板复制）。
内核线程只访问内核地址，所以**只要 CPU 上装的是任何一个用户进程的页表，内核半区都能正常访问**。

于是调度器在 `user → kernel` 时**故意不换页表**，只做：

```c
	enter_lazy_tlb(prev->active_mm, next);
	next->active_mm = prev->active_mm;
	mmgrab_lazy_tlb(prev->active_mm);       /* 借用要记账 */
```

换页表（`switch_mm_irqs_off` → 写 CR3）的代价：

1. **写 CR3 本身**：几十个 cycle 的序列化指令；
2. **TLB 失效**：不开 PCID 时**整张 TLB 作废**，接下来几十次访存都是 miss；
   开了 PCID（ASID）后，不同地址空间的 TLB 项可以共存，只失效"全局"项；
3. **PCID 耗尽时要 flush**：x86 只有 4096 个 PCID，一轮用完就得批量失效。

所以「**kernel → kernel 不换、user → kernel 不换、只有涉及用户进程切换才换**」
这个 lazy 设计直接省掉了大量 TLB flush。

**HFT 推论**：内核线程（中断线程、`ksoftirqd`、workqueue）切进来**不产生 TLB 开销**。
把网卡中断线程化（`threadirqs`）或让 NAPI 在 `ksoftirqd` 里跑，
在 lazy TLB 这件事上是**净收益**。

</details>


**Q4.** 书上说 VMA 存在"链表 + 红黑树"里，这个说法现在还对吗？为什么换掉了？

<details><summary>答案</summary>

**不对了。** v6.1 起两个都被 **`struct maple_tree mm_mt`** 取代。

证据（多版本 `include/linux/mm_types.h` diff）：

| 版本 | 字段 |
|------|------|
| v6.0 | `struct vm_area_struct *mmap;` + `struct rb_root mm_rb;` |
| **v6.1** | `struct maple_tree mm_mt;` ← 断崖在这一版 |
| v6.6 | `struct maple_tree mm_mt;` |

**为什么换**：

1. **深度浅一个数量级**。maple 节点 256 字节（4 个 cache line），64 位下 16 个槽位。
   1000 个 VMA 时红黑树约 10 层（每层一次指针追踪 = 一次 cache miss），maple 只有 3 层。
2. **RCU 安全**（`MT_FLAGS_USE_RCU`）。红黑树不支持无锁读，maple 支持
   → 这是 per-VMA lock（v6.4+）能成立的前提，缺页不再需要 `mmap_lock`。
3. **插入只维护一份结构**。以前要同时维护链表（供 `for_each_vma` 顺序遍历）
   和红黑树（供 `find_vma` 查找），现在一棵树同时支持两种遍历
   （`mas_find()` 按序遍历，`mt_find()` 按地址查找）。
4. **arange 节点记录区间最大空隙**，找空闲地址不用再靠 `mm->free_area_cache` 的启发式缓存。

**对 API 的影响**：

```c
find_vma(mm, addr)              →  mt_find(&mm->mm_mt, &index, ULONG_MAX)
find_vma_intersection(mm,s,e)   →  mt_find(&mm->mm_mt, &index, e - 1)
vma_lookup(mm, addr)            →  mtree_load(&mm->mm_mt, addr)
for_each_vma(vmi, vma, max)     →  mas_find() 游标
插入                             →  vma_iter_store(&vmi, vma)（内部 mas_store）
```

</details>


**Q5.** 多线程程序首次访问各自 `mmap` 出来的内存，会互相阻塞吗？

<details><summary>答案</summary>

**分情况，取决于内核版本和 VMA 是否独立。**

**v6.4 以前**：会。所有缺页都要先拿 `mmap_lock` 的**读锁**。
读锁理论上可共享，但 `mmap_lock` 是**一把 rwsem**，所有读者都要原子改同一个 `count` 字段
→ **cacheline 在核间弹跳**，64 线程同时缺页时这把锁本身成为瓶颈。

**v6.4+ 且 `CONFIG_PER_VMA_LOCK=y`（默认开）**：不会，前提是**各自 VMA 独立**。

快路径（`arch/x86/mm/fault.c:1354`）：
```
#PF → lock_vma_under_rcu(mm, addr)       /* RCU 读锁下用 mas_walk 找 VMA */
      → vma_start_read(vma)              /* 只锁这一个 VMA（seqcount 实现） */
      → handle_mm_fault(..., FAULT_FLAG_VMA_LOCK)
      → 完全不碰 mmap_lock
```

成功/失败可从 `/proc/vmstat` 观测：
```bash
grep vma_lock /proc/vmstat
# vma_lock_success / vma_lock_retry / vma_lock_miss / vma_lock_abort
```

**拿不到 VMA 锁的情况**（会退回 `mmap_lock` 慢路径）：
- 匿名 VMA 且 `vma->anon_vma == NULL`（并发 `mremap(MREMAP_DONTUNMAP)` 的竞态）；
- VMA 是 `detached` 状态（munmap 进行中）；
- 需要合并 `anon_vma`、需要分配新页表层级等"结构性"操作返回 `VM_FAULT_RETRY`。

**HFT 设计推论**：
- ✅ 每线程独立 `mmap` 一个 buffer → 独立 VMA → 并发缺页不串行；
- ❌ 所有线程共用**一个大 VMA 里切出来的块** → 并发首次访问全挤在同一把 VMA 锁上；
- 无论哪种，**都要在启动阶段预取页**（`MAP_POPULATE` / `MADV_POPULATE_WRITE`），
  把缺页彻底挪出盘中路径。

</details>

</details>
---
