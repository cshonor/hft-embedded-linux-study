## ⑦ 页表 · Page Tables

程序使用 **虚拟地址（VA）**；CPU 与 DMA（经 IOMMU）访问 **物理地址（PA）** — **页表** 保存 **VA→PFN** 映射与 **R/W/X** 权限。

> **本篇分工**：实体书 Ch15.7 覆盖了页表的基础概念（三级模型、PTE 位、TLB）。
> 本篇**不复述这些定义**，只做三件事：
> ① 用 **v6.6 源码**给出 x86_64 页表的**精确常量与真实位布局**（书上"三级模型"在 x86_64 上
> 实际是 **4 级或 5 级**，而且**是运行时可变的**）；
> ② 拆开 **PCID / TLB flush 的真实实现**（书上只说"PCID 让切换少 flush"，但真相是
> **Linux 只给每个 CPU 分了 6 个 ASID 槽**，硬件有 4096 个）；
> ③ 给 **可复现的量测方法**，不给拍脑袋的 ns 数字。
>
> 所有常量均核对自缓存的 v6.6 源码，行号可查。

---

## 1. ⚠️ 订正一：x86_64 是 **4 级 / 5 级**，不是三级

书上的三级模型（PGD → PMD → PTE）是 **i386 PAE 时代的简化**。x86_64 上：

| 配置项 | 编译 4 级 | 编译 5 级（`CONFIG_X86_5LEVEL=y`） |
|--------|-----------|-----------------------------------|
| `PGDIR_SHIFT` | `39`（**常量**） | `pgdir_shift`（**运行时变量** 39 或 48） |
| `PTRS_PER_PGD` | 512 | 512 |
| `P4D_SHIFT` | —（折叠） | `39` |
| `PTRS_PER_P4D` | 1（折叠） | `ptrs_per_p4d`（**运行时变量** 1 或 512） |
| `MAX_PTRS_PER_P4D` | **1** | **512** |
| `PUD_SHIFT` / `PTRS_PER_PUD` | 30 / 512 | 30 / 512 |
| `PMD_SHIFT` / `PTRS_PER_PMD` | 21 / 512 | 21 / 512 |
| `PTRS_PER_PTE` | 512 | 512 |

```c
/* arch/x86/include/asm/pgtable_64_types.h:53-97 */

#ifdef CONFIG_X86_5LEVEL
#define PGDIR_SHIFT	pgdir_shift
#define PTRS_PER_PGD	512

/* 4th level page in 5-level paging case */
#define P4D_SHIFT		39
#define MAX_PTRS_PER_P4D	512
#define PTRS_PER_P4D		ptrs_per_p4d
#define P4D_SIZE		(_AC(1, UL) << P4D_SHIFT)
#define P4D_MASK		(~(P4D_SIZE - 1))

#define MAX_POSSIBLE_PHYSMEM_BITS	52

#else /* CONFIG_X86_5LEVEL */

#define PGDIR_SHIFT		39
#define PTRS_PER_PGD		512
#define MAX_PTRS_PER_P4D	1

#endif /* CONFIG_X86_5LEVEL */

#define PUD_SHIFT	30
#define PTRS_PER_PUD	512
#define PMD_SHIFT	21
#define PTRS_PER_PMD	512
#define PTRS_PER_PTE	512
```

### 1.1 "P4D 折叠"：一套代码支持两种层级

4 级编译时**不会**到处写 `#ifdef`。内核的做法是引入一个**恒等层** `P4D`，
编译 4 级时把它折叠进 PGD（`include/asm-generic/pgtable-nop4d.h`）：

```c
#define __PAGETABLE_P4D_FOLDED 1
typedef struct { pgd_t pgd; } p4d_t;

#define P4D_SHIFT		PGDIR_SHIFT
#define PTRS_PER_P4D		1

static inline int pgd_none(pgd_t pgd)		{ return 0; }
static inline int pgd_bad(pgd_t pgd)		{ return 0; }
static inline int pgd_present(pgd_t pgd)	{ return 1; }
static inline void pgd_clear(pgd_t *pgd)	{ }
#define pgd_populate(mm, pgd, p4d)		do { } while (0)

/* p4d_offset() 直接把 pgd 指针转型返回 —— 零开销 */
static inline p4d_t *p4d_offset(pgd_t *pgd, unsigned long address)
{
	return (p4d_t *)pgd;
}
```

于是 `p4d_offset()` 在 4 级下**编译成一条赋值**，多出来的一级**不产生任何运行时代价**。
这是内核"用类型系统吸收架构差异"的典型手法：
`pgd → p4d → pud → pmd → pte` 五级 API **永远写全**，折叠层在各自的 `nopXX.h` 里被吃掉。

### 1.2 地址切分

**4 级（48-bit VA，`task_size = 128TB`）**

```
 63                47 46      38 37      29 28      20 19      11 10        0
┌──────────────────┬──────────┬──────────┬──────────┬──────────┬───────────┐
│  sign extension  │ PGD/PML4 │   PUD    │   PMD    │   PTE    │  offset   │
│   (canonical)    │  9 bits  │  9 bits  │  9 bits  │  9 bits  │  12 bits  │
└──────────────────┴──────────┴──────────┴──────────┴──────────┴───────────┘
                    bit 47-39  bit 38-30  bit 29-21  bit 20-12  bit 11-0
                     =512 项    =512 项    =512 项    =512 项    =4KB
                     覆盖 512G  覆盖 1G    覆盖 2M    覆盖 4K
```

**5 级（57-bit VA，`task_size = 64PB`）**

```
 63              56 55      47 46      38 37      29 28      20 19       0
┌─────────────────┬──────────┬──────────┬──────────┬──────────┬──────────┐
│ sign extension  │   PGD    │   P4D    │   PUD    │   PMD    │   PTE    │
│   (canonical)   │  9 bits  │  9 bits  │  9 bits  │  9 bits  │  9 bits  │
└─────────────────┴──────────┴──────────┴──────────┴──────────┴──────────┘
                   bit 56-48  bit 47-39  bit 38-30  bit 29-21  bit 20-12
                                         ↑ 原来的 4 级整体"下沉"一层
```

⚠️ **5 级的代价是真实的**：每次 page walk 多一次内存访问。
只有在地址空间 > 128TB 或需要更强 ASLR 时才值。可以用内核参数 `no5lvl` 关掉。

---

## 2. 五级页表是**运行时**开关，不是纯编译期常量

这是最容易写错的地方。`PGDIR_SHIFT` 和 `PTRS_PER_P4D` 在 5 级编译下是**变量**：

```c
/* arch/x86/include/asm/pgtable_64_types.h:47-48 */
extern unsigned int pgdir_shift;
extern unsigned int ptrs_per_p4d;
```

判定函数有两套实现，取决于是否在早期引导代码里：

```c
/* arch/x86/include/asm/pgtable_64_types.h:21-43 */
#ifdef CONFIG_X86_5LEVEL
extern unsigned int __pgtable_l5_enabled;

#ifdef USE_EARLY_PGTABLE_L5
/*
 * cpu_feature_enabled() is not available in early boot code.
 * Use variable instead.
 */
static inline bool pgtable_l5_enabled(void)
{
	return __pgtable_l5_enabled;
}
#else
#define pgtable_l5_enabled() cpu_feature_enabled(X86_FEATURE_LA57)
#endif
#else
#define pgtable_l5_enabled() 0
#endif
```

| 阶段 | 判定方式 | 原因 |
|------|----------|------|
| 早期引导（`USE_EARLY_PGTABLE_L5`） | 读变量 `__pgtable_l5_enabled` | `cpu_feature_enabled()` 尚未初始化 |
| 引导完成后 | `cpu_feature_enabled(X86_FEATURE_LA57)` | CPUID 可用，走 static key，`alternative` 打补丁 |

> 与 15.1 讲的 `task_size_max()` 是同一套思路：
> 用 `alternative_io` / static key 把"启动时决定一次"的事**变成零运行时分支**。

---

## 3. PTE 位布局全表（v6.6 逐字核对）

```c
/* arch/x86/include/asm/pgtable_types.h:10-42 */
#define _PAGE_BIT_PRESENT	0	/* is present */
#define _PAGE_BIT_RW		1	/* writeable */
#define _PAGE_BIT_USER		2	/* userspace addressable */
#define _PAGE_BIT_PWT		3	/* page write through */
#define _PAGE_BIT_PCD		4	/* page cache disabled */
#define _PAGE_BIT_ACCESSED	5	/* was accessed (raised by CPU) */
#define _PAGE_BIT_DIRTY		6	/* was written to (raised by CPU) */
#define _PAGE_BIT_PSE		7	/* 4 MB (or 2MB) page */
#define _PAGE_BIT_PAT		7	/* on 4KB pages */
#define _PAGE_BIT_GLOBAL	8	/* Global TLB entry PPro+ */
#define _PAGE_BIT_SOFTW1	9	/* available for programmer */
#define _PAGE_BIT_SOFTW2	10	/* " */
#define _PAGE_BIT_SOFTW3	11	/* " */
#define _PAGE_BIT_PAT_LARGE	12	/* On 2MB or 1GB pages */
#define _PAGE_BIT_SOFTW4	57	/* available for programmer */
#define _PAGE_BIT_SOFTW5	58	/* available for programmer */
#define _PAGE_BIT_PKEY_BIT0	59	/* Protection Keys, bit 1/4 */
#define _PAGE_BIT_PKEY_BIT1	60	/* Protection Keys, bit 2/4 */
#define _PAGE_BIT_PKEY_BIT2	61	/* Protection Keys, bit 3/4 */
#define _PAGE_BIT_PKEY_BIT3	62	/* Protection Keys, bit 4/4 */
#define _PAGE_BIT_NX		63	/* No execute: only valid after cpuid check */
```

