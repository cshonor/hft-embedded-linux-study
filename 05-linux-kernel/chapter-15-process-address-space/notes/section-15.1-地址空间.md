## ① 地址空间 · Address Spaces

Linux 是 **虚拟内存 OS** — 每个进程看到 **独立的线性 VA 空间**；物理页通过 **页表** 映射，**MMU** 在访问时完成 **VA→PA**。

> **本篇分工**：实体书小节已讲"地址空间是什么"，本篇不复述。本篇只做三件事：
> ① 把书中"128TB / 三级页表"这类**概念图换成 v6.6 的精确数值**（能背下来的那种）；
> ② 解释 mmap 区与栈之间那个洞**到底是怎么算出来的**（源码实证，不是经验值）；
> ③ 补上书上没有的现代演进（5 级页表、LAM、MDWE）。

---

## 1. 三个层次：别把「地址空间」当成一张表

地址空间在内核里**不是**一个数据结构，而是**三层协作**。搞混这层是后面所有笔记读不懂的根源：

| 层次 | 数据结构 | 回答什么问题 | 谁在维护 |
|------|----------|--------------|----------|
| **① 有哪些区间** | `struct maple_tree mm_struct.mm_mt` | "这个 VA 归谁管、什么权限" | `mmap`/`munmap`/`mprotect` |
| **② 区间长什么样** | `struct vm_area_struct`（VMA） | 一段 `[vm_start, vm_end)` 的属性 + 回调 | 同上 |
| **③ 页在哪** | **页表**（PGD→P4D→PUD→PMD→PTE） | "这个 VA 当前映射到哪个物理页" | **缺页异常**按需建立 |

关键点：**① ② 是"承诺"，③ 是"兑现"**。
`mmap(1GB)` 只做 ① 和 ②（建一个 VMA），**一个页表项都不建**；真正访问时才由 `#PF` 走 ③。

```
mmap(1GB) 成功   →  只是 mm_mt 里多了一个 VMA，页表为空，RSS = 0
首次写第 1 页    →  #PF → do_anonymous_page() → 分配 1 个物理页 → 填 1 个 PTE
                    其余 262143 页仍未兑现
```

> **HFT 直接推论**：`MAP_POPULATE` / `mlockall(MCL_CURRENT)` 就是把"兑现"提前到启动阶段。
> 不这么做，`mmap` 一把 1GB 的行情缓冲只会换来**盘中的 262144 次缺页中断**。

---

## 2. x86_64 的精确版图（v6.6 源码实证）

书里给的"128TB 用户 + 128TB 内核"是 4 级页表情形。**v6.6 有两套布局**，取决于 CPU 是否支持 LA57（57 位线性地址）。

### 2.1 用户空间上限的两个版本

```c
/* arch/x86/include/asm/page_64_types.h:64 */
#ifdef CONFIG_X86_5LEVEL
#define __VIRTUAL_MASK_SHIFT	(pgtable_l5_enabled() ? 56 : 47)
#else
#define __VIRTUAL_MASK_SHIFT	47
#define task_size_max()		((_AC(1,UL) << __VIRTUAL_MASK_SHIFT) - PAGE_SIZE)
#endif

#define TASK_SIZE_MAX		task_size_max()
#define DEFAULT_MAP_WINDOW	((1UL << 47) - PAGE_SIZE)   /* = 0x7ffffffff000 */
```

```c
/* arch/x86/include/asm/page_64.h:82 —— 5 级情形下的 2 选 1 */
static __always_inline unsigned long task_size_max(void)
{
	unsigned long ret;

	alternative_io("movq %[small],%0","movq %[large],%0",
			X86_FEATURE_LA57,
			"=r" (ret),
			[small] "i" ((1ul << 47)-PAGE_SIZE),   /* 128 TB - 4KB */
			[large] "i" ((1ul << 56)-PAGE_SIZE));  /*  64 PB - 4KB */
	return ret;
}
```

