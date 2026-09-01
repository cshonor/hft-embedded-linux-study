## ⑨ 高端内存的映射 · High Memory

在 **HIGHMEM zone** 分配的页 **没有** 永久 **内核线性映射地址** — CPU 要访问页内数据，必须先 **临时映射** 到 **PKMAP / fixmap 槽**。

> **版本前提**：本节机制基于 **v6.6 源码实证**
> （`include/linux/highmem.h`、`mm/highmem.c`、`include/linux/mmzone.h`、
> `arch/x86/include/asm/{highmem,fixmap,pgtable_32_areas}.h`、`include/asm-generic/kmap_size.h`、
> `arch/x86/Kconfig`、`arch/arm64/Kconfig`）。
> ⚠️ **最大的认知更新**：`kmap_atomic()` 在 v6.6 已**明确废弃**，
> 现代替代品是 **`kmap_local_page()`**（v5.11 引入）。书上（LKD 3rd）的 `kmap_atomic` 用法今天应改写。

#### 1. 问题从哪来（x86 32 位典型）

| 事实 | 数值/后果 |
|------|-----------|
| 内核 **直接映射窗口** | 约 **896MB** 物理 @ `PAGE_OFFSET` |
| 超过部分 | **ZONE_HIGHMEM** — **PA 存在，无固定 `page_address()`** |
| 64 位 | 通常 **全 direct map** — 本节作 **概念 + ARM32 / x86_32** 参考 |

**谁真的有 HIGHMEM（v6.6 Kconfig 实证）**：

| 架构 | 有 `ZONE_HIGHMEM`？ | 证据 |
|------|---------------------|------|
| **x86_32** | ✅ | `arch/x86/Kconfig:1444` — `config HIGHMEM` / `depends on X86_32 && (HIGHMEM64G \|\| HIGHMEM4G)`，默认 `HIGHMEM4G` |
| **x86_64** | ❌ | 同上 `depends on X86_32`，64 位根本不进这个分支 |
| **arm64** | ❌ | `arch/arm64/Kconfig` 全文 **没有任何 HIGHMEM 条目** |
| **ARM32** | ✅ | 32 位 ARM 支持 `CONFIG_HIGHMEM`（LPAE 下同样需要） |

`ZONE_HIGHMEM` 在通用代码里也是**条件编译**的（`mmzone.h:743`）：

```c
#ifdef CONFIG_HIGHMEM
	/*
	 * A memory area which is only addressable by the kernel through
	 * mapping portions into its own address space. ...
	 * (i386 上内核会为它要访问的每一页建立专门映射)
	 */
	ZONE_HIGHMEM,
#endif
```

#### 2. 两套映射窗口：PKMAP（持久）vs fixmap（临时）

原笔记把两者笼统写成"fixmap / kmap 槽"，v6.6 上它们**是两个完全不同的区域**。
`arch/x86/include/asm/highmem.h` 里的布局注释（逐字）：

```
high memory on:                      high memory off:
   FIXADDR_TOP                          FIXADDR_TOP
       fixed addresses                      fixed addresses
   FIXADDR_START                       FIXADDR_START
       temp fixed addresses /            VMALLOC_END
       persistent kmap area                 temp fixed addresses / vmalloc area
   PKMAP_BASE                          VMALLOC_START
   VMALLOC_END                             vmalloc area
       vmalloc area                    high_memory
   VMALLOC_START
   high_memory
```

| 窗口 | 服务对象 | 槽位数（v6.6 实证） | 生命周期 |
|------|---------|-------------------|---------|
| **PKMAP**（持久映射区） | **`kmap()` / `kunmap()`** | `LAST_PKMAP` = **512**（`CONFIG_X86_PAE`）或 **1024**（非 PAE） —— `pgtable_32_areas.h:22` | **全局**，跨上下文有效，直到 `kunmap()` |
| **fixmap / KMAP_LOCAL**（临时固定映射） | **`kmap_local_page()`、`kmap_atomic()`** | **`KM_MAX_IDX * NR_CPUS`**：`KM_MAX_IDX` = **16**（`CONFIG_DEBUG_KMAP_LOCAL` 时为 **33**，多出的用于 guard page） —— `kmap_size.h`；槽位范围 `FIX_KMAP_END = FIX_KMAP_BEGIN + (KM_MAX_IDX * NR_CPUS) - 1`（`fixmap.h:103`） | **栈式**，per-CPU，函数内临时 |