| bit | 硬件名 | Linux 语义 | 谁置位 |
|-----|--------|-----------|--------|
| **0** | Present | 页在内存；0 → **#PF** | 内核 |
| **1** | R/W | 可写 | 内核 |
| **2** | U/S | 用户态可访问 | 内核 |
| 3 | PWT | 页级写穿透（缓存策略） | 内核 |
| 4 | PCD | 页级 Cache 禁用 | 内核 |
| **5** | Accessed | 被读或写过 | **CPU 硬件置位**，内核清除 |
| **6** | Dirty | 被写过 | **CPU 硬件置位**，内核清除 |
| **7** | ⚠️ **PSE ‖ PAT** | 4KB 页：PAT；2MB/1GB 页：**PSE（大页）** | 内核 |
| **8** | ⚠️ **GLOBAL ‖ PROTNONE** | 正常：TLB 全局项（切换 CR3 不失效）<br>`PROT_NONE`：**内核借来表示"不可访问"** | 内核 |
| 9 | SOFTW1 | `_PAGE_SPECIAL` / `_PAGE_CPA_TEST` | 内核 |
| 10 | SOFTW2 | `_PAGE_UFFD_WP`（userfaultfd 写保护） | 内核 |
| 11 | SOFTW3 | `_PAGE_SOFT_DIRTY`（软件脏页跟踪） | 内核 |
| 12 | PAT_LARGE | 2MB/1GB 页的 PAT | 内核 |
| **13–51** | — | **PFN（物理页帧号）** | 内核 |
| 52–56 | — | 未使用 | — |
| 57 | SOFTW4 | `_PAGE_DEVMAP`（`ZONE_DEVICE` 设备内存） | 内核 |
| 58 | SOFTW5 | `_PAGE_SAVED_DIRTY` | 内核 |
| 59–62 | — | **Protection Key**（4 位，16 把钥匙） | 内核 |
| **63** | NX | 不可执行（DEP） | 内核 |

`PFN` 的位置由 `PFN_PTE_SHIFT` 定义，就是页内偏移的位数：

```c
/* arch/x86/include/asm/pgtable.h:221 */
#define PFN_PTE_SHIFT	PAGE_SHIFT
/* arch/x86/include/asm/pgtable_types.h:284,290 */
#define PTE_PFN_MASK		((pteval_t)PHYSICAL_PAGE_MASK)
#define PTE_FLAGS_MASK		(~PTE_PFN_MASK)
```

### 3.1 ⚠️ 三个"一位两用"陷阱

| 位 | 用法 A | 用法 B | 怎么区分 |
|----|--------|--------|----------|
| **7** | PAT（4KB 页的缓存策略位） | PSE（这一级直接映射 2MB/1GB） | 看这一项在**哪一级**表：PTE 里是 PAT，PMD/PUD 里是 PSE |
| **8** | GLOBAL（TLB 全局，切 CR3 不失效） | **PROTNONE**（见第 4 节） | 看 bit 0：Present=0 时 bit 8 读作 PROTNONE |
| **9** | SPECIAL | CPA_TEST（change_page_attr 自检） | 不同子系统复用，靠上下文 |

**后果**：改动 PTE 权限时，不是所有位变化都需要 flush TLB。v6.6 把这件事写成了显式规则：

```c
/* arch/x86/include/asm/tlbflush.h:280-305 */
static inline bool pte_flags_need_flush(unsigned long oldflags,
					unsigned long newflags,
					bool ignore_access)
{
	/*
	 * Flags that require a flush when cleared but not when they are set.
	 * Only include flags that would not trigger spurious page-faults.
	 * Non-present entries are not cached. Hardware would set the
	 * dirty/access bit if needed without a fault.
	 */
	const pteval_t flush_on_clear = _PAGE_DIRTY | _PAGE_PRESENT |
					_PAGE_ACCESSED;
	const pteval_t software_flags = _PAGE_SOFTW1 | _PAGE_SOFTW2 |
					_PAGE_SOFTW3 | _PAGE_SOFTW4 |
					_PAGE_SAVED_DIRTY;
	const pteval_t flush_on_change = _PAGE_RW | _PAGE_USER | _PAGE_PWT |
			  _PAGE_PCD | _PAGE_PSE | _PAGE_GLOBAL | _PAGE_PAT |
			  _PAGE_PAT_LARGE | _PAGE_PKEY_BIT0 | _PAGE_PKEY_BIT1 |
			  _PAGE_PKEY_BIT2 | _PAGE_PKEY_BIT3 | _PAGE_NX;
	unsigned long diff = oldflags ^ newflags;

	BUILD_BUG_ON(flush_on_clear & software_flags);
	BUILD_BUG_ON(flush_on_clear & flush_on_change);
	BUILD_BUG_ON(flush_on_change & software_flags);

	/* Ignore software flags */
	diff &= ~software_flags;
	if (ignore_access)
		diff &= ~_PAGE_ACCESSED;
	...
}
```

三类标志，语义完全不同：

| 类别 | 包含 | flush 条件 |
|------|------|-----------|
| **软件位**（永不 flush） | SOFTW1/2/3/4、SAVED_DIRTY | 改了不用 flush —— MMU 不关心 |
| **只在**清除**时 flush** | DIRTY、PRESENT、ACCESSED | 置位不用 flush（硬件会自己做，或本来就没缓存）；清零必须 flush |
| **任何变化都 flush** | RW、USER、PWT、PCD、PSE、GLOBAL、PAT、PAT_LARGE、PKEY×4、NX | 只要变了就 flush |

> **HFT 含义**：`mprotect()` 改权限属于"变化即 flush"，必然触发 TLB shootdown。
> 而 `MADV_DONTNEED` 之类清 Present 属于"清除时 flush"。
> 所以**别在热路径上改内存权限**；反过来，靠软件位做标记（如 uffd-wp、soft-dirty）是**免 flush** 的。

---

## 4. ⚠️ 订正二：`PROT_NONE` 的 PTE 是 **present** 的

这是 x86 上非常反直觉的一个 hack。源码里写得很直白：

```c
/* arch/x86/include/asm/pgtable_types.h:44-47 */
/* If _PAGE_BIT_PRESENT is clear, we use these: */
/* - if the user mapped it with PROT_NONE; pte_present gives true */
#define _PAGE_BIT_PROTNONE	_PAGE_BIT_GLOBAL
```

于是：

```c
/* arch/x86/include/asm/pgtable.h:966 */
static inline int pte_present(pte_t a)
{
	return pte_flags(a) & (_PAGE_PRESENT | _PAGE_PROTNONE);
}
```

`PROT_NONE` 的页，其 PTE 长这样（`Present=0, RW=0, User=0, Accessed=1, GLOBAL=1`）：

```c
/* arch/x86/include/asm/pgtable_types.h:203 */
#define PAGE_NONE	     __pg(   0|   0|   0|___A|   0|   0|   0|___G)
/*                               ^^^^  ^^^^  ^^^^  ^^^^
                                 P=0   RW=0  USR=0  A=1  ...  G(=PROTNONE)=1 */
```

**为什么要这么绕？**

| 方案 | Present | 后果 |
|------|---------|------|
| 朴素做法 | 0 | 访问 → #PF；但内核**无法区分**"这一页还没映射（该去分配）"和"这一页是 PROT_NONE（该给 SIGSEGV）" |
| x86 hack | 0 + bit8=1 | 访问 → #PF；`do_page_fault` 里 `pte_present()` 返回 **true** → 走到"权限错误"分支 → `SIGSEGV` |

配套的判定函数同样要小心：

```c
/* arch/x86/include/asm/pgtable.h:979 —— 用于 mmu_notifier / KSM 等路径 */
static inline bool pte_accessible(struct mm_struct *mm, pte_t a)
{
	if (pte_flags(a) & _PAGE_PRESENT)
		return true;
	if ((pte_flags(a) & _PAGE_PROTNONE) &&
			atomic_read(&mm->tlb_flush_pending))
		return true;
	return false;
}

/* arch/x86/include/asm/pgtable.h:1639 —— 真正的权限检查，只看 Present，不认 PROTNONE */
static inline bool __pte_access_permitted(unsigned long pteval, bool write)
{
	unsigned long need_pte_bits = _PAGE_PRESENT|_PAGE_USER;
	if (write)
		need_pte_bits |= _PAGE_RW;
	if ((pteval & need_pte_bits) != need_pte_bits)
		return 0;
	return __pkru_allows_pkey(pte_flags_pkey(pteval), write);
}
```

