## ⑪ 分配选型速查

内核分配 **没有万能 malloc** — 按 **大小、连续性、上下文、性能** 选型。本表作 **Ch 12 总复习 + 驱动/HFT 决策树**。

> **版本前提**：所有分界数字与 API 行为均基于 **v6.6 源码实证**
> （`mm/slab.h`、`mm/util.c`、`include/linux/{slab,gfp_types,sched/mm,highmem}.h`）。
> 本页在书的基础上新增了三个 v6.6 上**必须知道**的现代工具：
> **`kvmalloc()`**、**`memalloc_*_save()` 作用域 API**、**`__GFP_NOFAIL` 的正确用法**。

#### 主决策表

| 需求 | 首选 API | 避免 |
|------|----------|------|
| **若干连续物理页** | `alloc_pages` / `__get_free_pages` | `vmalloc` 再逐页查 PA |
| **小对象、物理连续、快** | **`kmalloc`** | 中断里 `GFP_KERNEL` |
| **中断 / softirq / 持 spinlock** | **`kmalloc(..., GFP_ATOMIC)`** 或 **预分配池** | 任何可能 **睡眠** 的路径 |
| **固定类型、高频 alloc/free** | **`kmem_cache_*`** | 裸 `kmalloc` 相同 size |
| **大小不确定、可能很大** | ⭐ **`kvmalloc` / `kvzalloc` + `kvfree`** | 自己写"先试 kmalloc 再 vmalloc"的 if |
| **大块、仅内核访问、非热点** | **`vmalloc` / `vzalloc`** | DMA 缓冲 |
| **设备 DMA 一致映射** | **`dma_alloc_coherent`**（驱动 API） | `vmalloc` + 手工 PA |
| **每核私有、高频写** | **per-CPU**（见 [12.10](./section-12.10-每个-CPU-的分配.md)） | 全局 `atomic_t` |
| **HIGHMEM 页内核访问** | ⚠️ **`kmap_local_page()`**（短） / `kmap()`（可睡） | ~~`kmap_atomic`~~（已废弃，见 [12.9](./section-12.9-高端内存的映射.md)） |
| **网络收包路径的小块临时内存** | **`page_frag_alloc()`**（`gfp.h:311`） | 每包 `kmalloc` |
| **栈上临时** | **仅几百字节内**（见 [12.8](./section-12.8-在栈上的静态分配.md)） | 大数组 |
| **一段代码里全部禁 I/O / 禁 FS** | ⭐ **`memalloc_noio_save()` / `memalloc_nofs_save()`** | 逐层函数签名里传 `GFP_NOIO`/`GFP_NOFS` |

#### 按大小（v6.6 精确分界，不再是"经验值"）

```c
/* mm/slab.h:309-327 */
#define KMALLOC_SHIFT_HIGH	(PAGE_SHIFT + 1)          /* = 13 */
#define KMALLOC_SHIFT_MAX	(MAX_ORDER + PAGE_SHIFT)  /* = 10 + 12 = 22 */
#define KMALLOC_MAX_SIZE	(1UL << KMALLOC_SHIFT_MAX)       /* 4 MB */
#define KMALLOC_MAX_CACHE_SIZE	(1UL << KMALLOC_SHIFT_HIGH)   /* 8 KB */
```

| 请求大小 | 实际路径 | 说明 |
|----------|---------|------|
| **≤ 8 KB** | **SLUB 的 `kmalloc-N` 分档缓存**（含 96/192 两个非 2 的幂档） | 走 per-CPU freelist，快路径一条原子指令 |
| **> 8 KB** | **直接 `alloc_pages()`**（buddy），**不进 slab** | 见 [12.4 Q5](./section-12.4-获得页.md) |
| **8 KB ~ 4 MB** | `kmalloc` 仍可工作（内部转 buddy），但**高阶分配易失败** | 要 4MB 连续页，显式 `alloc_pages(gfp, 10)` 更直白 |
| **> 4 MB** | 只能 **`vmalloc`** 或 **`kvmalloc`** | `KMALLOC_MAX_SIZE` = 4MB 是硬上限（`MAX_ORDER` 默认 10） |
| **> INT_MAX (2GB)** | **`kvmalloc` 会 WARN 并返回 NULL**（`mm/util.c`） | "Don't even allow crazy sizes" |
| **2MB / 1GB 大页** | 用户态 **hugetlb / THP**；内核态 `alloc_pages` 高阶或 **CMA** | 内核里没有"1GB 连续分配"这种事 |