```
PKMAP 区（全局共享，kmap 用）          fixmap KMAP 区（每 CPU 一段，kmap_local 用）
┌──┬──┬──┬──┬──┬──┬──┐             ┌──────────┬──────────┬──────────┐
│ 0│ 1│ 2│ 3│..│  │51│             │ CPU0 ×16 │ CPU1 ×16 │ CPU2 ×16 │
└──┴──┴──┴──┴──┴──┴──┘             └──────────┴──────────┴──────────┘
   ↓ 一个全局自旋锁 kmap_lock          ↓ 无锁：每 CPU 有自己的 16 个栈槽
   耗尽 → 睡眠等待（可能阻塞！）        耗尽 → BUG（映射深度超限，说明代码写错了）
```

#### 3. 三代 API：kmap → kmap_atomic → kmap_local_page

| API | 引入 | 上下文 | 睡眠？ | 指针能跨上下文传？ | 现状 |
|-----|------|--------|--------|------------------|------|
| **`kmap()`** | 最早 | **只能可抢占的任务上下文** | **可能睡眠**（槽位耗尽时） | ✅ **全局可见**，可交给别的上下文 | 仍在，但仅限进程上下文 |
| **`kmap_atomic()`** | 老接口 | 任何上下文（含中断） | ❌ | ❌ | ⚠️ **已废弃**（v5.11 起） |
| **`kmap_local_page()`** | **v5.11** | **任何上下文，包括中断** | ❌ | ❌ 只在调用者上下文有效 | ✅ **新代码应使用它** |

**v6.6 `include/linux/highmem.h` 的原文要点**（逐条对照源码注释）：

`kmap()`（:22-34）：
> "Can only be invoked from **preemptible task context** because on 32bit systems with
> CONFIG_HIGHMEM enabled this function **might sleep**." …
> "For highmem pages on 32bit systems this can be **slow** as the mapping space is
> **limited and protected by a global lock**. In case that there is no mapping slot
> available the function **blocks until a slot is released** via `kunmap()`."

`kmap_local_page()`（:74-95）：
> "Can be invoked from **any context, including interrupts**." …
> "Requires careful handling when **nesting multiple mappings because the map management
> is stack based**. The unmap has to be in the **reverse order**." …
> "the mapping is only valid **in the context of the caller** and **cannot be handed to
> other contexts**." …
> "On HIGHMEM enabled systems mapping a highmem page has the side effect of
> **disabling migration** … **No caller of kmap_local_page() can rely on this side effect.**"

`kmap_atomic()`（:146-149）—— 这就是"废弃"的实锤：
> "**Atomically map a page for temporary usage - Deprecated!** …
> In fact a **wrapper around kmap_local_page()** which also **disables pagefaults** and,
> depending on PREEMPT_RT configuration, also CPU migration and preemption. …
> **Do not use in new code. Use kmap_local_page() instead.**"

```
调用关系（v6.6）
    kmap_atomic(page)  ──►  kmap_local_page(page)  + pagefault_disable()
                            （RT 上还 migrate_disable()/preempt_disable()）
```

> **一个常见误解的纠正**：很多人以为 `kmap_atomic` 里的 "atomic" 是指"原子指令"。
> 源码注释说得很清楚——它只是 **`kmap_local_page()` 外面套一层"关页错误"**。
> 真正的"原子性"来自 **关抢占/关迁移** 这个副作用，而这个副作用
> **在 PREEMPT_RT 上才会出现**（非 RT 上只关 pagefault），**所以任何代码都不该依赖它**。

#### 4. `kmap()` 的槽位耗尽路径（PKMAP 的三态计数）