| 情形 | 用户空间大小 | 边界值 |
|------|-------------|--------|
| **4 级页表**（绝大多数机器） | **128 TB** | `0x0000000000000000 ~ 0x00007fffffffffff` |
| **5 级页表**（LA57 启用） | **64 PB** | `0x0000000000000000 ~ 0x00ffffffffffffff` |

⚠️ 常见的"5 级 = 128PB"说法是**硬件**能力（57 位），**Linux 只暴露 56 位**给进程。
另外注意 `alternative_io` —— 这不是运行时 `if`，而是启动时用 **self-patching** 把 `movq $imm,%0` 改写成另一个立即数，**热路径零分支开销**。

### 2.2 完整版图（4 级，`Documentation/arch/x86/x86_64/mm.rst` 逐字）

```
Start addr    |   Offset   |     End addr     |  Size   | VM area description
================================================================================
0000000000000000 |    0       | 00007fffffffffff |  128 TB | user-space virtual memory, different per mm
__________________|____________|__________________|_________|__________________
0000800000000000 | +128    TB | ffff7fffffffffff | ~16M TB | ... non-canonical hole（非规范地址，访问必然 #GP）
__________________|____________|__________________|_________|__________________
                                                              |
                                                              | Kernel-space：所有进程共享同一份
______________________________________________________________|__________________
ffff800000000000 | -128    TB | ffff87ffffffffff |    8 TB | ... guard hole, 也预留给 hypervisor
ffff880000000000 | -120    TB | ffff887fffffffff |  0.5 TB | LDT remap for PTI
ffff888000000000 | -119.5  TB | ffffc87fffffffff |   64 TB | direct mapping of all physical memory (page_offset_base)
ffffc88000000000 |  -55.5  TB | ffffc8ffffffffff |  0.5 TB | ... unused hole
ffffc90000000000 |  -55    TB | ffffe8ffffffffff |   32 TB | vmalloc/ioremap space (vmalloc_base)
ffffe90000000000 |  -23    TB | ffffe9ffffffffff |    1 TB | ... unused hole
ffffea0000000000 |  -22    TB | ffffeaffffffffff |    1 TB | virtual memory map (vmemmap_base)
ffffeb0000000000 |  -21    TB | ffffebffffffffff |    1 TB | ... unused hole
ffffec0000000000 |  -20    TB | fffffbffffffffff |   16 TB | KASAN shadow memory
__________________|____________|__________________|_________|__________________
fffffc0000000000 |   -4    TB | fffffdffffffffff |    2 TB | ... unused hole（KASLR 的 vaddr_end）
fffffe0000000000 |   -2    TB | fffffe7fffffffff |  0.5 TB | cpu_entry_area mapping
ffffffff00000000 |   -4    GB | ffffffff7fffffff |    2 GB | ... unused hole
ffffffff80000000 |   -2    GB | ffffffff9fffffff |  512 MB | kernel text mapping（映射到物理地址 0）
ffffffffa0000000 |-1536    MB | fffffffffeffffff | 1520 MB | module mapping space
     FIXADDR_START | ~-11    MB | ffffffffff5fffff | ~0.5 MB | kernel-internal fixmap range
ffffffffff600000 |  -10    MB | ffffffffff600fff |    4 kB | legacy vsyscall ABI
```

5 级页表下**用户从 128TB → 64PB**，内核各区整体下移并等比例扩大（direct map 变为 32PB、vmalloc 变 12.5PB），
`-4TB` 以上的部分与 4 级**完全一致**。

**非规范地址洞（non-canonical hole）** 是 x86 的硬件要求：位 63..47（4 级）必须与位 47 相同（符号扩展）。
落在洞里的地址**连页表都不查**，直接 `#GP`。这也是为什么用户指针合法性检查必须要它。

### 2.3 内核空间为什么"所有进程共享"

页表是**每个 mm 一份**，但**内核半区的 PGD 项是指向同一批中间表的**。
`fork()` 复制页表时（`kernel/fork.c` 的 `dup_mm` → `copy_page_range`）**只复制用户空间部分**，
内核半区的 PGD 项是从 `init_mm.pgd` 的 `swapper_pg_dir` 直接拷贝模板。