> ⚠️ **订正原表**：原稿写"≤512B～几 KB → Slab；~128KB～几 MB → kmalloc 上限"——
> 分界线其实是**确定的 8KB**（`KMALLOC_MAX_CACHE_SIZE`），
> 而"128KB"那一档早已越过 slab 缓存，**直接就是伙伴系统**。

#### 按上下文

| 上下文 | 可用 gfp | 不可用 |
|--------|----------|--------|
| **进程上下文，无锁** | `GFP_KERNEL` | — |
| **进程 + spinlock** | **`GFP_ATOMIC`** | `GFP_KERNEL` |
| **hardirq / timer callback** | **`GFP_ATOMIC`** + 预池 | `GFP_KERNEL`、`kmap()`、`vmalloc()`、`alloc_percpu()` |
| **softirq** | 同 atomic；仍须 **短** | 睡眠 |
| **文件系统/存储栈内部** | `GFP_NOFS`（或用 **`memalloc_nofs_save()`** 包住整段） | `GFP_KERNEL`（会递归回到 FS 触发死锁） |
| **块设备/网络驱动回收路径** | `GFP_NOIO`（或 **`memalloc_noio_save()`**） | `GFP_KERNEL`（递归触发 I/O） |
| **绝不允许失败且确有失败策略** | ⚠️ 见下面 §5 | 滥用 `__GFP_NOFAIL` |

```c
/* 现代写法：用"作用域"而不是逐层传 gfp（include/linux/sched/mm.h:320/341） */
unsigned int flags = memalloc_nofs_save();     /* 本段内所有分配隐含 GFP_NOFS */
    ...  这一整段里的 kmalloc/alloc_pages 都自动降级 ...
memalloc_nofs_restore(flags);
```

> **为什么这比传 `GFP_NOFS` 好**：`GFP_NOFS` 的语义是"**这个调用链上所有分配**都不能进 FS"，
> 但 C 语言没有"上下文继承"，你只能**给每个函数加个 gfp 参数一路传下去**，漏一个就是死锁。
> `memalloc_*_save()` 把它变成 **当前任务的一个标志位**（`PF_MEMALLOC_NOFS`/`PF_MEMALLOC_NOIO`），
> 源码注释说 "This function is safe to be used from **any context**"。

#### ⭐ `kvmalloc()`：先试 kmalloc，失败回落 vmalloc（v6.6 `mm/util.c` 实证）

```c
void *kvmalloc_node(size_t size, gfp_t flags, int node)
{
	gfp_t kmalloc_flags = flags;
	void *ret;

	if (size > PAGE_SIZE) {
		kmalloc_flags |= __GFP_NOWARN;
		if (!(kmalloc_flags & __GFP_RETRY_MAYFAIL))
			kmalloc_flags |= __GFP_NORETRY;
		/* nofail semantic is implemented by the vmalloc fallback */
		kmalloc_flags &= ~__GFP_NOFAIL;
	}

	ret = kmalloc_node(size, kmalloc_flags, node);
	if (ret || size <= PAGE_SIZE)
		return ret;

	/* non-sleeping allocations are not supported by vmalloc */
	if (!gfpflags_allow_blocking(flags))
		return NULL;

	if (unlikely(size > INT_MAX)) {
		WARN_ON_ONCE(!(flags & __GFP_NOWARN));
		return NULL;
	}
	return __vmalloc_node_range(size, 1, VMALLOC_START, VMALLOC_END,
			flags, PAGE_KERNEL, VM_ALLOW_HUGE_VMAP,
			node, __builtin_return_address(0));
}

void kvfree(const void *addr)          /* 释放：靠地址判断当初走的是哪条路 */
{
	if (is_vmalloc_addr(addr)) vfree(addr);
	else                       kfree(addr);
}
```