```c
/* mm/highmem.c:44-50 注释逐字 */
 * Virtual_count is not a pure "count".
 *  0 means that it is not mapped, and has not been mapped
 *    since a TLB flush - it is usable.
 *  1 means that there are no users, but it has been mapped
 *    since the last TLB flush - so we can't use it.
 *  n means that there are (n-1) current users of it.
static int pkmap_count[LAST_PKMAP];
static  __cacheline_aligned_in_smp DEFINE_SPINLOCK(kmap_lock);
```

| `pkmap_count[i]` | 含义 | 能立刻复用吗 |
|------------------|------|-------------|
| **0** | 未映射，且自上次 TLB flush 起没被用过 | ✅ 可用 |
| **1** | **没人用了，但自上次 flush 起被映射过**（TLB 里可能还有残留项） | ❌ 得先 flush |
| **n (>1)** | 有 **n-1** 个当前用户 | ❌ |

`map_new_virtual()`（:234）的完整流程：

```
扫描 LAST_PKMAP 个槽找 pkmap_count == 0
   │
   ├─ 绕回 nr == 0（no_more_pkmaps）→ flush_all_zero_pkmaps()
   │        └─ 对所有 count==1 的槽：清 PTE → flush_tlb_kernel_range(PKMAP_ADDR(0),
   │                                          PKMAP_ADDR(LAST_PKMAP)) → count 归 0
   │
   ├─ 仍找不到 → 睡！DECLARE_WAITQUEUE + TASK_UNINTERRUPTIBLE + schedule()
   │        └─ 醒来后先检查 page_address(page)（可能别人已经映射好了）→ 否则 goto start
   │
   └─ 找到 → set_pte_at(pkmap_page_table[nr], mk_pte(page, kmap_prot))
             + set_page_address(page, vaddr)，count = 1
```

> **这就是 `kmap()` 会睡眠、且"慢"的全部原因**：
> 一个 **全局自旋锁 `kmap_lock`** + 一个**有限槽位池** + 一条**可能挂起等待**的路径。
> 书上说的"kmap 可能阻塞"，机制就在这里。

#### 5. 使用规则（栈式 + 逆序 + 传返回值）

```c
/* ✅ 现代写法（v5.11+ 推荐） */
void *vaddr = kmap_local_page(page);
memcpy(vaddr, src, PAGE_SIZE);
kunmap_local(vaddr);                 /* 传的是 kmap 的返回值 */

/* 需要映射两页时：严格嵌套，逆序释放 */
void *v1 = kmap_local_page(page1);
void *v2 = kmap_local_page(page2);
memcpy(v1, v2, PAGE_SIZE);
kunmap_local(v2);                    /* 后映射的先释放 */
kunmap_local(v1);
```

| 规则 | 违反后果 |
|------|---------|
| **必须逆序 unmap**（栈式管理） | "Unmapping addr1 before addr2 is invalid and causes **malfunction**"（源码原话） |
| **`kunmap_local/atomic` 传返回值，不是 page** | 源码专门提醒："the `kunmap_atomic()` call takes the **result of** the `kmap_atomic()` call, **not the argument**" |
| **映射指针不能传给别人**（线程/下半部/别的结构体长期持有） | `kmap_local_page` 的映射**只在调用者上下文有效** |
| **映射期间不能睡眠**（`kmap_local_page` 虽不禁止，但语义上是临时映射） | 睡眠期间该槽被抢占者复用 → 地址失效 |
| **映射深度 ≤ `KM_MAX_IDX`（16）** | 超过就是 BUG（`CONFIG_DEBUG_KMAP_LOCAL` 用 guard page 帮你抓） |

#### 6. 谁能拿到 HIGHMEM 页

| 分配标志 | 会从 HIGHMEM 取页吗 |
|----------|-------------------|
| `GFP_KERNEL` | ❌ **不会** —— 内核自身数据结构要能直接寻址 |
| `__GFP_HIGHMEM` | ✅ |
| `GFP_HIGHUSER` / `GFP_HIGHUSER_MOVABLE` | ✅ —— **用户态页**才是 HIGHMEM 的主要客户（页缓存、匿名页） |

