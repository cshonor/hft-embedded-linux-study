## ⑦ Slab 层 · Slab Layer

内核 **大量固定大小对象**（`task_struct`、`inode`、`dentry`…）若每次走 **通用页分配器**，会 **慢** 且 **外部碎片** 严重 — **Slab 分配器** 在 **页之上** 做 **对象缓存**。

> **版本前提**：本节的机制描述全部基于 **v6.6 源码实证**（`mm/slub.c`、`mm/slab_common.c`、`mm/slab.h`、`include/linux/slub_def.h`）。
> v6.6 里 **SLOB 已被删除**、**SLAB 已标记弃用**（v6.8 彻底删除），所以下文「Slab 层」**默认指 SLUB**。

#### 1. 为什么页分配器不够用

| 问题 | 页分配器（buddy） | Slab 的解法 |
|------|------------------|------------|
| **外部碎片** | 反复分配/释放不同大小 → 空闲页被切碎，高阶分配失败 | 按 **固定大小分档**，同档对象在同一批 slab 页里周转 |
| **初始化摊销** | 每次拿到裸页都要走一遍初始化 | cache 记 **ctor**，对象**只在 slab 首次创建时构造**，之后复用即已初始化状态 |
| **缓存行冲突** | 同类型对象可能挤在同一缓存行/同一组 | **着色（slab coloring）** + `SLAB_HWCACHE_ALIGN` 对齐 |
| **锁竞争** | 分配要碰 `zone->lock` | **per-CPU freelist**：快路径 **一条原子指令，无锁** |

#### 2. 三层结构（心智模型）

```
kmem_cache（一种对象类型）
    │
    ├── slab 1  [满]  无空槽
    ├── slab 2  [半满] ← 优先从这里 alloc
    └── slab 3  [空]   备用
            │
            └── 每个 slab = 若干连续页，切成 fixed-size 槽
```

| 概念 | 说明 |
|------|------|
| **Cache（`kmem_cache`）** | 一种 **对象类型** 一条缓存 — 统一 **构造/析构** |
| **Slab** | 一条 cache 内 **一页或多页** 的块 — **满 / 半满 / 空** 状态 |
| **对象（object）** | 实际 **`kmalloc` 大小的槽** — 带 **着色** 减 cache line 冲突 |

⚠️ **上面是"经典 SLAB"的画法**。v6.6 的 **SLUB** 已经没有"满/半满/空三条链表"了，
它把**队列压到了两级**（per-CPU + per-node），slab 页只在 **frozen / partial / 无主** 三种状态间流动：

```
                        ┌──────────────────────── per-CPU（无锁）────────────────────────┐
kmem_cache_alloc() ──►  │ c->slab  （当前正在分配的 slab，frozen：只有本 CPU 能取对象）    │
                        │    └─ c->freelist：单链表，头指针 + tid 打包成 128 位原子更新   │
                        │ c->partial（本 CPU 的半满 slab 链，上限 cpu_partial_slabs 个）  │
                        └───────────────────────────────────────────────────────────────┘
                              │ 空                              │ 满/超限
                              ▼                                 ▼
                   ┌──── per-node（持 n->list_lock）────┐        直接换一块新 slab
                   │ n->partial：半满 slab 链表         │
                   │ 受 s->min_partial 控制保底数量     │
                   └───────────────────────────────────┘
                              │ 空
                              ▼
                   buddy 分配器 new_slab() → alloc_pages(order = oo_order)
```

> **frozen slab 是 SLUB 无锁的关键**：一块 slab 一旦被某 CPU "冻结"，
> **只有该 CPU 能从它的 freelist 取对象**；其他 CPU **只能往里还对象**（free 走慢路径）。
> 于是"取"这个动作天然无竞争，不需要 slab 锁。

#### 3. 实证：v6.6 的 per-CPU 结构

```c
/* include/linux/slub_def.h:48 —— 改布局时必须保证 freelist 与 tid
 * 仍满足 this_cpu_cmpxchg_double() 的对齐要求 */
struct kmem_cache_cpu {
	union {
		struct {
			void **freelist;	/* Pointer to next available object */
			unsigned long tid;	/* Globally unique transaction id */
		};
		freelist_aba_t freelist_tid;	/* 128 位整体，供 cmpxchg16b 用 */
	};
	struct slab *slab;	/* The slab from which we are allocating */
#ifdef CONFIG_SLUB_CPU_PARTIAL
	struct slab *partial;	/* Partially allocated frozen slabs */
#endif
	local_lock_t lock;	/* Protects the fields above */
#ifdef CONFIG_SLUB_STATS
	unsigned stat[NR_SLUB_STAT_ITEMS];
#endif
};
```