> 结果：**不同进程的同一内核 VA 一定映射到同一 PA**。内核代码可以直接解引用内核指针，不需要"当前进程是谁"。

---

## 3. 用户进程的实际布局怎么读（可复现）

```bash
# 看自己的
cat /proc/self/maps

# 精确版：带权限 + 偏移 + 设备 + inode + 路径
# 7f2b4c000000-7f2b4c021000 r--p 00000000 fd:01 131075  /usr/lib/x86_64-linux-gnu/libc.so.6
# ^start      ^end          ^perm ^pgoff  ^dev  ^inode  ^path

# 带名字的匿名 VMA（见 §5.4）
cat /proc/self/maps | grep '\[anon:'
```

| 区域 | 谁建的 | 典型权限 | 增长 |
|------|--------|----------|------|
| text | `execve` → `load_elf_binary` 的 `elf_map` | `r-xp` | 固定 |
| rodata | 同上（ELF 的只读段单独一个 VMA） | `r--p` | 固定 |
| data / bss | 同上，bss 复用 data 的 VMA 尾部 | `rw-p` | 固定 |
| **heap** | `brk` / `malloc`（大块走 mmap） | `rw-p` | **↑** |
| **mmap 区** | `mmap`、动态库、`ld.so`、线程栈 | 各异 | **↓**（top-down） |
| **vvar / vdso** | 内核在 `execve` 时自动装 | `r--p` / `r-xp` | 固定 |
| **vsyscall** | legacy，`0xffffffffff600000`，4KB | `---p`（**不可执行**，只做兼容陷阱） | 固定 |

默认（非 legacy）是 **top-down**：`mmap` 从 `mm->mmap_base` 往下分配。

---

## 4. mmap 区与栈之间那个洞是怎么算出来的（源码实证）

这是书上完全没有、但**做 `MAP_FIXED` 地址规划时必须知道**的部分。

```c
/* arch/x86/mm/mmap.c:82 */
#define SIZE_128M    (128 * 1024 * 1024UL)

static unsigned long mmap_base(unsigned long rnd, unsigned long task_size,
			       struct rlimit *rlim_stack)
{
	unsigned long gap = rlim_stack->rlim_cur;                        /* RLIMIT_STACK，通常 8MB */
	unsigned long pad = stack_maxrandom_size(task_size) + stack_guard_gap;
	unsigned long gap_min, gap_max;

	/* Values close to RLIM_INFINITY can overflow. */
	if (gap + pad > gap)
		gap += pad;

	/*
	 * Top of mmap area (just below the process stack).
	 * Leave an at least ~128 MB hole with possible stack randomization.
	 */
	gap_min = SIZE_128M;
	gap_max = (task_size / 6) * 5;

	if (gap < gap_min)   gap = gap_min;
	else if (gap > gap_max) gap = gap_max;

	return PAGE_ALIGN(task_size - gap - rnd);
}
```

| 量 | v6.6 值 | 来源 |
|----|---------|------|
| `gap_min` | **128 MB** | `SIZE_128M` 硬编码 |
| `gap_max` | **task_size × 5/6**（4 级下 ≈ **106 TB**） | 代码算出来的 |
| `stack_guard_gap` | **1 MB**（`256UL << PAGE_SHIFT`，`mm/mmap.c:2142`） | 全局变量，**v6.6 已不是 sysctl** |
| `stack_maxrandom_size` | 64 位下 `0x3fffff << 12` = **16 GB**（上限） | `__STACK_RND_MASK(is32bit)` = `0x3fffff` |
| `rnd`（mmap 随机） | ≤ `mmap_rnd_bits` 位页，默认 **28 位** → 最多 **1 TB** | `ARCH_MMAP_RND_BITS_MIN = 28 if 64BIT` |