PMD 层还有一个额外的坑 —— 多考虑一个 `PSE` 位：

```c
/* arch/x86/include/asm/pgtable.h:991 */
static inline int pmd_present(pmd_t pmd)
{
	/*
	 * Checking for _PAGE_PSE is needed too because
	 * split_huge_page will temporarily clear the present bit (but
	 * the _PAGE_PSE flag will remain set at all times while the
	 * _PAGE_PRESENT bit is clear).
	 */
	return pmd_flags(pmd) & (_PAGE_PRESENT | _PAGE_PROTNONE | _PAGE_PSE);
}
```

> 拆分 THP（`split_huge_page`）时会**先清 Present、保留 PSE**，
> 这中间窗口期的 PMD 必须仍然被认为是 present，否则并发 walk 会踩空。

**HFT 含义**：用 `mprotect(PROT_NONE)` 做 guard page / 内存池边界保护时，
**PTE 页不会被释放**，页表和 TLB 条目照样占着。它换来的是"越界立刻 SIGSEGV"的确定性，
这笔交易在**热路径之外的边界检查**上是划算的；但不要指望它能省内存。

---

## 5. 页表本身的分配与"内核半区共享"

### 5.1 只拷贝内核半区 —— 这就是"共享"的实现

```c
/* arch/x86/mm/pgtable.c:123 */
static void pgd_ctor(struct mm_struct *mm, pgd_t *pgd)
{
	/* If the pgd points to a shared pagetable level (either the
	   ptes in non-PAE, or shared PMD in PAE), then just copy the
	   references from swapper_pg_dir. */
	if (CONFIG_PGTABLE_LEVELS == 2 ||
	    (CONFIG_PGTABLE_LEVELS == 3 && SHARED_KERNEL_PMD) ||
	    CONFIG_PGTABLE_LEVELS >= 4) {
		clone_pgd_range(pgd + KERNEL_PGD_BOUNDARY,
				swapper_pg_dir + KERNEL_PGD_BOUNDARY,
				KERNEL_PGD_PTRS);
	}

	/* list required to sync kernel mapping updates */
	if (!SHARED_KERNEL_PMD) {
		pgd_set_mm(pgd, mm);
		pgd_list_add(pgd);
	}
}
```

`fork()` 时**只拷 `KERNEL_PGD_PTRS` 个内核项**（从 `swapper_pg_dir` 抄），用户半区一开始是空的。
这正是 15.1 里"所有进程的页表内核部分相同"的机制来源 —— 不是什么特殊魔法，就是这段 memcpy。

### 5.2 `pgd_alloc` 会**预分配** PMD

```c
/* arch/x86/mm/pgtable.c:430 */
pgd_t *pgd_alloc(struct mm_struct *mm)
{
	pgd_t *pgd;
	pmd_t *u_pmds[MAX_PREALLOCATED_USER_PMDS];
	pmd_t *pmds[MAX_PREALLOCATED_PMDS];

	pgd = _pgd_alloc();
	if (pgd == NULL)
		goto out;

	mm->pgd = pgd;

	if (sizeof(pmds) != 0 &&
			preallocate_pmds(mm, pmds, PREALLOCATED_PMDS) != 0)
		goto out_free_pgd;

	if (sizeof(u_pmds) != 0 &&
			preallocate_pmds(mm, u_pmds, PREALLOCATED_USER_PMDS) != 0)
		goto out_free_pmds;
	...
	spin_lock(&pgd_lock);
	pgd_ctor(mm, pgd);
```

预分配数量的定义有一个**和 PTI 绑定的细节**：

```c
/* arch/x86/mm/pgtable.c:176-186 */
#define PREALLOCATED_PMDS	UNSHARED_PTRS_PER_PGD
#define MAX_PREALLOCATED_PMDS	MAX_UNSHARED_PTRS_PER_PGD

#define PREALLOCATED_USER_PMDS	 (boot_cpu_has(X86_FEATURE_PTI) ? \
					KERNEL_PGD_PTRS : 0)
#define MAX_PREALLOCATED_USER_PMDS KERNEL_PGD_PTRS
```

> ⚠️ **只有开启 PTI 时才预分配用户 PMD**。因为 PTI 下每个 mm 有两套页表，
> 用户那套需要在 `pgd_alloc` 时就把结构备好；不开 PTI 时这一层是懒建立的。

**动机**：页表页的分配在缺页路径上发生，而缺页路径可能在原子上下文（不能睡眠、不能 `GFP_KERNEL`）。
预分配把这件事挪到进程创建时，是**把不确定性前移**的典型手法 —— 与 HFT 里"预热所有内存"
的思路完全一致。

### 5.3 页表内存开销的量级

| 映射量 | PTE 页 | 上级表 | 页表总开销 | 占比 |
|--------|--------|--------|-----------|------|
| 4KB | 1 页 | PGD+PUD+PMD 各 1 页 | ~16KB | 400% 😅 |
| 2MB | 1 页（512 PTE） | 3 页 | ~16KB | 0.8% |
| 1GB | 512 页 | PGD+PUD 各 1 页 | ~2MB + 8KB | ~0.2% |
| 1TB | 512K 页 | + 1 页 PUD + PMD 表 512 页 | ~2GB + 2MB | ~0.2% |

**渐进值**：映射足够大时，页表开销收敛到 **1/512 ≈ 0.2%**（每 512 个 PTE 页需要 1 个 PMD 页，再往上是 1/512 的 1/512，可忽略）。

用大页可以把这个比例压到 **1/512² ≈ 0.0004%**，同时**页表项本身也更 cache 友好**。

```bash
# 全系统页表占用（PageTables 在 /proc/meminfo）
grep PageTables /proc/meminfo

# 单进程（VmPTE 在 v6.6 的 /proc/self/status 里）
grep -E "VmPTE|VmPeak|VmSize|RssFile|RssAnon" /proc/self/status
```

---

## 6. ⚠️ 订正三：PCID 有 4096 个，但 Linux 每 CPU 只用 **6 个**

书上只说"PCID 让切换 mm 时少 flush TLB"。真实实现比这有意思得多，而且和 RISC 的 ASID 方案**根本不是一回事**。

### 6.1 三套编号空间

```c
/* arch/x86/mm/tlb.c:60-85 —— 注释逐字 */
/*
 * The x86 feature is called PCID (Process Context IDentifier). It is similar
 * to what is traditionally called ASID on the RISC processors.
 *
 * We don't use the traditional ASID implementation, where each process/mm gets
 * its own ASID and flush/restart when we run out of ASID space.
 *
 * Instead we have a small per-cpu array of ASIDs and cache the last few mm's
 * that came by on this CPU, allowing cheaper switch_mm between processes on
 * this CPU.
 *
 * We end up with different spaces for different things. To avoid confusion we
 * use different names for each of them:
 *
 * ASID  - [0, TLB_NR_DYN_ASIDS-1]
 *         the canonical identifier for an mm
 *
 * kPCID - [1, TLB_NR_DYN_ASIDS]
 *         the value we write into the PCID part of CR3; corresponds to the
 *         ASID+1, because PCID 0 is special.
 *
 * uPCID - [2048 + 1, 2048 + TLB_NR_DYN_ASIDS]
 *         for KPTI each mm has two address spaces and thus needs two
 *         PCID values, but we can still do with a single ASID denomination
 *         for each mm. Corresponds to kPCID + 2048.
 *
 */

/* There are 12 bits of space for ASIDS in CR3 */
#define CR3_HW_ASID_BITS		12
```

**数量对照表**（这是最容易写错的地方）：

| 名字 | 取值范围 | 数量 | 说明 |
|------|----------|------|------|
| 硬件 PCID 位数 | `CR3_HW_ASID_BITS = 12` | **4096** | `CR3_PCID_MASK = 0xFFF`（`processor-flags.h:41`） |
| `MAX_ASID_AVAILABLE` | `((1 << CR3_AVAIL_PCID_BITS) - 2)` | **2046** | 开 PTI 时 `CR3_AVAIL_PCID_BITS = 12 - 1 = 11` |
| **`TLB_NR_DYN_ASIDS`** | `[0, 5]` | **6** | ⭐ **Linux 实际使用的数量** |

```c
/* arch/x86/include/asm/tlbflush.h:58-65 */
#ifndef MODULE
/*
 * 6 because 6 should be plenty and struct tlb_state will fit in two cache
 * lines.
 */
#define TLB_NR_DYN_ASIDS	6

struct tlb_context {
	u64 ctx_id;
	u64 tlb_gen;
};
```

选择 6 的理由写在注释里：**"6 should be plenty"**，且能让 `struct tlb_state` 塞进
**两条 cache line**。这是典型的"用容量换 cache 局部性" —— 每 CPU 的 ASID 表是
热得发烫的数据，宁可小也要快。

### 6.2 为什么 kPCID = ASID + 1