| 字段 | 作用 | 为什么要这样设计 |
|------|------|-----------------|
| `freelist` + `tid` **同一 union** | 打包成一个 **16 字节对齐的整体** | 一次 `cmpxchg16b`（`this_cpu_cmpxchg_double`）**同时**更新指针和版本号 |
| `tid` | 每次成功操作 `next_tid(tid)` | 防 **ABA**：见下节 |
| `slab` | 当前 slab | 决定 fast/slow path 的分支点 |
| `partial` | per-CPU 半满链（`CONFIG_SLUB_CPU_PARTIAL`，默认 y） | 让"本 CPU 刚释放的对象"能**不进全局锁**就被再次分配 |
| `local_lock` | 只保护**慢路径** | 非 RT 上它只关抢占；快路径压根不碰它 |

#### 4. 分配快路径：无锁 cmpxchg_double

```c
/* mm/slub.c:3329 节选 */
	c = raw_cpu_ptr(s->cpu_slab);
	tid = READ_ONCE(c->tid);
	barrier();				/* 必须先读 tid，再读 freelist/slab */
	object = c->freelist;
	slab   = c->slab;

	if (!USE_LOCKLESS_FAST_PATH() ||
	    unlikely(!object || !slab || !node_match(slab, node))) {
		object = __slab_alloc(s, gfpflags, node, addr, c, orig_size);  /* 慢路径 */
	} else {
		void *next_object = get_freepointer_safe(s, object);
		if (unlikely(!__update_cpu_freelist_fast(s, object, next_object, tid))) {
			note_cmpxchg_failure("slab_alloc", s, tid);
			goto redo;		/* 被抢占/中断插队 → 重来，不是加锁 */
		}
		prefetch_freepointer(s, next_object);
		stat(s, ALLOC_FASTPATH);
	}
```

**`tid` 解决的 ABA 问题**（这是 Slab 层最精妙的一处设计）：

```
本 CPU 读到 freelist = A，tid = 7
        │
        ├── 被中断/抢占 ──► 中断里 free 一个对象：freelist 变 B，tid 变 8
        │                    再 alloc 一次：      freelist 又变回 A，tid 变 9
        ▼
回到原路径：cmpxchg_double 期望 (A,7)，实际 (A,9) → 失败 → goto redo
```

> **关键认知**：`freelist` 的值**回到了 A**，肉眼看上去"没变过"——
> 但期间对象 B 已被取走又归还。**只比较指针的 CAS 会误判成功**（经典 ABA）。
> `tid` 把"这段时间内发生过几次操作"编码进去，让 CAS 变成 **指针 + 版本号** 双校验。

**"重来"而不是"加锁"**：`goto redo` 是 SLUB 的核心取舍——
被抢占/中断打断时**不退化成锁**，而是**重新读一次再试**。
代价是最坏情况下多转几圈，收益是**常态下一条原子指令**完成分配。

#### 5. 释放快路径：也靠同一个 `tid` 校验

```c
/* mm/slub.c do_slab_free() 节选 */
	c = raw_cpu_ptr(s->cpu_slab);
	tid = READ_ONCE(c->tid);
	barrier();

	if (unlikely(slab != c->slab)) {	/* 不是本 CPU 当前 slab */
		__slab_free(s, slab, head, tail_obj, cnt, addr);   /* 慢路径：可能跨 CPU 释放 */
		return;
	}
	if (USE_LOCKLESS_FAST_PATH()) {
		freelist = READ_ONCE(c->freelist);
		set_freepointer(s, tail_obj, freelist);
		if (unlikely(!__update_cpu_freelist_fast(s, freelist, head, tid)))
			goto redo;
	} else {				/* PREEMPT_RT：走 local_lock */
		local_lock(&s->cpu_slab->lock);
		...
	}
	stat(s, FREE_FASTPATH);
```

> **实战含义**：`slab != c->slab` 这一个判断决定了你是 30 ns 还是 300 ns。
> **"CPU A 分配、CPU B 释放"** 会让释放**必然**走慢路径（要碰 `slab_lock` 与 node 链表），
> 这与 [12.4 页分配器 pcp](./section-12.4-获得页.md) 的跨 CPU 释放代价是**同一个病的两种表现**。

#### 6. 队列容量：v6.6 的实际数字

```c
/* mm/slub.c:4335 set_cpu_partial() */
	if (!kmem_cache_has_cpu_partial(s))   nr_objects = 0;
	else if (s->size >= PAGE_SIZE)        nr_objects = 6;
	else if (s->size >= 1024)             nr_objects = 24;
	else if (s->size >= 256)              nr_objects = 52;
	else                                  nr_objects = 120;
```