```
0x7ffffffff000  ←── TASK_SIZE / STACK_TOP（DEFAULT_MAP_WINDOW = 128TB - 4KB）
│
├─ 栈随机偏移（≤ 16GB）
├─ 栈本身（≤ RLIMIT_STACK，通常 8MB）
├─ stack_guard_gap（1MB，防栈溢出踩到下面）
│                                    ←── 这一整段 = gap（≥128MB，≤ task_size*5/6）
├─ ASLR 随机（≤ 1TB）
│
└─ 0x...  ←── mm->mmap_base，mmap 区顶，从此往下分配
```

⚠️ **源码注释有两处已经过时，别照抄**：

| 注释写的 | 实际值 | 位置 |
|----------|--------|------|
| "default scan ... every **30 second**" | `khugepaged_scan_sleep_millisecs = 10000`（**10 秒**） | `mm/khugepaged.c:71/74` |
| "**1GB** for 64bit, 8MB for 32bit" | `0x3fffff` 页 = **16 GB** | `arch/x86/include/asm/elf.h:329` |

> **坑**：内核注释会腐化。**要数字就去看定义，不要看注释**——这是本仓库笔记里反复出现的教训。

---

## 5. 现代演进：书上没有的几件事

### 5.1 5 级页表（LA57）
`CONFIG_X86_5LEVEL` + CPU 支持 LA57 才会启用。用户空间 128TB → **64PB**。
代价：**多一级页表 walk**（4 次内存访问 → 5 次）。普通应用几乎不会用超过 128TB，
**HFT 机器上 LA57 是纯负收益**（多一次 TLB miss 时的访存），除非确实需要映射 >128TB。

### 5.2 Linear Address Masking（LAM）
`CONFIG_ADDRESS_MASKING`（`arch/x86/Kconfig`）。允许软件**使用 VA 的高位存元数据**，
硬件在做地址翻译前自动屏蔽这些位。用途：ASAN 的 tag、JIT 的指针标记。
⚠️ 与 5 级页表**抢占同一批高位**——LAM 可用位数 = 57 - 实际 VA 位数。

### 5.3 Memory-Deny-Write-Execute（MDWE，v6.3+）
```c
#define PR_SET_MDWE			65
# define PR_MDWE_REFUSE_EXEC_GAIN	1
```
进程可以 `prctl(PR_SET_MDWE, PR_MDWE_REFUSE_EXEC_GAIN, 0, 0, 0)` 声明：
**本进程永不允许出现 W+X 的映射，也不允许把一个非可执行 VMA 变成可执行**。
内核侧实现在 `map_deny_write_exec()`（`include/linux/mman.h`），命中就 `-EACCES`。

### 5.4 匿名 VMA 命名（v5.17+，CONFIG_ANON_VMA_NAME）
```c
/* 给一块匿名内存起名字 —— /proc/pid/maps 里会显示 [anon:orderbook] */
prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, ring, RING_SIZE, "orderbook");
```
产出：`7f2b4c000000-7f2b4c200000 rw-p 00000000 00:00 0 [anon:orderbook]`
→ **HFT 上线审计神器**：不用再靠地址猜哪块内存是什么。

### 5.5 内核栈在 vmalloc 区（Ch 12.8 已实证）
进程内核栈（`CONFIG_VMAP_STACK` default y）分配在 **vmalloc 区**并带 **guard page**
——溢出立刻 fault，而不是悄悄踩坏邻居。

---

## 6. HFT 清单