| 行为 | 说明 |
|------|------|
| **先试物理连续** | 注释："attempt a large physically contiguous block first because it is **less likely to fragment** multiple larger blocks" —— **优先 kmalloc 是为了减少长期碎片** |
| **回落的 kmalloc 尝试被"弱化"** | `>PAGE_SIZE` 时自动加 `__GFP_NOWARN` + `__GFP_NORETRY`：**失败了不许报警、不许死磕**（因为还有 vmalloc 兜底） |
| **`__GFP_NOFAIL` 被清掉** | 注释："nofail semantic is implemented by the **vmalloc fallback**" |
| ⚠️ **`GFP_ATOMIC` + 大块 = 必然失败** | `if (!gfpflags_allow_blocking(flags)) return NULL;` —— **vmalloc 要建页表，不能在原子上下文做**，所以 `kvmalloc(1MB, GFP_ATOMIC)` 在 kmalloc 失败后**直接返回 NULL** |
| `>INT_MAX` 直接拒绝 | "Don't even allow crazy sizes" + `WARN_ON_ONCE` |
| **`kvfree()` 自动分派** | `is_vmalloc_addr()` 判断当初走的是 slab/buddy 还是 vmalloc —— 与 [12.7 中 `kfree()` 用 `folio_test_slab()` 反查](./section-12.7-Slab-层.md)是**同一种"靠地址反推归属"的思路** |

> **什么时候用 `kvmalloc`**：**"大小由用户输入/配置决定，可能是 1KB 也可能是 10MB"** 这类场景
> （典型的：哈希表、数组、sysfs/配置缓冲区）。
> 它的价值是：小的时候拿到**物理连续**（快），大的时候**自动降级**（不失败），
> 调用方**不用写分支**。

#### ⚠️ `__GFP_NOFAIL` 的正确用法（v6.6 文档原话）

```
%__GFP_NOFAIL: The VM implementation _must_ retry infinitely: the caller
cannot handle allocation failures. The allocation could block
indefinitely but will never return with failure. Testing for
failure is pointless.
New users should be evaluated carefully (and the flag should be
used only when there is no reasonable failure policy) but it is
definitely preferable to use the flag rather than opencode endless
loop around allocator.
Using this flag for costly allocations is _highly_ discouraged.
                                        —— include/linux/gfp_types.h:198
```

| 说法 | 结论 |
|------|------|
| "`__GFP_NOFAIL` 不该用" | ❌ 文档说：**比自己写 `while (!ptr) ptr = kmalloc(...)` 循环更好** |
| "`__GFP_NOFAIL` 万无一失" | ❌ 它 **可能无限阻塞**，且**测试返回值毫无意义** |
| "大分配也能用" | ❌ 文档明确 **"highly discouraged"** |

> **判据**：只有**小额、无替代失败策略**的分配（且调用者真的无法处理 NULL）才用；
> 大块分配请走 **`kvmalloc` + 显式失败处理**。

#### ASCII 决策流（v6.6 版）

```
需要分配？
    │
    ├─ 大小在编译期不定、可能很大？ ────────────► kvmalloc + kvfree
    │                                        （注意：GFP_ATOMIC 时大块必然失败）
    │
    ├─ 固定类型、极高频？ ───────────────────► kmem_cache_alloc / free
    │
    ├─ 每核计数 / 队列 / 临时缓冲？ ──────────► per-CPU（alloc_percpu 会睡，启动时做）
    │
    ├─ 要物理连续？
    │     ├─ 整页 / DMA？ ─────────────────► alloc_pages / dma_alloc_coherent
    │     └─ 字节、≤8KB？ ─────────────────► kmalloc（看 gfp；分档见 12.7）
    │
    ├─ 大块、不要求 PA 连续、可睡眠？ ────────► vmalloc / vzalloc
    │
    └─ 网络收包的小块临时内存？ ─────────────► page_frag_alloc（page_frag_cache）
```