| 对象大小 | `cpu_partial`（按对象数） | 实际限制 |
|----------|--------------------------|---------|
| < 256 B | **120** | 转成 `cpu_partial_slabs = DIV_ROUND_UP(nr*2, oo_objects)`，即**按 slab 数**限流 |
| ≥ 256 B | **52** | 同上（源码注释：为了兼容保留了"对象数"语义，实际按页限流） |
| ≥ 1 KB | **24** | 同上 |
| ≥ PAGE_SIZE | **6** | 同上 |

`min_partial`（**per-node** 半满 slab 的保底数量，`mm/slub.c:4542`）：

```c
	s->min_partial = min_t(unsigned long, MAX_PARTIAL, ilog2(s->size) / 2);
	s->min_partial = max_t(unsigned long, MIN_PARTIAL, s->min_partial);
	/* MIN_PARTIAL = 5, MAX_PARTIAL = 10（CONFIG_SLUB_TINY 时为 0） */
```

> **两个数字说的是不同的事**：`cpu_partial` 控制 **per-CPU 缓存的上限**（防止无限膨胀），
> `min_partial` 控制 **per-node 半满链的保留下限**（空了也不立刻还 buddy，防抖动）。
> 二者都可经 sysfs 读写（见 §10）。

#### 7. ⚠️ 版本断崖：三种实现的兴亡（源码逐版本核对）

| 版本 | 事实 | 依据 |
|------|------|------|
| **v6.3** | 三种并存：`SLAB` / `SLUB` / `SLOB_DEPRECATED`+`SLOB` | `mm/Kconfig` v6.3 choice 内三个成员 |
| **v6.4** | **SLOB 被删除**（此前已先标 `SLOB_DEPRECATED`） | v6.5 Kconfig 已无 `config SLOB` |
| **v6.5** | **SLAB 被弃用**：新增 `config SLAB_DEPRECATED`（且 `depends on !PREEMPT_RT`），`config SLAB` 挂在它下面 | v6.5 Kconfig:237 |
| **v6.6** | SLUB 是 `choice` 的 **default**；SLAB 仍在但已弃用 | `mm/Kconfig:230-266` |
| **v6.8** | **SLAB 被彻底删除**：`config SLUB` 变成 `def_bool y`，不再有 choice | v6.8 Kconfig:245 |
| v6.4 起 | 新增 **`CONFIG_SLUB_TINY`** — 专给"原本用 SLOB 的最小系统"，牺牲可扩展性换内存 | `SLUB_TINY` 会关掉 per-CPU `cpu_slab`、cpu partial、KMALLOC_DMA/CGROUP 分档 |
| **v6.6 新增** | **`CONFIG_RANDOM_KMALLOC_CACHES`**（默认 `n`）：为普通 kmalloc **建 15 份副本 cache**，按**调用点地址 hash** 随机选一份 | v6.5 源码 0 处引用、v6.6 `mm/slab.h:349` 出现；`kmalloc_type()` 里 `hash_64(caller ^ random_kmalloc_seed, ilog2(15+1))` |

> **这本书（LKD）写作时是三种并存的年代**。今天的结论只剩一句：**SLUB 是唯一实现。**
> 书上"SLOB 适合嵌入式"的说法在 v6.6 上已**无从选择**——嵌入式极小系统的对应物是 `CONFIG_SLUB_TINY`。

**`RANDOM_KMALLOC_CACHES` 是什么思路**：同一大小的 `kmalloc-128` **复制 15 份**，
分配时按**调用点代码地址**（`caller`）hash 决定用哪份。这样攻击者想做
**堆喷（heap spray）** 时，无法保证自己可控的对象与目标漏洞对象落在同一条 cache 里——
**把 UAF/溢出利用的确定性打散**。代价：cache 数量 ×16，内存占用与 TLB/cold cache 上升。

#### 8. `kmalloc` 分档：不只有 2 的幂

```c
/* mm/slab_common.c:823 kmalloc_info[]（v6.6 逐字） */
const struct kmalloc_info_struct kmalloc_info[] __initconst = {
	INIT_KMALLOC_INFO(0, 0),
	INIT_KMALLOC_INFO(96, 96),		/* ← 非 2 的幂 */
	INIT_KMALLOC_INFO(192, 192),		/* ← 非 2 的幂 */
	INIT_KMALLOC_INFO(8, 8),
	INIT_KMALLOC_INFO(16, 16),
	... 32 / 64 / 128 / 256 / 512 / 1k / 2k / 4k / 8k ...
	INIT_KMALLOC_INFO(2097152, 2M)		/* 最后一项 */
};
```