> **推论**：高端内存**不是给内核数据结构用的**，是给**用户态页和页缓存**用的。
> 内核代码里几乎只有在**处理用户页内容**（页缓存读写、clear_page、copy_page、加解密等）
> 时才会碰到 highmem 页，于是才需要 `kmap_local_page()` 临时映射一下。
> 这也解释了为什么 `highmem.h` 里那一大堆 `*_user_highpage` / `copy_highpage` 辅助函数
> 内部清一色是 `kmap_local_page()` + 操作 + `kunmap_local()`。

#### 7. 64 位 / `CONFIG_HIGHMEM=n` 上是空操作

| 条件 | `kmap()` | `kmap_local_page()` |
|------|---------|---------------------|
| `CONFIG_HIGHMEM=n`（x86_64、arm64） | 直接返回 `page_address(page)` | 同上 |
| `CONFIG_HIGHMEM=y` 但页在 lowmem | 同样直接返回直接映射地址 | 同上 |
| `CONFIG_HIGHMEM=y` 且页在 highmem | 走 PKMAP 槽（可能睡眠） | 走 fixmap 槽（栈式，无锁） |

> 源码原话："For systems with **CONFIG_HIGHMEM=n** and for pages in the **low memory area**
> this returns the virtual address of the **direct kernel mapping**."
> 所以 **`kmap_local_page()` 在 64 位上基本是零成本**——可以放心把它当成
> "面向页的通用访问接口"来写可移植代码，代价为零。

#### 8. 嵌入式 ARM32

| 场景 | 做法 |
|------|------|
| **小 RAM SoC** | 可能 **全 LOWMEM**（闪存/内存都小，直接映射窗口够用） |
| **> 直接映射窗口的物理 @ 32-bit kernel** | 驱动 **DMA / 帧缓冲 / 大块用户缓冲** 落在 HIGHMEM — 访问前必须 `kmap_local_page()`，**访问窗口要短** |
| **推荐** | 新设计直接上 **arm64 / 64 位内核**，HIGHMEM 整套机制用不上（arm64 Kconfig 无此项） |
| **已存在的 32 位代码** | 批量把 `kmap_atomic()` 改成 `kmap_local_page()` —— 语义更清楚、开销更低，且**不再依赖"关抢占"这个副作用** |

**HFT：** 现代 **x86-64 / arm64 交易服务器** 几乎 **碰不到 HIGHMEM** — 但 **`kmap_atomic` 思想** 同构于 **「短临界区访问临时缓冲」**。用户态等价：**mmap 大池 + 指针** 即可；内核 HIGHMEM 是 **VA 不够** 时的 **历史包袱**。

> **HFT 补充（把 HIGHMEM 的思想提炼出来）**：
> ① HIGHMEM 的教训是 **"地址空间不够 → 引入映射间接层 → 每次访问多一次映射/解映射 + TLB 失效"**。
> 用户态大内存方案（mmap 巨型池、hugetlb、DAX）本质是**反过来做**：
> 一次性建立映射并**长期持有**，把映射成本**摊销到零**；
> ② `kunmap_local()` 要**逆序**释放这个约束，和 **栈式/arena 分配器**（12.8 的 alloca 类比、
> [12.6 vmalloc](./section-12.6-vmalloc.md) 的 vmap 栈）是同一种"栈式资源管理"思维；
> ③ 最实际的迁移建议：手上有 32 位内核驱动代码在做 `kmap_atomic` 时，
> 换成 `kmap_local_page()`，因为前者在 **PREEMPT_RT 上会关迁移与抢占**——
> 在低抖动场景下，这等于**无谓地牺牲调度自由度**。

→ [06 Gorman Ch9 高端内存](../../../06-linux-mm/chapter-09-high-memory-management/) · [Ch 12.3 Zones](./section-12.3-区.md)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** kmap 和 kmap_atomic 的区别？哪个更高效？

<details><summary>答案</summary>