| 技术 | 目的 | v6.6 注意事项 |
|------|------|--------------|
| **`mmap` 环形缓冲** | 订单/行情零拷贝跨线程 | 独立 VMA + `PR_SET_VMA_ANON_NAME` 便于审计 |
| **`MAP_SHARED`** | 多进程共享 state | 匿名 `MAP_SHARED` 走 `shmem_zero_setup()`（tmpfs 后备） |
| **`mlock` / `MAP_LOCKED`** | 禁止 swap，避免缺页尖刺 | 受 `RLIMIT_MEMLOCK` 限制（见 15.6）；`CAP_IPC_LOCK` 可绕过 |
| **`MAP_HUGETLB` / hugetlbfs** | 2MB/1GB 大页，TLB miss ↓ | 需预分配 pool；与 THP 二选一 |
| **`MAP_POPULATE`** | 启动阶段把页表填完 | 走 `__mm_populate()`；见 15.6 |
| **`MADV_POPULATE_WRITE`** | 事后预取页（v5.14+） | 比手写"遍历写一遍"更省：不产生脏页 |
| **`MADV_DONTFORK`** | 防 fork 时子进程 COW 整块 | 见 15.6 |
| **关 THP / 用 hugetlb** | 消除 khugepaged 与规整引入的抖动 | 见 15.7 |
| **绑核 + 单进程** | 减少 CR3 切换与 TLB shootdown | 见 15.2 |
| **别开 LA57** | 少一级页表 walk | 除非真需要 >128TB |

---

→ [01 CSAPP Ch9 VM](../../../02-computer-systems/chapter-09-virtual-memory/) · [06 Gorman Ch4 进程地址空间](../../../06-linux-mm/chapter-04-process-address-space/) · [Ch 3 fork/COW](../../chapter-03-process-management/) · [Ch 15.7 页表](./section-15.7-页表.md)


<details>
<summary>自测题（点击展开）</summary>

**Q1.** 虚拟地址和物理地址的关系？为什么需要虚拟地址？

<details><summary>答案</summary>

虚拟地址(VA) → 页表 → 物理地址(PA)。需要 VA 因为：1) 进程隔离（每进程独立地址空间，互不干扰）；2) 内存超用（物理不够时换出到磁盘）；3) 简化链接/加载（统一地址空间布局）；4) 共享内存（多进程映射同一物理页）。HFT 关心 VA→PA 翻译延迟：TLB miss → page table walk → ~100ns。Huge Page 减少 TLB miss。

**按 v6.6 修订/补充**：
- 「VA → 页表 → PA」中间还夹着 **VMA** 这一层：`mmap` 只建 VMA 不建页表项，
  页表由**缺页异常**按需建立。所以"mmap 成功"≠"内存已可用"。
- 「~100ns」是**4 级页表且全部 cache miss** 的量级。开 LA57 会变成 5 次访存；
  2MB 大页只需 3 级（少一次），1GB 大页只需 2 级。
- 补充第 5 个理由：**非规范地址洞**提供了一类"硬件保证必然 #GP"的地址，
  内核用它做 `access_ok()` 之外的第二道防线。

</details>


**Q2.** x86_64 上用户空间到底有多大？为什么有的文档说 128TB、有的说 64PB、有的说 128PB？

<details><summary>答案</summary>

三者说的不是同一件事：

| 说法 | 指什么 | 出处 |
|------|--------|------|
| **128 TB** | Linux 在 **4 级页表**下给进程的上限 | `DEFAULT_MAP_WINDOW = (1UL<<47) - PAGE_SIZE` |
| **64 PB** | Linux 在 **5 级页表（LA57）**下给进程的上限 | `task_size_max()` 的 large 立即数 `(1ul<<56)-PAGE_SIZE` |
| **128 PB** | **硬件** 5 级页表支持的线性地址空间（57 位 = 2^57 = 128PB） | Intel/AMD 手册 |

Linux 只把 **56 位**给用户（位 56 用作内核/用户的分隔保护），所以进程可见上限是 64PB 而非 128PB。

`task_size_max()` 用 `alternative_io` 在启动时按 `X86_FEATURE_LA57` 把 `movq $imm, %0` 改写掉，
**热路径没有分支**。

</details>


**Q3.** 为什么内核空间在所有进程里是"同一份"？切换进程时内核地址不会变吗？

<details><summary>答案</summary>

不会变。每个进程有自己的 PGD，但**内核半区的 PGD 项是从 `swapper_pg_dir`（`init_mm.pgd`）复制的同一批指针**，
指向同一套 PUD/PMD/PTE。