| 请求大小 | 落到的 cache | 说明 |
|----------|-------------|------|
| 1~8 B | `kmalloc-8` | `KMALLOC_SHIFT_LOW = 3`（SLUB）→ `KMALLOC_MIN_SIZE = 8` |
| 65~96 B | **`kmalloc-96`** | 补档：72/80/88/96 都挤进 `-128` 太浪费 |
| 129~192 B | **`kmalloc-192`** | 同上，省下 64 B/对象 |
| 193~256 B | `kmalloc-256` | 之后回到 2 的幂 |
| ≤ 8192 B | `kmalloc-8k` | `KMALLOC_MAX_CACHE_SIZE = 1UL << (PAGE_SHIFT+1)` |
| **> 8192 B** | **不进 slab**，直接 `alloc_pages()` | 呼应 [12.4 Q5](./section-12.4-获得页.md) |

> 源码注释说得很清楚：`kmalloc_info[]` 存在的**主要目的**是让
> **`slub_debug=,kmalloc-xx`** 这样的**内核启动参数**能被解析——
> 所以表一直列到 `kmalloc-2M`（`kmalloc_index()` 支持到 2^21），
> 但 **16k 以上那些名字并不会真的建 cache**。

**cache 的四个"类型维度"**（`mm/slab.h:362`，`kmalloc_caches[NR_KMALLOC_TYPES][KMALLOC_SHIFT_HIGH + 1]`）：

| 类型 | 触发 gfp 标志 | sysfs 里的名字前缀 | 条件 |
|------|--------------|-------------------|------|
| `KMALLOC_NORMAL` | 无特殊标志（**最常见**） | `kmalloc-N` | 总是存在 |
| `KMALLOC_DMA` | `__GFP_DMA` | **`dma-kmalloc-N`** | `CONFIG_ZONE_DMA` |
| `KMALLOC_CGROUP` | `__GFP_ACCOUNT`（memcg 计费） | **`kmalloc-cg-N`** | `CONFIG_MEMCG_KMEM` |
| `KMALLOC_RECLAIM` | `__GFP_RECLAIMABLE` | **`kmalloc-rcl-N`** | 非 `SLUB_TINY` |

> 优先级写在 `kmalloc_type()` 里：**`__GFP_DMA` > `__GFP_RECLAIMABLE` > `__GFP_ACCOUNT`**。
> 且最前面有一句 `if (likely((flags & KMALLOC_NOT_NORMAL_BITS) == 0))` —— **最热的那条路只判一次位掩码**。

#### 9. `kfree()` 怎么知道这块内存属于哪个 cache？

`kmem_cache_free(cache, obj)` 需要显式传 cache，但 **`kfree(ptr)` 什么都不传**。
v6.6 的做法是**从页反查**：

```c
/* mm/slab_common.c:1056 */
void kfree(const void *object)
{
	struct folio *folio;
	struct slab *slab;
	struct kmem_cache *s;

	trace_kfree(_RET_IP_, object);
	if (unlikely(ZERO_OR_NULL_PTR(object)))
		return;

	folio = virt_to_folio(object);
	if (unlikely(!folio_test_slab(folio))) {	/* 不是 slab 页！ */
		free_large_kmalloc(folio, (void *)object);   /* 当初走的是 buddy */
		return;
	}
	slab = folio_slab(folio);
	s = slab->slab_cache;
	__kmem_cache_free(s, (void *)object, _RET_IP_);
}
```

```
kfree(ptr)
   │
   ├─ virt_to_folio(ptr) ─► folio_test_slab()?
   │        │
   │        ├─ 是 ─► folio_slab(folio)->slab_cache ─► __kmem_cache_free()   （≤8KB 路径）
   │        │
   │        └─ 否 ─► free_large_kmalloc()  → __free_pages()                （>8KB 路径）
```

> **这是个漂亮的对称设计**：分配时靠**大小**决定走 slab 还是 buddy，
> 释放时靠**页上的标志位**反推回去——**调用者完全不需要知道**。
> 代价是 `kfree()` 比 `kmem_cache_free()` 多一次 `virt_to_folio` + 标志位检查
> （约一次地址换算 + 一次位测试），所以对**超热路径的固定类型对象**，
> 用专用 cache + `kmem_cache_free()` 仍是有意义的微优化。

#### 10. 观测手段

| 接口 | 看什么 |
|------|--------|
| **`cat /proc/slabinfo`** | 每条 cache 的 `active_objs / num_objs / objsize / objperslab / pagesperslab`；`slabtop` 是它的实时版 |
| **`/sys/kernel/slab/<cache>/`** | 每个 cache 一个目录。v6.6 的属性（源码 `slab_attrs[]`）：`object_size` `objs_per_slab` `order` **`min_partial`** **`cpu_partial`** `partial` `cpu_slabs` `objects_partial` `align` `hwcache_align` `aliases` `ctor` `shrink`（可向 `shrink` 写 1 触发回收） |
| **`slub_debug=FZPU`** 启动参数 | F=一致性检查、Z=red zone、P=poison、U=记录分配/释放者（还有 T=trace、A=failslab、O=关掉"会抬高最小 order"的 cache、`-`=全关）。**只给指定 cache 开**：`slub_debug=FZ,dentry`；也可**按块**给不同 cache 不同选项：`slub_debug=Z,dentry;U,kmalloc-*`；还可以"全开但排除"：`slub_debug=FZ;-,zs_handle,zspage`（`kmalloc_info[]` 的名字就是为此存在）。详见 **v6.6 `Documentation/mm/slub.rst`**（⚠️ 已不在 `Documentation/vm/` 下） |
| **`CONFIG_SLUB_STATS=y`** | 打开 `struct kmem_cache_cpu.stat[]` 计数，暴露 **快/慢路径命中率**（见下表） |
| `trace_kmalloc` / `trace_kfree` | tracepoint，可挂 BPF 做**按调用点**的分配画像 |