kmap：可睡眠、用全局锁保护 fixmap 槽、可能阻塞。kmap_atomic：不可睡眠、per-CPU fixmap 槽、无需锁、极快。高频场景用 kmap_atomic（如网络包处理中映射 HIGHMEM 页到内核地址）。x86_64 没有 HIGHMEM，这两个函数是空操作（直接返回 page_address）。

> **按 v6.6 修订**：三处需要更新。
> ① **`kmap_atomic()` 已废弃**（`include/linux/highmem.h:146` 正文标题就写着 "Deprecated!"，
> "Do not use in new code. Use **`kmap_local_page()`** instead"）。
> 它现在只是 `kmap_local_page()` + `pagefault_disable()` 的包装。
> ② **"用全局锁保护 fixmap 槽"说错了窗口**：`kmap()` 用的是 **PKMAP 区**
> （`LAST_PKMAP` 个全局槽，x86 PAE 下 512，非 PAE 下 1024），
> 而临时映射用的是 **fixmap 的 FIX_KMAP 区**（`KM_MAX_IDX × NR_CPUS` 个 per-CPU 槽，KM_MAX_IDX=16）。
> 两个区域、两套机制，不能混说。
> ③ "x86_64 上是空操作"是对的，但**建议照写 `kmap_local_page()`**——
> 源码保证 `CONFIG_HIGHMEM=n` 或 lowmem 页时直接返回直接映射地址，**零成本**，
> 换来的是同一份代码在 32 位上也能编译通过。

</details>

**Q2.** 为什么 x86_64 不需要高端内存？

<details><summary>答案</summary>

x86_64 有 48 位虚拟地址空间（256TB），而物理内存通常 < 256TB。内核直接映射区（PAGE_OFFSET 开始）可以覆盖所有物理内存，不需要临时映射。高端内存是 32 位系统的限制：32 位内核只有 896MB 直接映射区，超过部分需要 HIGHMEM 机制。

> **按 v6.6 补充**：这条可以直接用 Kconfig 坐实——
> `arch/x86/Kconfig:1444` 的 `config HIGHMEM` 写着 `depends on X86_32`，
> 所以 **x86_64 的配置里这个符号根本不存在**，
> `mmzone.h:747` 的 `ZONE_HIGHMEM` 也就被 `#ifdef CONFIG_HIGHMEM` 编译掉。
> **arm64 同样没有**（`arch/arm64/Kconfig` 全文无 HIGHMEM 条目）。
> 所以真正的结论是：**v6.6 上 HIGHMEM 只存在于 32 位架构（x86_32、ARM32 等）**。

</details>

**Q3.** `kmap_local_page()` 返回的指针，能不能存进数据结构、等下一个中断/下半部再用？

<details><summary>答案</summary>

**不能。** v6.6 `include/linux/highmem.h:88` 的注释逐字：
"Contrary to `kmap()` mappings the mapping is **only valid in the context of the caller**
and **cannot be handed to other contexts**."

对比：`kmap()` 的映射是 **PKMAP 区的固定槽位**，注释说 "The returned virtual address is
**globally visible** and valid up to the point where it is unmapped via `kunmap()`.
The pointer **can be handed to other contexts**."——**只有 `kmap()` 的指针能跨界传递**，
代价是它可能睡眠（槽位耗尽时阻塞）且必须显式 `kunmap()`。

另外两个配套约束：
① **栈式管理**：多个映射必须**逆序**释放，先 `addr1` 后 `addr2` 映射，就必须先 `kunmap_local(addr2)`；
② **释放传返回值**：`kunmap_local(addr)` 的参数是 `kmap_local_page()` 的**返回值**，不是 page 指针。

</details>

**Q4.** `kmap()` 在什么情况下会睡眠？内核是怎么处理的？

<details><summary>答案</summary>

只发生在 **32 位 + `CONFIG_HIGHMEM=y` + 要映射的页真在 highmem** 时，
且 **PKMAP 槽位被占满**。路径在 `mm/highmem.c` 的 `map_new_virtual()`：