```c
/* arch/x86/mm/tlb.c:113-145 */
static inline u16 kern_pcid(u16 asid)
{
	VM_WARN_ON_ONCE(asid > MAX_ASID_AVAILABLE);
#ifdef CONFIG_PAGE_TABLE_ISOLATION
	/* Make sure that the dynamic ASID space does not conflict with the
	 * bit we are using to switch between user and kernel ASIDs. */
	BUILD_BUG_ON(TLB_NR_DYN_ASIDS >= (1 << X86_CR3_PTI_PCID_USER_BIT));
	/* The ASID being passed in here should have respected the
	 * MAX_ASID_AVAILABLE and thus never have the switch bit set. */
	VM_WARN_ON_ONCE(asid & (1 << X86_CR3_PTI_PCID_USER_BIT));
#endif
	/*
	 * ...
	 * If PCID is on, ASID-aware code paths put the ASID+1 into the
	 * PCID bits.  This serves two purposes.  It prevents a nasty
	 * situation in which PCID-unaware code saves CR3, loads some other
	 * value (with PCID == 0), and then restores CR3, thus corrupting
	 * the TLB for ASID 0 if the saved ASID was nonzero.  It also means
	 * that any bugs involving loading a PCID-enabled CR3 with
	 * CR4.PCIDE off will trigger deterministically.
	 */
	return asid + 1;
}
```

**两个理由**：
1. 防止"PCID-unaware 代码保存 CR3 → 载入 PCID=0 的值 → 恢复 CR3"污染 ASID 0 的 TLB
2. 让"没开 CR4.PCIDE 却载入带 PCID 的 CR3"这类 bug **确定性触发**，而不是随机踩坑

### 6.3 `CR3_NOFLUSH`：PCID 省下 flush 的关键

```c
/* arch/x86/include/asm/processor-flags.h:42 */
#define CR3_NOFLUSH	BIT_ULL(63)

/* arch/x86/mm/tlb.c:158-181 */
static inline unsigned long build_cr3(pgd_t *pgd, u16 asid, unsigned long lam)
{
	unsigned long cr3 = __sme_pa(pgd) | lam;
	if (static_cpu_has(X86_FEATURE_PCID)) {
		VM_WARN_ON_ONCE(asid > MAX_ASID_AVAILABLE);
		cr3 |= kern_pcid(asid);
	} else {
		VM_WARN_ON_ONCE(asid != 0);
	}
	return cr3;
}

static inline unsigned long build_cr3_noflush(pgd_t *pgd, u16 asid,
					      unsigned long lam)
{
	VM_WARN_ON_ONCE(!boot_cpu_has(X86_FEATURE_PCID));
	return build_cr3(pgd, asid, lam) | CR3_NOFLUSH;
}
```

**CR3 的 bit 63 = NOFLUSH**。写上它，CPU 加载 CR3 时**不清 TLB**，
而是靠 CR3 里的 PCID 字段把新旧条目分开。这就是 `write CR3` 不再等于"全清 TLB"的原因。

### 6.4 ASID 分配算法：线性扫描 + 轮转淘汰

```c
/* arch/x86/mm/tlb.c:219-253 */
static void choose_new_asid(struct mm_struct *next, u64 next_tlb_gen,
			    u16 *new_asid, bool *need_flush)
{
	u16 asid;

	if (!static_cpu_has(X86_FEATURE_PCID)) {
		*new_asid = 0;
		*need_flush = true;
		return;
	}

	if (this_cpu_read(cpu_tlbstate.invalidate_other))
		clear_asid_other();

	for (asid = 0; asid < TLB_NR_DYN_ASIDS; asid++) {
		if (this_cpu_read(cpu_tlbstate.ctxs[asid].ctx_id) !=
		    next->context.ctx_id)
			continue;

		*new_asid = asid;
		*need_flush = (this_cpu_read(cpu_tlbstate.ctxs[asid].tlb_gen) <
			       next_tlb_gen);
		return;
	}

	/*
	 * We don't currently own an ASID slot on this CPU.
	 * Allocate a slot.
	 */
	*new_asid = this_cpu_add_return(cpu_tlbstate.next_asid, 1) - 1;
	if (*new_asid >= TLB_NR_DYN_ASIDS) {
		*new_asid = 0;
		this_cpu_write(cpu_tlbstate.next_asid, 1);
	}
	*need_flush = true;
}
```

**三步**：
1. **线性扫 6 个槽**，找 `ctx_id` 匹配当前 mm 的 → 命中就不用换 ASID；
   但还要比 `tlb_gen`：如果 mm 的 generation 比槽里记录的新，说明**这期间有人改了页表** → 必须 flush
2. 没命中 → **轮转分配** `next_asid++`，越过 6 归零
3. 新分配的槽 → `need_flush = true`

> **HFT 直觉**：这台机器上如果**同时活跃的进程数 > 6**，每次切换都会挤掉别人的 ASID，
> 从而**强制 flush** —— PCID 的收益直接归零。
> 这也是"**关键线程独占物理核**"在 TLB 层面最硬的理由：核上只有你一个用户 mm，
> ASID 槽永远命中，切换永不 flush。

---

## 7. TLB flush：三代计数与 shootdown

### 7.1 入口与"范围 flush 阈值"

```c
/* arch/x86/mm/tlb.c:1001 */
void flush_tlb_mm_range(struct mm_struct *mm, unsigned long start,
			unsigned long end, unsigned int stride_shift,
			bool freed_tables)
{
	struct flush_tlb_info *info;
	u64 new_tlb_gen;
	int cpu;

	cpu = get_cpu();

	/* Should we flush just the requested range? */
	if ((end == TLB_FLUSH_ALL) ||
	    ((end - start) >> stride_shift) > tlb_single_page_flush_ceiling) {
		start = 0;
		end = TLB_FLUSH_ALL;
	}

	/* This is also a barrier that synchronizes with switch_mm(). */
	new_tlb_gen = inc_mm_tlb_gen(mm);

	info = get_flush_tlb_info(mm, start, end, stride_shift, freed_tables,
				  new_tlb_gen);

	/*
	 * flush_tlb_multi() is not optimized for the common case in which only
	 * a local TLB flush is needed. Optimize this use-case by calling
	 * flush_tlb_func_local() directly in this case.
	 */
	if (cpumask_any_but(mm_cpumask(mm), cpu) < nr_cpu_ids) {
		flush_tlb_multi(mm_cpumask(mm), info);
	} else if (mm == this_cpu_read(cpu_tlbstate.loaded_mm)) {
		lockdep_assert_irqs_enabled();
		local_irq_disable();
		flush_tlb_func(info);
		local_irq_enable();
	}

	put_flush_tlb_info();
	put_cpu();
	mmu_notifier_arch_invalidate_secondary_tlbs(mm, start, end);
}
```

⚠️ **一个重要阈值**：

```c
/* arch/x86/mm/tlb.c:957 */
unsigned long tlb_single_page_flush_ceiling __read_mostly = 33;
```

**超过 33 页就直接退化成全量 flush**。因为逐页 `invlpg` 在页数多时并不比全刷便宜。
单线程程序（只有自己在 `mm_cpumask` 里）走**本地快路径**，不发 IPI。

### 7.2 接收端：三代计数

```c
/* arch/x86/mm/tlb.c:743-758 */
static void flush_tlb_func(void *info)
{
	/*
	 * We have three different tlb_gen values in here.  They are:
	 *
	 * - mm_tlb_gen:     the latest generation.
	 * - local_tlb_gen:  the generation that this CPU has already caught
	 *                   up to.
	 * - f->new_tlb_gen: the generation that the requester of the flush
	 *                   wants us to catch up to.
	 */
	const struct flush_tlb_info *f = info;
	struct mm_struct *loaded_mm = this_cpu_read(cpu_tlbstate.loaded_mm);
	u32 loaded_mm_asid = this_cpu_read(cpu_tlbstate.loaded_mm_asid);
	u64 local_tlb_gen = this_cpu_read(cpu_tlbstate.ctxs[loaded_mm_asid].tlb_gen);
	bool local = smp_processor_id() == f->initiating_cpu;
	unsigned long nr_invalidate = 0;
	u64 mm_tlb_gen;
	...
	if (!local) {
		inc_irq_stat(irq_tlb_count);
		count_vm_tlb_event(NR_TLB_REMOTE_FLUSH_RECEIVED);

		/* Can only happen on remote CPUs */
		if (f->mm && f->mm != loaded_mm)
			return;
	}
```

**generation 机制是整个远程 flush 的核心**：
- `mm->context.tlb_gen`：mm 的全局版本号，每次改页表前 `inc_mm_tlb_gen()` 自增
- 每个 CPU 每个 ASID 槽记住 `local_tlb_gen`：我追到第几代了
- IPI 带的是"请追到第 N 代"

好处是 **IPI 可以合并/去重**：多个并发 flush 只要把 generation 推高，
接收端一次就能追到最新，不需要每个都刷一遍。

```c
/* arch/x86/include/asm/tlbflush.h:265 */
static inline u64 inc_mm_tlb_gen(struct mm_struct *mm)
{
	/*
	 * Bump the generation count.  This also serves as a full barrier
	 * that synchronizes with switch_mm(): callers are required to order
	 * their read of mm_cpumask after their writes to the paging
	 * structures.
	 */
	return atomic64_inc_return(&mm->context.tlb_gen);
}
```

