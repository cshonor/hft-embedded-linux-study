# Ch 8 §2 核心数据结构：Cache 与 Slab（SLUB 版）

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**
> 源码核验：Linux **v6.6**（`mm/slub.c` 头注释 :4-:110、`include/linux/slub_def.h`）

---

## 本节讲什么

原书的 `kmem_cache_t` + `slab_t` + `kmem_bufctl_t` 三件套在 SLUB 里被重构成 **三个 struct**：`kmem_cache`（类型）、`kmem_cache_cpu`（每 CPU 热层）、`kmem_cache_node`（每 NUMA 节点冷层）。数据结构的形状就是算法的形状——本节把这个形状讲透。

---

## 1. 原书结构 → SLUB 结构对照

| 原书（SLAB） | SLUB v6.6 | 变化本质 |
|--------------|-----------|----------|
| `kmem_cache_t` 三条链（full/partial/free） | `kmem_cache` + 每 node **只保留 partial 链** | full/free 是冗余状态——full slab 无事可做，free 归还 buddy 即可 |
| `slab_t`（on/off-slab 描述符） | **`struct slab`**（复用 `struct page`，v5.9 起独立类型名） | 描述符寄生在页元数据里，零额外内存 |
| `kmem_bufctl_t[]` 空闲索引数组 | **freelist 嵌在对象体内**（首 word 当 next） | 元数据随对象走，cache 密度最大化 |
| per-CPU `array_cache` | `struct kmem_cache_cpu`（active slab + 本地 partial） | 同思想，更精的结构 |

## 2. SLUB 三件套（v6.6）

```c
struct kmem_cache {              /* 一种对象类型一份 */
    const char        *name;
    unsigned long      size;      /* 对象实际大小（含元数据）*/
    unsigned int       object_size, offset;  /* offset: freelist 指针在对象内的偏移 */
    slab_flags_t       flags;
    int                cpu_partial;  /* 每 CPU partial 链的对象数上限 */
    ...
    struct kmem_cache_cpu __percpu *cpu_slab;   /* 热层 */
    struct kmem_cache_node *node[MAX_NUMNODES]; /* 冷层 */
};

struct kmem_cache_cpu {          /* 每 CPU 一份（热路径常驻） */
    void  **freelist;            /* 本 CPU active slab 的空闲链头 */
    struct slab *slab;           /* 当前 active slab */
    unsigned long tid;           /* 事务号：cmpxchg 双字段校验 */
    struct slab *partial;        /* 本 CPU partial 链（无锁挂载） */
};

struct kmem_cache_node {         /* 每 NUMA 节点一份（冷层） */
    spinlock_t  list_lock;
    unsigned long nr_partial;
    struct list_head partial;     /* 节点级 partial slab 链 */
};
```

## 3. 关键设计：frozen slab（v6.6 slub.c 头注释 :80-:89 原文）

> "If a slab is frozen then it is exempt from list management. It is not on any list except per cpu partial list. The processor that froze the slab is the one who can perform list operations on the slab. Other processors may put objects onto the freelist but the processor that froze the slab is the only one who can retrieve the objects from the slab's freelist."

翻译成机制语言：

```
active slab（被 CPU0 冻结）：
  CPU0：分配走 slab->freelist（取）
  CPU1：free 到该 slab？→ CAS 挂回 slab->freelist（只放不取）
```

| frozen 带来的性质 | 意义 |
|-------------------|------|
| 分配 = 单边操作（只有 owner 取） | 快路径只需本地 cmpxchg，无需与 free 方协商 |
| free 方与 alloc 方通过原子链表会合 | 生产者-消费者天然解耦 |
| slab 不在全局链上 | node->list_lock 完全不进分配快路径 |

## 4. 锁层级（slub.c :51-:56 原文，v6.6 实锚）

```
1. slab_mutex        （全局，create/destroy cache 时才拿）
2. node->list_lock   （per-node，慢路径：partial 链增删）
3. cpu_slab->lock    （local lock，慢路径改 kmem_cache_cpu 字段）
4. slab_lock(slab)   （仅无 cmpxchg_double 的 arch）
5. object_map_lock   （仅 debug）
```

**快路径（§3 详述）一把锁都不拿**——直接 `this_cpu_cmpxchg_double(freelist, tid)`。这就是头注释说的 "operations can continue without any centralized lock"。

## 5. slab 的页阶（order）与对象数