#### 反模式检查表（写驱动时逐条过）

| 反模式 | 后果 | 正解 |
|--------|------|------|
| 中断里 `kmalloc(GFP_KERNEL)` | 可能睡眠 → **BUG / 死锁** | `GFP_ATOMIC` 或**预分配池** |
| 忘了检查 `kmalloc` 返回 NULL | 空指针解引用 → panic | 一律判 NULL（`GFP_ATOMIC` 尤其容易失败） |
| 用 `vmalloc` 的返回值做 DMA | vmalloc 页**物理不连续**，且需要 `page_address` 逐页查 | `dma_alloc_coherent` / `kmalloc` |
| `kvmalloc(10MB, GFP_ATOMIC)` | **必然返回 NULL**（vmalloc 不能用于非睡眠上下文） | 原子上下文**只能预分配** |
| 在 FS 栈里用 `GFP_KERNEL` | 回收路径递归回 FS → **死锁** | `GFP_NOFS` 或 `memalloc_nofs_save()` |
| 在中断里 `alloc_percpu()` | 会睡眠（mutex） | 初始化时分配 |
| 用 `kmap_atomic` 写新代码 | 已废弃，且 RT 上会关迁移/抢占 | `kmap_local_page()` |
| 栈上 `char buf[4096]` | 触发 guard page fault（或静默破坏） | `kmalloc` / per-CPU 缓冲 |
| 热路径每次 `kmalloc` | 尾延迟尖刺（慢路径可能微秒级） | **启动时分配 + 运行时复用** |

#### HFT / 低延迟清单

| # | 原则 |
|---|------|
| 1 | **启动期** 完成所有 **`kmalloc` / `alloc_pages` / Slab create / `alloc_percpu`** |
| 2 | **数据面** 仅 **cache hit / ring pop** — **零 `GFP_ATOMIC`** |
| 3 | **NUMA** — 网卡所在 node **`set_mempolicy` / 驱动 local alloc** |
| 4 | **测失败路径** — `GFP_ATOMIC` 耗尽时 **丢包 vs 延迟尖刺**，两者必须二选一并写进设计 |
| 5 | 用户态 **mlock + hugepage** 与内核 **CMA/reserve** 对称规划 |
| 6 | **分配/释放同核** — 跨核释放会打掉 SLUB 与页分配器 pcp 的快路径（12.4 / 12.7） |
| 7 | **收包路径** 用 `page_frag_alloc` / skb 复用池，避免每包一次分配器往返 |

→ [Ch 12 各节](./section-12.1-为何内核内存更复杂.md) · [06 Gorman 全书索引](../../../06-linux-mm/) · [Ch 15 用户 mmap](../../chapter-15-process-address-space/) · [14 HFT Practice](../../../14-hft-engineering/)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 总结：内核中需要分配 200 字节、4MB、100MB 分别用什么？

<details><summary>答案</summary>

200 字节 → kmalloc(200, GFP_KERNEL)，从 kmalloc-256 slab 分配，O(1)。4MB → alloc_pages(GFP_KERNEL, 10)（2^10=1024 页=4MB），从 buddy 系统分配，物理连续。100MB → vmalloc(100MB)，虚拟连续物理可不连续，需改页表。选型口诀：小用 kmalloc、大且连续用 alloc_pages、大且不连续用 vmalloc。