### 7.3 部分 flush 的两个前提条件

```c
/* arch/x86/mm/tlb.c:861-891 */
	if (f->end != TLB_FLUSH_ALL &&
	    f->new_tlb_gen == local_tlb_gen + 1 &&
	    f->new_tlb_gen == mm_tlb_gen) {
		/* Partial flush */
		unsigned long addr = f->start;
		...
		nr_invalidate = (f->end - f->start) >> f->stride_shift;
		while (addr < f->end) {
			flush_tlb_one_user(addr);
			addr += 1UL << f->stride_shift;
		}
		if (local)
			count_vm_tlb_events(NR_TLB_LOCAL_FLUSH_ONE, nr_invalidate);
	} else {
		/* Full flush. */
		nr_invalidate = TLB_FLUSH_ALL;
		flush_tlb_local();
		if (local)
			count_vm_tlb_event(NR_TLB_LOCAL_FLUSH_ALL);
	}
```

源码注释解释了为什么要有这两个条件（并发 flush 乱序到达时，若贸然做部分 flush 会破坏
"我已经做完了追上 local_tlb_gen 所需的全部 flush"这个不变式）。

### 7.4 ⭐ 发给谁：lazy TLB 模式会被跳过

```c
/* arch/x86/mm/tlb.c:905-942 */
static bool tlb_is_not_lazy(int cpu, void *data)
{
	return !per_cpu(cpu_tlbstate_shared.is_lazy, cpu);
}

STATIC_NOPV void native_flush_tlb_multi(const struct cpumask *cpumask,
					 const struct flush_tlb_info *info)
{
	count_vm_tlb_event(NR_TLB_REMOTE_FLUSH);
	if (info->end == TLB_FLUSH_ALL)
		trace_tlb_flush(TLB_REMOTE_SEND_IPI, TLB_FLUSH_ALL);
	else
		trace_tlb_flush(TLB_REMOTE_SEND_IPI,
				(info->end - info->start) >> PAGE_SHIFT);

	/*
	 * If no page tables were freed, we can skip sending IPIs to
	 * CPUs in lazy TLB mode. They will flush the CPU themselves
	 * at the next context switch.
	 *
	 * However, if page tables are getting freed, we need to send the
	 * IPI everywhere, to prevent CPUs in lazy TLB mode from tripping
	 * up on the new contents of what used to be page tables, while
	 * doing a speculative memory access.
	 */
	if (info->freed_tables)
		on_each_cpu_mask(cpumask, flush_tlb_func, (void *)info, true);
	else
		on_each_cpu_cond_mask(tlb_is_not_lazy, flush_tlb_func,
				(void *)info, 1, cpumask);
}
```

| `freed_tables` | 行为 | 原因 |
|----------------|------|------|
| `false` | `on_each_cpu_cond_mask(tlb_is_not_lazy, ...)` —— **跳过 lazy TLB 的 CPU** | lazy 模式下该 CPU 反正下次切换会自己 flush |
| `true`（页表页被释放） | `on_each_cpu_mask(...)` —— **全发，一个不落** | 否则 lazy CPU 可能**投机访问到刚被释放又另作他用的页表页内存** |

> 这与 15.2 讲的 `active_mm` / lazy TLB 是同一件事的两个侧面。
> `is_lazy` 标记就在 `struct tlb_state_shared` 里（`tlbflush.h:151-172`）。

---

## 8. PTI：一个 mm，两套页表

Meltdown 之后引入的 **Kernel Page Table Isolation（KPTI / PTI）**，让每个 mm 有**两张 PGD**：
内核态用的（完整）和用户态用的（只含用户区 + 少量入口代码）。

### 8.1 何时启用

```c
/* arch/x86/mm/pti.c:78 */
void __init pti_check_boottime_disable(void)
{
	char arg[5];
	int ret;

	/* Assume mode is auto unless overridden. */
	pti_mode = PTI_AUTO;

	if (hypervisor_is_type(X86_HYPER_XEN_PV)) {
		pti_mode = PTI_FORCE_OFF;
		pti_print_if_insecure("disabled on XEN PV.");
		return;
	}

	ret = cmdline_find_option(boot_command_line, "pti", arg, sizeof(arg));
	if (ret > 0)  {
		if (ret == 3 && !strncmp(arg, "off", 3)) { ... }
		if (ret == 2 && !strncmp(arg, "on", 2))  { ... goto enable; }
		if (ret == 4 && !strncmp(arg, "auto", 4)) { ... goto autosel; }
	}

	if (cmdline_find_option_bool(boot_command_line, "nopti") ||
	    cpu_mitigations_off()) {
		pti_mode = PTI_FORCE_OFF;
		pti_print_if_insecure("disabled on command line.");
		return;
	}

autosel:
	if (!boot_cpu_has_bug(X86_BUG_CPU_MELTDOWN))
		return;
enable:
	setup_force_cpu_cap(X86_FEATURE_PTI);
}
```

| 内核参数 | 效果 |
|----------|------|
| `pti=off` / `nopti` / `mitigations=off` | 强制关闭 |
| `pti=on` | 强制开启（哪怕 CPU 没漏洞） |
| `pti=auto`（默认） | **只有** `boot_cpu_has_bug(X86_BUG_CPU_MELTDOWN)` 时开 |
| Xen PV guest | 自动关闭（PV 下本来就有隔离） |

### 8.2 uPCID = kPCID + 2048

```c
/* arch/x86/include/asm/processor-flags.h:55 */
# define X86_CR3_PTI_PCID_USER_BIT	11

/* arch/x86/mm/tlb.c:149-156 */
static inline u16 user_pcid(u16 asid)
{
	u16 ret = kern_pcid(asid);
#ifdef CONFIG_PAGE_TABLE_ISOLATION
	ret |= 1 << X86_CR3_PTI_PCID_USER_BIT;
#endif
	return ret;
}
```

因为 PTI 下每个 mm 有两套地址空间，PCID 也要翻倍 —— 用 **bit 11** 区分用户/内核两张表。
这也是前面 `MAX_ASID_AVAILABLE` 要减掉 1 位（12 → 11）的原因。

### 8.3 ⭐ 一个漂亮的纵深防御：用户可访问的 PGD 项在内核表里被标 NX

```c
/* arch/x86/mm/pti.c:123-161 */
pgd_t __pti_set_user_pgtbl(pgd_t *pgdp, pgd_t pgd)
{
	/*
	 * Changes to the high (kernel) portion of the kernelmode page
	 * tables are not automatically propagated to the usermode tables.
	 * ...
	 */
	if (!pgdp_maps_userspace(pgdp))
		return pgd;

	/* The user page tables get the full PGD, accessible from userspace: */
	kernel_to_user_pgdp(pgdp)->pgd = pgd.pgd;

	/*
	 * If this is normal user memory, make it NX in the kernel
	 * pagetables so that, if we somehow screw up and return to
	 * usermode with the kernel CR3 loaded, we'll get a page fault
	 * instead of allowing user code to execute with the wrong CR3.
	 *
	 * As exceptions, we don't set NX if:
	 *  - _PAGE_USER is not set.  This could be an executable
	 *     EFI runtime mapping or something similar, and the kernel
	 *     may execute from it
	 *  - we don't have NX support
	 *  - we're clearing the PGD (i.e. the new pgd is not present).
	 */
	if ((pgd.pgd & (_PAGE_USER|_PAGE_PRESENT)) == (_PAGE_USER|_PAGE_PRESENT) &&
	    (__supported_pte_mask & _PAGE_NX))
		pgd.pgd |= _PAGE_NX;

	/* return the copy of the PGD we want the kernel to use: */
	return pgd;
}
```

**一次 `set_pgd` 同时写两张表，且两份内容不同**：
- 用户表：完整可访问
- 内核表：同一范围标 **NX**

于是"万一内核态 bug 让用户代码带着内核 CR3 跑起来" → 立刻 #PF，而不是静默执行。
这是用**同一个数据结构的不对称性**来做攻击缓解，设计得很巧妙。

**HFT 成本**：PTI 让**每次系统调用/中断**都要切换 CR3（用户表 ↔ 内核表），
TLB 里用户条目在进内核后基本作废。**这是 syscall 变贵的直接原因之一**。
内网隔离、专用硬件的场景下，`mitigations=off` + 关闭 PTI 是常见调优项 —— 但要清楚自己在换什么。

---

## 9. 大页：2MB / 1GB / THP

### 9.1 常量

```c
/* include/linux/huge_mm.h:67-73 */
#define HPAGE_PMD_ORDER (HPAGE_PMD_SHIFT-PAGE_SHIFT)
#define HPAGE_PMD_NR (1<<HPAGE_PMD_ORDER)
#define HPAGE_PMD_SHIFT PMD_SHIFT
#define HPAGE_PMD_SIZE	((1UL) << HPAGE_PMD_SHIFT)
#define HPAGE_PMD_MASK	(~(HPAGE_PMD_SIZE - 1))
```