所以：
- 进程 A 和进程 B 的 `0xffffffff81000000`（内核 text）映射到**同一个物理页**；
- 内核代码可以直接解引用内核指针，不必关心 `current` 是谁；
- `fork()` 复制页表时（`copy_page_range`）**只遍历用户空间区间**，内核半区走模板拷贝。

x86_64 版图里，内核半区从 `0xffff800000000000`（-128TB）开始，**所有进程共用**，
只有 `vsyscall` / `cpu_entry_area` 这类 per-CPU 或兼容区是例外（它们也是固定 VA，只是内容 per-CPU）。

</details>


**Q4.** mmap 区和栈之间那个"洞"为什么存在？我要用 `MAP_FIXED` 手动规划地址时该注意什么？

<details><summary>答案</summary>

那个洞是**栈向下增长的安全余量**，由 `mmap_base()`（`arch/x86/mm/mmap.c:82`）算出来：

```
gap = RLIMIT_STACK(默认 8MB) + 栈随机偏移(≤16GB) + stack_guard_gap(1MB)
gap ∈ [128MB, task_size*5/6]        ← 夹逼
mmap_base = PAGE_ALIGN(task_size - gap - ASLR随机(≤1TB))
```

`stack_guard_gap` 是 **1MB**（`mm/mmap.c:2142` 的 `256UL << PAGE_SHIFT`），
作用是让栈向下增长时**先踩到未映射的 guard 区触发 SIGSEGV**，而不是静默覆盖下面的 mmap 区。
它同时用于 `expand_downwards()` 的检查：新地址与下一个 VMA 之间必须留出这个间隔。

`MAP_FIXED` 规划地址的注意事项：
1. **别往栈下方 128MB 内放东西**——那片是留给栈增长的，你放进去后栈增长会撞上你的 VMA；
2. `MAP_FIXED` 会**静默覆盖**已有映射（先 `do_vmi_munmap`），
   想要"占用就失败"请用 `MAP_FIXED_NOREPLACE`（返回 `-EEXIST`，见 15.6）；
3. 地址必须 **页对齐** 且 `< TASK_SIZE`；
4. 别依赖具体数值——**先关掉 ASLR**（`setarch -R` 或 `personality(ADDR_NO_RANDOMIZE)`）再规划，
   否则 mmap_base 每次运行都不一样（≤1TB 抖动）。

</details>


**Q5.** 「地址空间」和「页表」是一回事吗？`mmap(1GB)` 之后 RSS 为什么还是 0？

<details><summary>答案</summary>

不是一回事。这是本篇最核心的一点：

- **地址空间（VMA 集合）** = **承诺**：这段 VA 归你，权限是 R/W/X，越权就 SIGSEGV。
- **页表** = **兑现**：这段 VA 具体指向哪个物理页。

`mmap(1GB, MAP_ANONYMOUS|MAP_PRIVATE)` 只做了前者：
`mmap_region()` 里分配一个 VMA、插进 `mm_mt`、更新 `mm->total_vm`，
**一个 PTE 都没建**，物理页一个没分配 → RSS 不变。

首次访问时：
```
CPU: 访问 VA → TLB miss → walk 页表 → PTE 的 Present 位为 0 → #PF（缺页异常）
内核: do_user_addr_fault() → handle_mm_fault() → __handle_mm_fault()
      → do_anonymous_page() → alloc_zeroed_user_highpage_movable() → 填 PTE
用户: 指令重跑，这次命中
```

所以 1GB 的 mmap = 潜在的 **262144 次缺页中断**。
HFT 必须在启动阶段用 `MAP_POPULATE` / `MADV_POPULATE_WRITE` / `mlockall(MCL_CURRENT)` 提前兑现，
把这笔开销挪到开盘之前。

反过来也成立：**`munmap` 之后 RSS 立刻掉**，因为页表被拆、页被回收；
而 **`madvise(MADV_DONTNEED)` 之后 VMA 还在、页表没了**，下次访问重新走一次缺页（匿名区会得到**新的零页**）。

</details>

</details>
---