`CONFIG_SLUB_STATS` 暴露的计数器（`enum stat_item`，`slub_def.h:13`）里最值得盯的四个：

| 计数器 | 含义 | 读法 |
|--------|------|------|
| `ALLOC_FASTPATH` | 从 `c->slab` 直接取到 | **这个数占比低 = 你的分配有问题** |
| `ALLOC_SLOWPATH` | 需要换 slab / 向 buddy 要页 | 上升 = 分配压力或 cache 太小 |
| `CMPXCHG_DOUBLE_FAIL` | 原子操作失败重来 | 上升 = **抢占/中断打断频繁**（RT 上恒为 0，因为根本不用快路径） |
| `CPU_PARTIAL_DRAIN` | per-CPU partial 被排空回 node | 频繁发生 = 跨 CPU 分配释放模式 |

#### 11. 安全硬化四件套（都在 kmalloc 的关键路径上）

| 机制 | 配置 | 干什么 |
|------|------|--------|
| **freelist 随机化** | `CONFIG_SLAB_FREELIST_RANDOM` | **打乱 slab 内对象的初始顺序**，让"连续分配的两个对象"不再相邻 |
| **freelist 指针混淆** | `CONFIG_SLAB_FREELIST_HARDENED` | 空闲对象的 next 指针与 `s->random ^ 对象地址` **异或后存储**，溢出覆盖它也无法可靠劫持 |
| **分配/释放即清零** | `CONFIG_INIT_ON_ALLOC_DEFAULT_ON` / `init_on_free=1` | `slab_want_init_on_alloc()` 在 `slab_post_alloc_hook` 里清零；**有成本**（大对象 memset） |
| **KFENCE** | `CONFIG_KFENCE=y` | 在 `slab_alloc_node()` 里**先**调 `kfence_alloc()`：以极低概率把请求**劫持**到带 guard page 的采样池，检测越界/UAF |

> 注意 `kmem_cache` 结构体里的 `unsigned long random;` 字段——
> 它就挂在 `CONFIG_SLAB_FREELIST_HARDENED` 下面，是上面第二个机制的密钥。

#### 12. ⚠️ PREEMPT_RT 上快路径是关掉的

```c
/* mm/slub.c:173 */
#ifndef CONFIG_PREEMPT_RT
#define USE_LOCKLESS_FAST_PATH()	(true)
#else
#define USE_LOCKLESS_FAST_PATH()	(false)		/* RT：一律走 local_lock 慢路径 */
#endif
```

原因写在文件头部注释里：RT 上 `local_lock` **既不关中断也不关抢占**，
快路径的 lockless 操作会与正在进行的慢路径操作**互相干扰**，
所以 RT 内核**放弃 lockless 快路径**，改为始终持 `local_lock`
（但**仍在用 freelist**，不是退回到加锁遍历）。

> **对 RT 用户的直接结论**：`CONFIG_PREEMPT_RT` 上 slab 分配的**确定性更好**（无重试循环），
> 但**单次开销更高**（要拿锁）。做实时/低抖动评估时，这是必须纳入模型的差异。

#### 主要 API

| API | 作用 |
|-----|------|
| **`kmem_cache_create(name, size, align, flags, ctor)`** | 建 **专用 cache**（v6.6 签名实证：`(const char *, unsigned int, unsigned int, slab_flags_t, void (*)(void *))`） |
| **`kmem_cache_create_usercopy(...)`** | 额外声明**可拷贝给用户态的区间** `useroffset/usersize`，其余区域不允许 `copy_to_user` |
| **`kmem_cache_destroy(cache)`** | 销毁（须 **无 live 对象**） |
| **`kmem_cache_alloc(cache, gfp)`** | 取对象 |
| **`kmem_cache_free(cache, obj)`** | 归还（比 `kfree` 少一次 folio 反查） |
| **`kmem_cache_shrink(cache)`** | 把空 slab 还给 buddy（`min_partial` 以下的会被**保留**） |
| **通用 Slab** | **`kmalloc`** 内部选 **合适 size 的 general cache** |