| 量 | 值 | 推导 |
|----|----|----|
| `PMD_SHIFT` | 21 | |
| `HPAGE_PMD_SIZE` | **2MB** | `1 << 21` |
| `HPAGE_PMD_ORDER` | 9 | `21 - 12` |
| **`HPAGE_PMD_NR`** | **512** | `1 << 9` —— 一个大页 = 512 个 4KB 页 |

**1GB 页**由 PUD 层直接映射（`PUD_SHIFT = 30`），需要 CPU 支持 `pdpe1gb`：

```c
/* arch/x86/include/asm/cpufeatures.h:67 */
#define X86_FEATURE_GBPAGES		( 1*32+26) /* "pdpe1gb" GB pages */
```

### 9.2 THP 三态（v6.6）

```c
/* include/linux/huge_mm.h:43 */
enum transparent_hugepage_flag {
	TRANSPARENT_HUGEPAGE_UNSUPPORTED,
	TRANSPARENT_HUGEPAGE_FLAG,
	TRANSPARENT_HUGEPAGE_REQ_MADV_FLAG,
	TRANSPARENT_HUGEPAGE_DEFRAG_DIRECT_FLAG,
	TRANSPARENT_HUGEPAGE_DEFRAG_KSWAPD_FLAG,
	TRANSPARENT_HUGEPAGE_DEFRAG_KSWAPD_OR_MADV_FLAG,
	TRANSPARENT_HUGEPAGE_DEFRAG_REQ_MADV_FLAG,
	TRANSPARENT_HUGEPAGE_DEFRAG_KHUGEPAGED_FLAG,
	TRANSPARENT_HUGEPAGE_USE_ZERO_PAGE_FLAG,
};
```

sysfs 节点（`mm/huge_memory.c`）：

| 路径 | 取值 | 含义 |
|------|------|------|
| `/sys/kernel/mm/transparent_hugepage/enabled` | `always` \| `madvise` \| `never` | 主开关（**v6.6 只有三态**，没有后来的 `defer`） |
| `.../defrag` | `always` \| `defer` \| `defer+madvise` \| `madvise` \| `never` | 内存紧张时是否等规整 |
| `.../use_zero_page` | 0 \| 1 | 读零页时是否用共享零大页 |
| `.../hpage_pmd_size` | 只读 | 当前 PMD 大页字节数（这里是 2097152） |
| `.../khugepaged/pages_to_scan` | 整数 | 每轮扫多少页 |
| `.../khugepaged/scan_sleep_millisecs` | 整数 | 轮间隔 |
| `.../khugepaged/alloc_sleep_millisecs` | 整数 | 分配失败后的退避 |
| `.../khugepaged/max_ptes_none` | ≤ 511 | 允许的最大"未映射 PTE 数" |
| `.../khugepaged/max_ptes_swap` | ≤ 511 | 允许的最大"已换出 PTE 数" |
| `.../khugepaged/max_ptes_shared` | ≤ 511 | 允许的最大"共享 PTE 数" |

⚠️ **默认值与书上/网上的老资料不同**，v6.6 实测（`mm/khugepaged.c:74/76/394-397`）：

| 参数 | 默认值 | 源码 |
|------|--------|------|
| `scan_sleep_millisecs` | **10000**（10 秒） | `khugepaged.c:74` |
| `alloc_sleep_millisecs` | **60000**（60 秒） | `khugepaged.c:76` |
| `pages_to_scan` | `HPAGE_PMD_NR * 8` = **4096** | `khugepaged.c:394` |
| `max_ptes_none` | `HPAGE_PMD_NR - 1` = **511** | `khugepaged.c:395` |
| `max_ptes_swap` | `HPAGE_PMD_NR / 8` = **64** | `khugepaged.c:396` |
| `max_ptes_shared` | `HPAGE_PMD_NR / 2` = **256** | `khugepaged.c:397` |

> 📌 **又一处过时注释**：`khugepaged.c` 里说 "30 second" 的地方，实际默认值是 **10000 ms**（10 秒）。
> 与 15.1 那两处一样，**要数字就看定义，别看注释**。

### 9.3 ⭐ THP 与实时内核互斥

```c
/* mm/Kconfig:811-815 */
menuconfig TRANSPARENT_HUGEPAGE
	bool "Transparent Hugepage Support"
	depends on HAVE_ARCH_TRANSPARENT_HUGEPAGE && !PREEMPT_RT
	select COMPACTION
	select XARRAY_MULTI
```

**`depends on ... && !PREEMPT_RT`** —— 只要开了 `PREEMPT_RT`（实时补丁），
THP **根本不会被编译进去**。原因是 THP 的合并/分裂带有不可预测的延迟（compaction、khugepaged 扫描）。

> 这条对 HFT 极重要：如果你的目标内核是 PREEMPT_RT，"要不要关 THP"这个问题**不存在** ——
> 它已经不在了，只能用**显式 hugetlbfs / `MAP_HUGETLB`**。
> 反过来说，非 RT 内核上 THP 存在时，关掉它的标准做法是 `echo never > .../enabled`。

---

## 10. 现代演进（书上完全没有的部分）

| 特性 | Kconfig / CPUID | 版本 | 与页表的关系 |
|------|-----------------|------|-------------|
| **LA57**（5 级页表） | `CONFIG_X86_5LEVEL` / `X86_FEATURE_LA57` | v4.17+ | 多一级 PGD，地址空间 128TB → 64PB；**walk 多一次访存** |
| **PTI**（页表隔离） | `CONFIG_PAGE_TABLE_ISOLATION` | v4.15+ | 每 mm 两套 PGD；PCID 减 1 位 |
| **PCID / INVPCID** | `X86_FEATURE_PCID` / `X86_FEATURE_INVPCID` | v4.14 起默认启用 | 见第 6、7 节 |
| **LAM**（地址掩码） | `CONFIG_ADDRESS_MASKING` / `X86_FEATURE_LAM` | v6.3+ | 忽略 VA 高 6/15 位做元数据标签；`tlb_state.lam`、`build_cr3(..., lam)` |
| **PKS / PKU**（保护钥匙） | `X86_FEATURE_PKU` / `CONFIG_ARCH_ENABLE_MEMORY_HOTPLUG`… | v4.6 / v5.13 | PTE bit 59-62 存 key；免 flush 改权限 |
| **Memory Protection Keys** | `X86_FEATURE_OSPKE` | v4.6+ | `mm->context.pkey_allocation_map` |
| **`MAP_HUGETLB` + `MADV_COLLAPSE`** | — | v6.1+ | 显式请求/就地折叠大页，避开 khugepaged 的不确定性 |

### 10.1 ⚠️ 订正：v6.6 **还没有** AMD INVLPGB

很多资料会告诉你"Linux 用 AMD 的 INVLPGB 指令做广播式 TLB 无效化，取代 IPI"。
**在 v6.6 上这是错的**：

```
$ grep -c "INVLPGB" arch/x86/include/asm/cpufeatures.h (v6.6)
0
```

`X86_FEATURE_INVLPGB` 在 v6.6 的 `cpufeatures.h` 里**不存在**（v6.6 里该特性位尚未合入）。
v6.6 的远程 flush 走的是实打实的 **IPI**（`on_each_cpu_mask` / `on_each_cpu_cond_mask`，见 7.4 节）。

> 这正是 skill 里"写内核机制前必须核对源码"的意义：
> 一个"我记得明明有"的特性，很可能只是版本还没到。

### 10.2 LAM 对页表的影响

```c
/* arch/x86/include/asm/tlbflush.h:103-111 */
#ifdef CONFIG_ADDRESS_MASKING
	/*
	 * Active LAM mode.
	 *
	 * X86_CR3_LAM_U57/U48 shifted right by X86_CR3_LAM_U57_BIT or 0 if LAM
	 * disabled.
	 */
	u8 lam;
#endif
```

LAM（Linear Address Masking）让 CPU 在地址翻译时**忽略 VA 的高位**，
于是软件可以在指针里塞元数据。注意 `build_cr3()` 的第三个参数就是 `lam` ——
LAM 模式是**编进 CR3** 的，切换 mm 时跟着变。

---

## 11. 观测：把 TLB 变成数字

| 想看的量 | 命令 | 说明 |
|----------|------|------|
| TLB miss 率 | `perf stat -e dTLB-loads,dTLB-load-misses ./app` | 数据 TLB |
| 指令侧 | `perf stat -e iTLB-loads,iTLB-load-misses ./app` | 指令 TLB |
| walk 完成次数 | `perf stat -e dTLB-load-misses.walk_completed ./app` | Intel PMU |
| walk 走内存的次数 | `perf stat -e dTLB-load-misses.walk_active ./app` | 配合 `walk_completed` 估算 walk 延迟 |
| 远程 flush 次数 | `grep nr_tlb_remote_flush /proc/vmstat` | 收到 IPI 的 flush 次数 |
| TLB 中断 | `grep TLB /proc/interrupts` | `irq_tlb_count` 累加 |
| 页表占用 | `grep PageTables /proc/meminfo` | 全系统 |
| 单进程页表 | `grep VmPTE /proc/$PID/status` | v6.6 有 |
| flush 事件明细 | `perf record -e tlb:tlb_flush -a` | tracepoint，带 `freed_tables` 语义 |