> **按 v6.6 补充**：
> - **200 字节**：落到 **`kmalloc-256`** 是对的（v6.6 的分档是
>   `0 / 96 / 192 / 8 /16 /32 /64 /128 /256 /512 /1k /2k /4k /8k`，
>   200 在 193~256 区间）。注意**不是** `-512`，因为 96 和 192 两个中间档把 200 挡在了 256；
> - **4MB**：`alloc_pages(GFP_KERNEL, 10)` 正确，但要意识到
>   `MAX_ORDER` 默认就是 **10**，即 **4MB 是伙伴系统单次分配的理论上限**，
>   而高阶分配在运行时**极易失败**（需要 1024 个连续空闲页）。
>   若只是"想要 4MB 内存"而不要求物理连续，**`kvmalloc`** 更省心；
> - **100MB**：`vmalloc` 正确。但如果是"大小不确定"的通用代码，直接写 **`kvmalloc`** 更好——
>   它先试物理连续（碎片友好），失败才回落 vmalloc，调用方不用写分支。
>
> 另附一条原答案没提的**硬上限**：`kvmalloc` 对 **`size > INT_MAX`** 会
> `WARN_ON_ONCE` 并返回 NULL（源码："Don't even allow crazy sizes"）。

</details>

**Q2.** `kvmalloc(10MB, GFP_ATOMIC)` 在中断上下文里能成功吗？为什么？

<details><summary>答案</summary>

**基本不可能，而且这不是概率问题——源码里明确判掉了。**

`mm/util.c` 的 `kvmalloc_node()` 流程：

1. `size > PAGE_SIZE` → 给 kmalloc 尝试加上 `__GFP_NOWARN | __GFP_NORETRY`，并清掉 `__GFP_NOFAIL`；
2. 调 `kmalloc_node(size, ...)` —— 10MB 走 buddy 高阶分配，**`GFP_ATOMIC` 下几乎必然失败**；
3. 失败后准备回落 vmalloc，但**第一行就是**：
   ```c
   /* non-sleeping allocations are not supported by vmalloc */
   if (!gfpflags_allow_blocking(flags))
           return NULL;
   ```
   即 **非睡眠上下文不允许 vmalloc**，于是**直接返回 NULL**。

**根本原因**：vmalloc 要**分配页 + 建页表 + 可能的 TLB flush**，
这些操作本身**需要睡眠**（`__vmalloc_node_range` 里要 `GFP_KERNEL` 级的页表分配）。

> **实践结论**：**原子上下文里不存在"大块分配"这回事**。
> 要在中断里用 10MB 缓冲，唯一正确的做法是**启动时分配好**（`vmalloc`/`kvmalloc` + 保存指针），
> 中断里只做**读写**。这条是本页 HFT 清单第 1、2 条的直接推论。

</details>

**Q3.** 为什么内核提供了 `memalloc_noio_save()` / `memalloc_nofs_save()`，直接传 `GFP_NOIO` 有什么问题？

<details><summary>答案</summary>

因为 **`GFP_NOIO`/`GFP_NOFS` 的本质是"整段调用链的约束"，而 C 的 gfp 参数只能管"这一次调用"**。

比如文件系统要写回一个 inode，调用链可能是
`ext4_writepages → ... → 某个通用库函数 → kmalloc()`。
中间那一层**根本不知道**自己身处 FS 栈，它只会写 `kmalloc(GFP_KERNEL)`——
于是回收路径被触发 → 又要进文件系统 → **死锁**。

传统解法是给链上每个函数都加一个 `gfp_t` 参数一路传下去，
但**漏一个就是死锁**，而且污染了一堆无关函数的签名。

现代解法（`include/linux/sched/mm.h:320/341`）把它变成**当前任务的一个标志位**：

```c
unsigned int flags = memalloc_nofs_save();   /* 设 current->PF_MEMALLOC_NOFS */
    ...  /* 这一整段里所有分配都自动隐含 GFP_NOFS，无需改任何函数签名 */
memalloc_nofs_restore(flags);
```