1. 线性扫描 `pkmap_count[LAST_PKMAP]` 找值为 **0** 的槽；
2. 扫到 `nr == 0`（绕回原点）时触发 `flush_all_zero_pkmaps()`——
   把所有 `count == 1`（"无用户但映射过、TLB 里可能有残留"）的槽清 PTE，
   然后 `flush_tlb_kernel_range(PKMAP_ADDR(0), PKMAP_ADDR(LAST_PKMAP))`，再把这些槽的计数归 0；
3. 仍找不到 → `DECLARE_WAITQUEUE` + `__set_current_state(TASK_UNINTERRUPTIBLE)` +
   `unlock_kmap()` + `schedule()`，**睡在 `pkmap_map_wait` 上等别人 `kunmap()`**；
4. 醒来后先 `if (page_address(page))` 检查（可能别人已经替你映射好了），否则 `goto start` 重来。

计数语义（`mm/highmem.c:44-50` 原文）：**0**=未映射且 flush 过（可用）；
**1**=无用户但自上次 flush 起映射过（**不能直接用**）；**n**=有 n-1 个用户。
整条路径受**全局自旋锁 `kmap_lock`** 保护——这就是"kmap 慢且可能睡眠"的机制全貌。

</details>

**Q5.** 为什么 `kmap_atomic()` 被废弃？它和 `kmap_local_page()` 到底差在哪？

<details><summary>答案</summary>

源码注释（`highmem.h:146-149`）说它是
"a **wrapper around kmap_local_page()** which also **disables pagefaults** and,
depending on **PREEMPT_RT** configuration, also CPU migration and preemption"。
差异只有"副作用"这一层：

| | `kmap_local_page()` | `kmap_atomic()` |
|---|---|---|
| 映射本体 | 同一个（fixmap 栈槽） | 同一个 |
| 关页错误 | ❌ | ✅ `pagefault_disable()` |
| 关迁移/抢占 | ❌ | **只在 PREEMPT_RT 上** ✅ |

被废弃的理由正是这层"副作用的不确定性"：**同一个 API 在不同配置下语义不同**，
于是调用者要么白白付出关 pagefault 的代价，要么（在非 RT 上）**误以为**自己拿到了"原子上下文"。
源码明确写："users should **not count on** the latter two side effects"。

对低延迟/RT 场景尤其重要：在 RT 内核上 `kmap_atomic()` 会**关迁移与抢占**，
等于无谓地牺牲调度自由度；换成 `kmap_local_page()` 就没有这个开销。
迁移时要检查：是否曾**依赖** `kmap_atomic` 期间"不会被抢占"——如果依赖了，那是个 bug，要显式处理。

</details>

**Q6.** 为什么内核自己的数据结构几乎不会分配到 HIGHMEM？

<details><summary>答案</summary>

因为 **gfp 标志决定了 zone**：只有带 **`__GFP_HIGHMEM`** 的请求才会从 `ZONE_HIGHMEM` 取页
（用户态相关组合是 `GFP_HIGHUSER` / `GFP_HIGHUSER_MOVABLE`）。
**`GFP_KERNEL` 不含 `__GFP_HIGHMEM`**，所以内核自身的数据结构
（`task_struct`、`inode`、`sk_buff` 等）**永远落在可直接寻址的 lowmem**。

这是设计上的必然：内核代码访问自己的数据结构时**必须**能直接解引用，
不可能每次访问都 kmap 一次。于是 HIGHMEM 的客户只能是**"不需要内核经常看内容"的页**——
主要是**用户态匿名页与页缓存**。所以你在内核里看到 `kmap_local_page()` 的地方，
几乎都在**处理用户页内容**的路径上：`clear_user_highpage()`、`copy_highpage()`、
页缓存读写、加解密等——`highmem.h` 里那一大族 `*_highpage` 辅助函数内部
清一色是 `kmap_local_page()` + 操作 + `kunmap_local()`。

> 反过来也解释了 **`CONFIG_HIGHMEM=n` 时这些函数就是普通的 `clear_page`/`copy_page`**
> （源码：`/* when CONFIG_HIGHMEM is not set these will be plain clear/copy_page */`）。

</details>

</details>
---