```c
/* calculate_order() 决定每 slab 几页 */
slab_order = min(能放下 min_objects 个对象的 order, max_order);
```

| 参数 | 默认 | 效果 |
|------|------|------|
| `slub_max_order` | 3（8KiB..32KiB slab） | 超过则降对象数 |
| `slub_min_objects` | 0（按 CPU 数推） | 每页对象太少 → 提阶 |
| `kmem_cache` sysfs | `/sys/kernel/slab/<name>/order` | 观测实际 order |

**HFT 视角：** order 越大，一次 grow 的 buddy 压力越大（连续页难找），但 partial 链换 slab 频率越低。默认平衡即可；调专用 cache 时 `SLAB_HUGE_HP` 不存在——内核 slab **不用** hugetlbfs。

## 6. 观测

```bash
cat /proc/slabinfo | head -3        # name active_objs num_objs objsize objperslab pagesperslab
cat /sys/kernel/slab/<name>/object_size /sys/kernel/slab/<name>/slab_size
slabtop -o -s c | head              # 按 cache 占用排序（依赖 slabinfo）
grep -w 'slab_objects\|slabs' /proc/vmstat
```

`smaps` 里内核 slab 不计入进程 RSS——**容器/memcg 场景要单独盯 `/sys/fs/cgroup/.../memory.stat` 的 slab 行**。

## 7. HFT / 嵌入式关联

| 结构性质 | 用户态镜像 |
|----------|-----------|
| frozen 单边取 | 每核 ring：单消费者 CAS 取，多生产者 CAS 放（MPSC） |
| freelist 嵌对象 | pool 槽位首 8B 作 next——零元数据池 |
| per-node 冷层 | 按.NUMA 节点分 arena（`numa_alloc_onnode`） |
| tid 事务号 | ABA 问题防护：CAS 双字段（指针+计数器），与 lock-free 队列的 tagged pointer 同构 |

## 8. 衔接

- [§3 对象分配与释放](./section-3-对象分配与释放.md)：在这套结构上跑的快慢路径
- [§5 每 CPU 对象缓存](./section-5-每-CPU-对象缓存.md)：`kmem_cache_cpu` 的完整行为
- [Ch 6 物理页分配](../../chapter-06-physical-page-allocation/)：slab 页的来源
- [06.5/ch02](../../../06.5-modern-mm/chapter-02-slab-slub-allocator/)

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：SLUB 为什么删掉 full 和 free 两条链？**
A：full slab 永远不会出现在任何查找路径里（它没有空闲对象，partial 查找跳过它），维护它进/出链表纯属白付锁开销；全空 slab 直接还 buddy（或留 per-cpu partial 少量备胎）。**状态机越少，不变量越好守**——删掉的不是链，是两种状态转换。

**Q2：`struct slab` 和 `struct page` 什么关系？**
A：同一块内存的两种"视图"（union 意义上）：页被 slab 化后，`page` 的部分字段位置由 slab 复用（freelist/counters）。v5.9 起引入 `struct slab` 类型别名并清理 accessor（`slab_address()` 等），让"这页是 slab"的语义显式化。类似 struct folio 的路数（06.5/ch06）。

**Q3：freelist 指针为什么可以放对象体内而不会污染业务数据？**
A：`kmem_cache->offset` 记录指针位置：默认 = 对象首 word（对象空闲时首 word 无意义）。开启 `CONFIG_SLAB_FREELIST_HARDENED` 时还 XOR 一次指针值。分配时内核把 next 读走后才把对象交出去——**借了"空闲对象的身体"当链表节点**。

**Q4：为什么 free 到别的 CPU 的 active slab 是安全的？**
A：frozen 语义：free 方做的是"原子地把对象 CAS 进 slab->freelist"（slab_free_freelist_hook 链），只放不取不与 owner 冲突；owner 的分配也是 CAS。两个 CAS 竞争同一 freelist 头时一方重试即可——无锁会合。

**Q5：`/proc/slabinfo` 里 objsize=64 objperslab=64 pagesperslab=1 是什么意思？**
A：kmalloc-64 档：每 slab 一页放 64 个 64B 对象，零头 0B。这就是 §4 要讲的"档位"设计的直接后果——**64 的整数倍请求零内部碎片**；65B 的请求进 96 档浪费 31B（32%）。热路径 struct 尺寸应向 2 幂对齐的根据在此。

</details>

---