常用 `slab_flags_t`（`include/linux/slab.h` 实证）：

| 标志 | 效果 |
|------|------|
| `SLAB_HWCACHE_ALIGN` | 按缓存行对齐对象（**防 false sharing**，代价是内部碎片） |
| `SLAB_POISON` | 调试：对象用毒值填充，捕获"未初始化就使用" |
| `SLAB_RED_ZONE` | 调试：对象前后插入 red zone，捕获越界写 |
| `SLAB_CACHE_DMA` / `SLAB_CACHE_DMA32` | 从 DMA 区取页 |
| `SLAB_TYPESAFE_BY_RCU` | **RCU 安全复用**：延迟归还 **slab 页**（注意：**不是**延迟对象复用！） |
| `SLAB_RECLAIM_ACCOUNT`（= `SLAB_TEMPORARY`） | 标记为可回收（如 `dentry`/`inode`），走 `kmalloc-rcl-*` |
| `SLAB_NO_MERGE` | 禁止与其他 cache 合并（调试时保留独立可见性） |
| `SLAB_SKIP_KFENCE` | 不参与 KFENCE 采样（sysfs 可运行时开关） |

> **`SLAB_TYPESAFE_BY_RCU` 是最大的误解高发区**（源码警告原文）：
> 它 **只延迟"slab 页"的释放**，**不延迟对象复用**。
> `kmem_cache_free()` 之后，**那个地址随时可能被别的对象占用**。
> 它保证的是"你手里的指针所在的页不会被立刻还给 buddy 导致访问时缺页"，
> 而不是"对象内容还在"。RCU 读者必须**先拿引用再上锁**（参见 `include/linux/slab.h:48-96` 的长注释）。

#### Cache 合并（为什么 `slabinfo` 里名字对不上号）

`CONFIG_SLAB_MERGE_DEFAULT` **默认 = y**：`kmem_cache_create()` 时若发现
已有 cache 的 **size / align / flags** 足够接近，就**直接复用**那条已有 cache，
只在 sysfs 里留下一个 **alias**。所以：

- `cat /proc/slabinfo` 里看到的条目数 **远少于** 内核里 `kmem_cache_create` 的调用数；
- 想让调试对象**独立可见**（不被合并掉别人也看不出是谁），建 cache 时加 **`SLAB_NO_MERGE`**；
- sysfs 的 **`aliases`** 属性能看到"这条 cache 被谁共用了"。

**HFT：** 用户态 **typed object pool**（订单对象、事件 struct）= **`kmem_cache_*` 用户版**。内核 **网络栈 `sk_buff`** 等有 **专用 cache** — **NAPI poll** 路径 **复用 skb** 而非每次 `alloc_pages`。实盘：**池化 + 复用** 减 **allocator 锁竞争** 与 **TLB 抖动**。

> **落到 HFT/低延迟上的四条硬结论**：
> ① **同一个 CPU 上"刚 free 就 alloc"是最快的**（命中 `c->slab` 的 freelist 头，热在 L1）——
>   所以**每 CPU 一份对象池 + 单线程绑核**的收益，与 SLUB 的设计完全同构；
> ② **跨 CPU 释放会打掉快路径**（`slab != c->slab` → `__slab_free`），
>   这与 12.4 页分配器的 pcp 跨 CPU 释放是**同一类病**：分配/释放必须**同核**；
> ③ **热路径零分配**：SLUB 快路径再快也要一次 `cmpxchg16b`，而且**可能被抢占打断后重来**，
>   `ALLOC_SLOWPATH` 一旦出现就可能是**微秒级**（`new_slab` → `alloc_pages`）。
>   实盘驱动：probe 阶段把 ring 建好，数据路径只读写；
> ④ **`cpu_partial` 是尾延迟的隐形来源**：per-CPU partial 超限时**批量排空回 node**（要持 `n->list_lock`），
>   表现为偶发尖刺。用 `CONFIG_SLUB_STATS` 盯 `CPU_PARTIAL_DRAIN` 能验证这一点。

→ [06 Gorman Ch8 Slab](../../../06-linux-mm/chapter-08-slab-allocator/) · [Ch 3 task_struct Slab](../../chapter-03-process-management/) · [Ch 12.10 per-CPU](./section-12.10-每个-CPU-的分配.md)


> ↔ [ULK Ch8 §3 Slab分配器](../../../16-linux-kernel-deep/chapter-08-memory-management/notes/section-3-Slab分配器.md)


<details>
<summary>自测题（点击展开）</summary>

**Q1.** Slab 分配器的三级缓存是什么？为什么能加速对象分配？

<details><summary>答案</summary>