源码注释："Marks **implicit** GFP_NOFS allocation scope… This function is **safe to be used from any context**."
`memalloc_noio_save()` 对应 `PF_MEMALLOC_NOIO`（块设备/存储栈同理）。

> 适用判据：**约束来自"我在哪段代码里"，而不是"我这次要分配什么"** 时，用作用域 API；
> 只是一次孤立分配需要特殊 gfp，直接传标志即可。

</details>

**Q4.** 什么时候该用 `__GFP_NOFAIL`？

<details><summary>答案</summary>

v6.6 `include/linux/gfp_types.h:198` 的文档说得很完整，核心四句：

1. **语义**："The VM implementation _must_ **retry infinitely**… The allocation could
   **block indefinitely** but will **never return with failure**. **Testing for failure is pointless**."
2. **准入门槛**："New users should be **evaluated carefully** (and the flag should be used
   **only when there is no reasonable failure policy**)."
3. **反直觉的一条**：它"is **definitely preferable** to use the flag rather than
   **opencode endless loop** around allocator" —— 也就是说，
   与其自己写 `while (!(p = kmalloc(...)));` 这种死循环，**不如用这个标志**；
4. **禁区**："Using this flag for **costly allocations** is _highly_ discouraged."

**判据**：
- ✅ **小额**（页级以内）+ **调用者确实无法处理 NULL**（无处返回错误码的上下文）；
- ❌ **大块分配**——大块本来就难以满足，"无限重试"可能真的**无限**；
- ❌ **有替代路径时**——能降级、能丢包、能返回 `-ENOMEM` 就别用；
- 💡 想要"尽量成功但有兜底"，用 **`kvmalloc` + 显式判 NULL**，
  它在内部处理了"先试连续、再回落 vmalloc"的逻辑。

</details>

**Q5.** 网络驱动收包路径要一块 1.5KB 的临时缓冲，有哪几种做法？延迟上怎么排？

<details><summary>答案</summary>

| 做法 | 延迟特征 | 适用 |
|------|---------|------|
| **`page_frag_alloc(&nc, 1536, gfp)`** | **最快**：从 per-CPU 的 `page_frag_cache` 里**切一段**，绝大多数情况下**连分配器都不进**（只是挪一个 offset） | ✅ **网络收包首选**（`gfp.h:311`；`sk_buff` 的头部数据就是这么来的） |
| **skb 复用 / napi 回收环** | 快：对象池命中 | ✅ 驱动与协议栈的常规做法 |
| **`kmem_cache_alloc(cache, GFP_ATOMIC)`** | 中：SLUB 快路径一条 `cmpxchg16b`（命中 `c->slab` 时）；**跨 CPU 释放会退化到慢路径** | ✅ 固定类型的专用 cache |
| **`kmalloc(1536, GFP_ATOMIC)`** | 中：同上（走 `kmalloc-2k` 档） | ⚠️ 可以，但每包都做就是尾延迟源 |
| **局部数组 `char buf[1600]`** | 最快（零分配）**但很危险**：吃掉 16KB 内核栈的 10% | ❌ 别在深调用链里用（见 12.8） |
| **`vmalloc` / `kvmalloc`** | 极慢且 **`GFP_ATOMIC` 下直接失败** | ❌ 完全不适用 |

> **排序背后的原理**：`page_frag_alloc` 赢在它**根本不碰分配器**——
> 它把一页（或一个高阶块）切成若干 frag，per-CPU 缓存，**取一段只是指针运算**。
> 只有在 **frag cache 耗尽**时才真的走一次 `alloc_pages`。
> 这就是"**批分配 + 切分**"的威力，与 12.4 页分配器的 pcp、
> 12.7 的 SLUB per-CPU freelist 是**同一个思想的不同尺度**：
> **把昂贵的全局操作批量化，让热路径只做 O(1) 的本地操作**。
>
> HFT 上完全可以直接照搬：**每核预取一大块内存，热路径上只挪 offset**。

</details>

</details>
---