`trace_tlb_flush` 的三个类型（`arch/x86/mm/tlb.c:896-899`）：

| 类型 | 含义 |
|------|------|
| `TLB_REMOTE_SHOOTDOWN` | 收到别人的 IPI 而 flush（`!local`） |
| `TLB_LOCAL_SHOOTDOWN` | 本地 flush，`f->mm == NULL`（全 mm） |
| `TLB_LOCAL_MM_SHOOTDOWN` | 本地 flush，指定 mm |

```bash
# 一条命令看全：某个进程的 TLB 行为画像
perf stat -e \
  dTLB-loads,dTLB-load-misses,iTLB-load-misses,\
  dTLB-load-misses.walk_completed,\
  dTLB-load-misses.walk_active,\
  tlb:tlb_flush \
  ./your_hft_binary
```

---

## 12. HFT 关联：TLB 是隐形杀手

| 现象 | 机制 | 怎么办 |
|------|------|--------|
| **TLB miss 抖动** | walk 要 4~5 次访存，全 miss 时是串行 DRAM 延迟 | **大页**（`MAP_HUGETLB` / hugetlbfs，比 THP 确定） |
| **TLB shootdown IPI** | 别的核改了共享页表 → 给你发 IPI → 你的核被打断 | 关键线程**独占核**（`isolcpus`），并**避免共享可写映射** |
| **ASID 槽被打穿** | 同核活跃进程 > 6 → 每次切换都 flush | 独占核；或至少保证关键线程是核上唯一的"热" mm |
| **PTI 的 syscall 成本** | 每次进内核切 CR3，用户 TLB 条目失效 | 隔离网络下 `mitigations=off`（需评估风险）；批量化 syscall（io_uring、`readv`） |
| **mprotect 的 flush 风暴** | RW/USER/NX 变化属于 `flush_on_change` | 权限**一次性设好**，别在热路径改 |
| **khugepaged 抖动** | 后台线程扫描 + compaction，随时抢占 | `echo 0 > /sys/kernel/mm/transparent_hugepage/khugepaged/pages_to_scan` 或整体 `never`；RT 内核下 THP 不存在 |

### 12.1 一个可复现的 TLB 覆盖能力实验

用 stride 访问扫过一个大数组，stride 越大，单位数据量的 TLB 覆盖越差：

```c
/* tlb_probe.c —— 不同 stride 下的访问延迟，间接反映 TLB 覆盖能力
 * 编译：gcc -O2 -o tlb_probe tlb_probe.c
 * 运行：for s in 4096 65536 2097152; do ./tlb_probe $s; done
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static double now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

int main(int argc, char **argv)
{
    size_t total  = 256ul << 20;              /* 256MB 工作集 */
    size_t stride = argc > 1 ? strtoul(argv[1], 0, 10) : 4096;
    const int ITERS = 20;

    /* MAP_POPULATE: 先把页表建好，把"首次缺页"排除在计时之外 */
    char *buf = mmap(NULL, total, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
    if (buf == MAP_FAILED) { perror("mmap"); return 1; }

    size_t n = total / stride;
    double best = 1e18;
    volatile char sink;

    for (int it = 0; it < ITERS; it++) {
        double t0 = now_ns();
        for (size_t i = 0; i < n; i++)
            sink += buf[i * stride];         /* 每个 stride 只碰一个字节 */
        double t1 = now_ns();
        double per = (t1 - t0) / n;
        if (per < best) best = per;
    }

    printf("stride=%-10zu n=%-10zu  best=%7.2f ns/access\n",
           stride, n, best);
    (void)sink;
    return 0;
}
```

**怎么读结果**：

| stride | 4KB 页模式下的行为 | 期望现象 |
|--------|-------------------|----------|
| 4096（= 一页） | 每次访问换一页 | **TLB miss 最多**，单次延迟最高 |
| 65536（跨过 15 页） | 每 16 次访问才换一页 | 延迟明显下降 |
| 2097152（2MB） | 每 512 次访问换页 | 最低；若用了大页，TLB 条目数骤降 |

配 `perf stat` 一起跑，才算把因果坐实：

```bash
perf stat -e dTLB-loads,dTLB-load-misses ./tlb_probe 4096
perf stat -e dTLB-loads,dTLB-load-misses ./tlb_probe 2097152
```

> **为什么不给"TLB miss = X ns"这种数字**：
> 它同时取决于 CPU 型号、页表项的 cache 命中情况、是否开了大页、有没有 EPT（虚拟化）。
> 与其背一个不可靠的数字，不如**给你一把尺子** —— 在自己的目标机器上量一次。

---

→ [01 CSAPP Ch9 TLB/翻译](../../../02-computer-systems/chapter-09-virtual-memory/notes/section-9.6-地址翻译.md) · [06 Gorman Ch3 页表](../../../06-linux-mm/chapter-03-page-table-management/) · [06 THP note](../../../06-linux-mm/chapter-03-page-table-management/notes/note-透明大页THP.md)

> ↔ [ULK Ch9 §4 缺页异常](../../../16-linux-kernel-deep/chapter-09-process-address-space/notes/section-4-缺页异常.md) · [15.1 地址空间](./section-15.1-地址空间.md) · [15.2 内存描述符](./section-15.2-内存描述符.md) · [15.8 从访问到缺页](./section-15.8-从访问到缺页概念.md)

<details>
<summary>自测题（点击展开）</summary>

**Q1.** x86_64 四级页表的翻译过程？TLB miss 的代价是多少？

<details><summary>答案</summary>

4 级页表：CR3 → PML4 → PDPT → PD → PT → 物理页。48 位 VA = 9+9+9+9+12。TLB miss 时 CPU 硬件遍历 4 级页表（4 次内存访问），约 100-300ns。TLB hit 约 1-2ns。Huge Page（2MB）只需 3 级页表，减少一级查找 + 大幅减少 TLB 条目数。HFT 用 Huge Page 将 TLB miss 率从 5% 降到 < 0.1%。

> **⚠️ 按 v6.6 修订/补充（本篇核过源码后的修正）**
>
> 上面这段**方向对，但几个数字和表述需要修正**：
>
> 1. **"四级"只在 4 级编译时成立**。`CONFIG_X86_5LEVEL=y` 时是 **5 级**：
>    CR3 → PML5(PGD) → P4D → PUD → PMD → PT，57 位 VA = 9+9+9+9+9+12。
>    而且 `PGDIR_SHIFT` / `PTRS_PER_P4D` 是**运行时变量**（`pgdir_shift` / `ptrs_per_p4d`），
>    由 `pgtable_l5_enabled()`（即 CPUID `X86_FEATURE_LA57`，或启动参数 `no5lvl`）决定。
>
> 2. **"TLB hit 约 1-2ns"偏高**。L1 dTLB 命中的 load-to-use 延迟与 L1D 命中同量级，
>    **约 4~5 个 cycle**（3GHz 下 ≈ 1.3~1.7ns）。真正的"1 cycle"是 TLB 查表本身的开销，
>    不含随后的 L1D 访问。
>
> 3. **"4 次内存访问 → 100-300ns"是最坏情况，不是典型值**。
>    page walk 的 4 次访存**绝大多数命中 L1/L2**（页表项是普通内存，会被缓存）。
>    只有页表项本身被换出、或首次 cold 访问时，才真的走 DRAM。量级：
>    - 各级命中 L2：约 **10~20 cycle**
>    - 全 miss 走 DRAM：4 次串行，约 **200~300 cycle**（≈ 70~100ns @3GHz）
>
>    所以"100-300ns"这个区间作为**最坏情况上界**勉强成立，但把它当常态会高估 TLB 的代价。
>
> 4. **"Huge Page 只需 3 级"要注意**：2MB 大页是 **PMD 层直接映射**，
>    walk 到 PMD 就停（少一次 PT 访存）；1GB 大页是 **PUD 层直接映射**（需 `pdpe1gb`）。
>    "减少一级"成立，但更关键的收益是 **TLB 覆盖能力提升 512 倍**（2MB）或 262144 倍（1GB）。
>
> 5. **"5% → < 0.1%"这种数字没有普适性**，取决于工作集大小与访问模式。
>    要数字就用本篇第 11、12 节的方法在目标机器上量。

</details>

**Q2.** PTE 的 Dirty 位和 Accessed 位分别什么作用？

<details><summary>答案</summary>

Dirty 位：页被写过（写回磁盘时需要 flush）。Accessed 位：页被读过或写过（页面回收时优先换出未访问页）。这两个位由 CPU 硬件设置，内核清除。页面回收器扫描 Accessed 位判断页面活跃度。HFT 内存锁定后不参与回收，Accessed 位不重要。