Slab > Slub（现代默认）> Slob（嵌入式）。以 Slub 为例：每个 CPU 有 per-CPU partial 页，分配时从当前 CPU 的 partial 页上取空闲对象，无需锁、无需 buddy 调用。释放时放回 per-CPU partial。只有 partial 耗尽才向 buddy 申请新页。这就是为什么内核频繁分配/释放 task_struct 不会变慢。

> **按 v6.6 修订（重要）**："Slab > Slub > Slob"**不是层级关系**，而是**三种可替换的实现**
> （同一组 API `kmem_cache_*` 的不同后端）。现状是：
> **SLOB 在 v6.4 被删除、SLAB 在 v6.5 被弃用并在 v6.8 被删除**，
> 所以 v6.6 的默认（也是 v6.8+ 的唯一）实现是 **SLUB**。
> 上面答案里"每个 CPU 有 per-CPU partial 页"的说法也需要细化：
> 真正承担分配的不是 partial 链，而是 **`c->slab`（当前 frozen slab）+ `c->freelist`**；
> per-CPU partial 只是"备胎池"。另外"无需锁"的准确说法是
> **一次 `this_cpu_cmpxchg_double()`（freelist + tid 打包）**，
> 失败则 `goto redo` 重试——**不是加锁，但也不是一次必成**。

</details>

**Q2.** kmalloc-128 和 kmalloc-256 是什么？为什么有这么多 slab cache？

<details><summary>答案</summary>

内核为每个 2 的幂大小（8/16/32/64/128/256/512/1024/2048/4096/8192）预创建专用 slab cache。kmalloc(100) 会在 kmalloc-128 中分配（向上取整到 128）。这样不同大小的对象不会互相碎片化，且每个 cache 的对象大小一致、对齐一致，cache 友好。

> **按 v6.6 修订**：分档**不全是 2 的幂**——还有 **`kmalloc-96`** 和 **`kmalloc-192`**
> （`mm/slab_common.c:823` 逐字）。所以 **`kmalloc(100)` 落在 `kmalloc-128`** 是对的，
> 但 `kmalloc(80)` 落在 **`kmalloc-96`** 而不是 `-128`，`kmalloc(150)` 落在 **`kmalloc-192`**。
> 另外 cache 的数量还要再乘一个维度：v6.6 有
> **`kmalloc-N` / `dma-kmalloc-N` / `kmalloc-cg-N` / `kmalloc-rcl-N`** 四类（按 gfp 标志选），
> 若开 `CONFIG_RANDOM_KMALLOC_CACHES` 还会再复制 **15 份**（`kmalloc-rnd-01-N` … `kmalloc-rnd-15-N`）。
> 还有一个上限：**> 8 KB 根本不进 slab**，直接走 `alloc_pages()`。

</details>

**Q3.** SLUB 的快路径为什么不直接 CAS 替换 `freelist` 指针，非要带上一个 `tid`？

<details><summary>答案</summary>

因为 **ABA 问题**。`struct kmem_cache_cpu` 把 `freelist` 和 `tid` 放在同一个 union 里
（`freelist_aba_t freelist_tid`），满足 `this_cpu_cmpxchg_double()` 的 16 字节对齐要求，
一次原子指令**同时**比较并替换"指针 + 版本号"。

场景：本 CPU 读到 `freelist=A, tid=7` → 被中断，中断里 free 一个对象（freelist=B, tid=8）
再 alloc 一次（freelist 又变回 **A**, tid=**9**）→ 回到原路径。
**只比指针的话 A==A 会误判"期间无人动过"**，但事实上 B 已被取走又归还，
本 CPU 手里的 `next_object` 等派生数据已经失效。`tid` 每次操作 `next_tid()`，
把"发生过几次操作"编码进去，CAS 因此失败 → `goto redo` 重读重试。
这就是为什么源码里 `barrier()` 前的注释强调 **tid 必须在 freelist/slab 之前读**。

</details>

**Q4.** 为什么 `kfree(ptr)` 不需要传 cache 指针？它怎么分辨"这块是 slab 对象还是大块 buddy 分配"？

<details><summary>答案</summary>

靠**从 folio 反查**（`mm/slab_common.c:1056`）：

```c
folio = virt_to_folio(object);
if (unlikely(!folio_test_slab(folio))) {
        free_large_kmalloc(folio, (void *)object);   /* 当初 >8KB，走的是 buddy */
        return;
}
slab = folio_slab(folio);
s = slab->slab_cache;
__kmem_cache_free(s, (void *)object, _RET_IP_);
```

即：**分配时按大小选路（≤8KB 进 slab，>8KB 走 `alloc_pages`），释放时靠页上的 slab 标志位反推**，
调用者完全无感。推论：
① `kfree()` 比 `kmem_cache_free()` 多一次 `virt_to_folio` + 位测试，超热路径上用专用 cache 仍值得；
② 反过来说明 **`kfree()` 不能用于非 kmalloc/kmem_cache_alloc 得到的指针**（比如 `vmalloc` 的返回值）——
它的 folio 没有 slab 标志，会被当成"大块 kmalloc"错误处理。