> **⚠️ 按 v6.6 修订/补充**
>
> 1. **bit 位置**：`_PAGE_BIT_ACCESSED = 5`，`_PAGE_BIT_DIRTY = 6`
>    （`arch/x86/include/asm/pgtable_types.h:15-16`），两者都是**硬件置位、内核清除**。
>
> 2. **"Dirty = 写回磁盘"只对文件页成立**。更准确的分法：
>    - **文件页**：Dirty → `writeback` 时需要回写文件系统
>    - **匿名页**：没有"回写"，Dirty 用来判断这一页**从换入后是否被写过**，
>      决定 swap out 时是否需要真的写盘
>
> 3. **清除它们的代价不一样**，这条很关键（v6.6 `tlbflush.h:284` 的 `flush_on_clear`）：
>    - **置位**：硬件自己会做，**不需要 flush TLB**
>    - **清除**：**必须 flush TLB**，否则 CPU 可能继续用缓存的旧条目而不重新置位
>
>    所以 `madvise(MADV_FREE)`、`ptep_clear_flush_young()`（回收扫 Accessed）
>    这类操作都带着 TLB flush 成本 —— 这是回收路径变贵的隐藏来源。
>
> 4. **HFT 那句"mlock 后 Accessed 位不重要"要加个限定**：
>    确实不参与回收了，但 **`mlock` 之后仍然可以有缺页**（`VM_LOCKONFAULT` 语义下首次访问才兑现），
>    而且 **`MADV_DONTNEED` 清 Present 也是要 flush 的**。
>    准确的说法是：**锁定 + 预取（`MAP_POPULATE`）之后，运行期不再有页表改动，
>    因而不再有 TLB flush** —— 这才是真正的收益来源。
>
> 5. 还有一个容易漏的：**`_PAGE_SOFT_DIRTY`（bit 11，软件位）** 与硬件 Dirty 是两回事，
>    用于 CRIU 的增量 checkpoint。改它**不需要 flush**（属于 `software_flags`）。

</details>

**Q3.** x86_64 硬件支持 4096 个 PCID，Linux 实际用了多少个？为什么？

<details><summary>答案</summary>

**6 个**，即 `TLB_NR_DYN_ASIDS = 6`（`arch/x86/include/asm/tlbflush.h:65`）。

而且 Linux **故意不用** RISC 那套"每个进程一个 ASID、用完就全刷重来"的方案。
源码注释说得很直白：

> "We don't use the traditional ASID implementation, where each process/mm gets
> its own ASID and flush/restart when we run out of ASID space.
> Instead we have a small per-cpu array of ASIDs and cache the last few mm's
> that came by on this CPU, allowing cheaper switch_mm between processes on this CPU."

**为什么只要 6 个**：注释给的理由是"6 should be plenty"，以及
**让 `struct tlb_state` 塞进两条 cache line**。每 CPU 的 ASID 表是极热的数据，
宁可小也要快。

**三套编号别搞混**：

| 名字 | 范围 | 说明 |
|------|------|------|
| ASID | `[0, 5]` | 内核内部的槽位下标 |
| kPCID | `[1, 6]` | 写进 CR3 的值，`= ASID + 1`（避开特殊的 PCID 0） |
| uPCID | `[2049, 2054]` | PTI 下用户表用的 PCID，`= kPCID + 2048`（即置 bit 11） |

**分配流程**（`choose_new_asid()`）：
1. 线性扫 6 个槽找 `ctx_id` 匹配当前 mm 的
2. 命中还要比 `tlb_gen`，mm 更新了就得 flush
3. 没命中就轮转分配 `next_asid++`（超 6 归零），且必定 flush

**HFT 推论**：核上同时活跃的 mm 超过 6 个，PCID 收益归零 —— 这是"关键线程独占物理核"
在 TLB 层面最硬的理由。

</details>

**Q4.** 为什么 `mprotect(PROT_NONE)` 之后访问会 SIGSEGV，而不是被当成"还没映射"？

<details><summary>答案</summary>

因为 x86 上 **`PROT_NONE` 的 PTE 被 `pte_present()` 判定为 present**，
靠这个把"未映射（该分配）"和"不可访问（该 SIGSEGV）"区分开。

具体做法（`arch/x86/include/asm/pgtable_types.h:44-47`）：

```c
/* If _PAGE_BIT_PRESENT is clear, we use these: */
/* - if the user mapped it with PROT_NONE; pte_present gives true */
#define _PAGE_BIT_PROTNONE	_PAGE_BIT_GLOBAL
```

即 **bit 8（GLOBAL）被复用为 PROTNONE**，而：

```c
static inline int pte_present(pte_t a)
{
	return pte_flags(a) & (_PAGE_PRESENT | _PAGE_PROTNONE);
}
```

`PROT_NONE` 的 PTE 模板（`PAGE_NONE`）是
`Present=0, RW=0, User=0, Accessed=1, GLOBAL(=PROTNONE)=1`：
- CPU 看到 Present=0 → **#PF**
- 内核 `do_page_fault` 里 `pte_present()` 返回 **true** → 走"权限错误"分支 → `SIGSEGV`

对比：真正未映射的 PTE 全 0，`pte_present()` 返回 false → 走"缺页分配"分支。

**注意两个补充点**：
1. 真正的权限检查 `pte_access_permitted()` **只认 `_PAGE_PRESENT`**，不认 PROTNONE
   —— 所以 PROTNONE 的页会被判为"不可访问"，这正是我们想要的。
2. PMD 版的 `pmd_present()` 还要多看一个 `_PAGE_PSE`，
   因为拆分 THP 时会"先清 Present、保留 PSE"，那个窗口期必须仍算 present。

**代价**：`PROT_NONE` 的页**不会释放 PTE 页也不释放 TLB 条目**，
用它做 guard page 买到的是"越界立刻崩"的确定性，不是内存。

</details>

**Q5.** 改 PTE 时，哪些位的变化必须 flush TLB？

<details><summary>答案</summary>

v6.6 把规则写死在 `pte_flags_need_flush()`（`arch/x86/include/asm/tlbflush.h:280`）里，分三类：

| 类别 | 位 | flush 条件 |
|------|----|-----------|
| **软件位（永远不 flush）** | SOFTW1(9)、SOFTW2(10)、SOFTW3(11)、SOFTW4(57)、SAVED_DIRTY(58) | 改了随便改 —— MMU 不读这些位 |
| **只在清除时 flush** | DIRTY(6)、PRESENT(0)、ACCESSED(5) | 置位不用 flush（硬件自己会置，或未缓存项无所谓）；**清零必须 flush** |
| **任何变化都 flush** | RW(1)、USER(2)、PWT(3)、PCD(4)、PSE(7)、GLOBAL(8)、PAT、PAT_LARGE、PKEY×4、NX(63) | 只要变了就 flush |

源码里还有三个 `BUILD_BUG_ON` 保证这三类**两两不相交**：

```c
BUILD_BUG_ON(flush_on_clear & software_flags);
BUILD_BUG_ON(flush_on_clear & flush_on_change);
BUILD_BUG_ON(flush_on_change & software_flags);
```

**实践含义**：
- `mprotect()` 改 RW/EXEC → **必然 flush + 可能的 IPI shootdown**，别放热路径
- soft-dirty / uffd-wp 这类**软件位标记 → 零 flush 成本**，适合高频打标
- 回收路径清 Accessed（`ptep_clear_flush_young`）→ 每次都要 flush，这是回收变贵的隐藏成本

</details>

**Q6.** 远程 TLB flush（shootdown）什么时候会跳过某些 CPU？

<details><summary>答案</summary>

只有当**没有页表页被释放**时，才会跳过处于 **lazy TLB 模式**的 CPU
（`arch/x86/mm/tlb.c:932-942`）：

```c
if (info->freed_tables)
	on_each_cpu_mask(cpumask, flush_tlb_func, (void *)info, true);
else
	on_each_cpu_cond_mask(tlb_is_not_lazy, flush_tlb_func,
			(void *)info, 1, cpumask);
```

| `freed_tables` | 做法 | 原因 |
|----------------|------|------|
| `false` | 用 `tlb_is_not_lazy` 过滤，**跳过 lazy CPU** | lazy 模式下这些 CPU 反正下次切换会自己 flush |
| `true` | **全发，一个不落** | 否则 lazy CPU 可能**投机访问到刚被释放、已另作他用的页表页**，读到垃圾 |

另外还有个 **generation 合并机制**：`mm->context.tlb_gen` 是全局版本号，
IPI 里带的是"请追到第 N 代"。接收端（`flush_tlb_func`）
维护自己的 `local_tlb_gen`：
- 如果已经追上（`f->new_tlb_gen <= local_tlb_gen`）→ **直接返回，什么都不做**
- 如果 `local_tlb_gen == mm_tlb_gen` → 也是直接返回

所以**并发的多个 flush 会自然合并**，不会重复刷。

**部分 flush 还有两个前提条件**（`tlb.c:861`）：
1. `f->new_tlb_gen == local_tlb_gen + 1`（只差一代，中间没有漏掉的全量 flush）
2. `f->new_tlb_gen == mm_tlb_gen`（刷完这一发就完全追上）

不满足就退化成**全量 flush** —— 因为"部分 flush 并不比全量便宜多少"
（源码注释原话），不如一步到位。

最后，还有个**范围阈值**：`tlb_single_page_flush_ceiling = 33` 页。
超过 33 页就直接全量刷，不逐页 `invlpg`。

</details>

</details>
---