</details>

**Q5.** `SLAB_TYPESAFE_BY_RCU` 能保证"free 之后对象内容还在"吗？

<details><summary>答案</summary>

**不能。这是最常见的误解。** 源码注释（`include/linux/slab.h:48-96`）明确警告：
它**只延迟 slab 页的释放**（delay freeing the SLAB page by a grace period），**不延迟对象复用**
（does NOT delay object freeing）。`kmem_cache_free()` 之后，
那块地址**随时可能被另一个对象占用**——即使在同一个 RCU grace period 内。

它真正保证的是：**指针所在的页不会立刻被还给 buddy**，所以 RCU 读者
**解引用这个地址不会踩到已解除映射的内存**（不会缺页/不会读到被复用给别处的页），
但**读到的内容可能是新对象**。因此正确用法是：
`rcu_read_lock()` → 读指针 → **先拿对象引用** → 拿到后校验对象身份 → 才上对象内部的锁。
注释里点名了三个真实用户：`__i915_request_ctor()`、`sighand_ctor()`、`anon_vma_ctor()`。

</details>

**Q6.** 在 `CONFIG_PREEMPT_RT` 内核上，SLUB 的分配路径有什么不同？为什么？

<details><summary>答案</summary>

`USE_LOCKLESS_FAST_PATH()` 在 RT 上被定义为 **`false`**（`mm/slub.c:173`），
于是所有分配/释放都走 `local_lock(&s->cpu_slab->lock)` 保护的分支，**不再用 cmpxchg_double 快路径**。

原因：RT 上 `local_lock` **既不禁抢占也不禁中断**，
lockless 快路径的读写会与**正在进行的慢路径操作**互相干扰（数据竞争），
所以干脆放弃快路径、始终持锁。注意两点：

1. 它**仍然在使用 freelist**（`c->freelist` / `c->slab` 照旧），
   不是退化成"每次都去 node 链表找"，所以**不是数量级的退化**；
2. RT 上 `CMPXCHG_DOUBLE_FAIL` 计数**恒为 0**（根本没有重试循环），
   代价是**单次开销更高、但更可预测**。做实时抖动预算时必须把这个差异建模进去。

</details>

**Q7.** 想给某个特定 cache（比如 `dentry`）单独开 slub_debug，而不影响其他 cache 的性能，怎么做？为什么能这样做？

<details><summary>答案</summary>

启动参数：**`slub_debug=FZ,dentry`**（`F`=sanity checks、`Z`=red zoning）。
v6.6 `Documentation/mm/slub.rst` 给了完整语法：

```
slub_debug=<Options>                        # 全部 cache（不写选项 = 全功能调试）
slub_debug=<Options>,<slab1>,<slab2>,...    # 只给指定 cache（逗号后不加空格）
slub_debug=<Options>,<name>;<Options>,<name>   # 多个块，用 ; 分隔
slub_debug=FZ;-,zs_handle,zspage            # 全部开，但排除这两个（性能关键）
slub_debug=P,kmalloc-*,dentry               # 名字支持 * 前缀通配
```

选项表：`F`=sanity checks、`Z`=red zoning、`P`=poison、`U`=user tracking（记录 alloc/free）、
`T`=trace（**只应在单条 cache 上用**）、`A`=failslab 标记、`O`=给"会因调试抬高最小 order"的 cache 关掉调试、
`-`=全关（配合 `CONFIG_SLUB_DEBUG_ON` 使用）。
多个块时：**最后一个"全局块"作用于除匹配块以外的所有 cache；cache 只会命中第一个匹配的块。**

**为什么能用名字定位**：内核在 `__init` 阶段建了一张
**`kmalloc_info[]` / cache name 对照表**（`mm/slab_common.c:823`），
源码注释直说了这张表的存在意义就是
"**to make `slub_debug=,kmalloc-xx` option work at boot time**"。
所以表里才会一直列到 `kmalloc-2M`——**用于启动参数解析**，而不是真的建了这么多 cache。

两个实践提醒：
① 开了 debug 的 cache，源码注释明说 **"For debug caches, all allocations are forced to go through a list_lock protected region"**——
即**强制走慢路径并加锁**，性能会明显下降，别在生产上给热 cache 开；
② 若目标 cache 被**合并**（`CONFIG_SLAB_MERGE_DEFAULT=y`），名字可能对不上——
先到 `/sys/kernel/slab/<name>/aliases` 确认，或建 cache 时加 `SLAB_NO_MERGE`。

</details>

</details>
---
