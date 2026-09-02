## ⑤ 选择合适的数据结构 · Choosing the Right Structure

> 承接 [6.5 红黑树](./section-6.5-二叉树.md)（如存在）与本章前文。
> 本节回答：**内核提供了哪些结构、各自的实现代价是什么、以及"选型"真正的四个维度。**
>
> 🔗 **本篇只讲「选型维度」**：XArray / maple tree 的完整机制（B-Tree 节点布局、
> RCU 安全读、per-VMA lock 为什么必须依赖它）见
> [06.5 现代 MM · Maple Tree](../../../06.5-modern-mm/chapter-05-vm-address-space-maple-tree/notes/02-maple-tree.md)。

| 主要需求 | 选用 |
|----------|------|
| **遍历所有元素** | **链表** `list_head` |
| **生产者 / 消费者、FIFO** | **队列** `kfifo` |
| **UID → 对象指针** | **映射** `idr`（底层已是 XArray） |
| **大量数据 + 高效随机检索** | **红黑树** `rbtree` |

#### 决策简图

```
要存内核对象？
    │
    ├─ 主要靠「扫一遍」─────────► list_head
    ├─ 一边产一边消、FIFO ──────► kfifo（SPSC 无锁）
    ├─ 要小整数 handle ─────────► idr → 现代 xarray
    ├─ 按键排序/最值/查找 ──────► rbtree
    ├─ 按键范围查询（区间）─────► maple tree（v6.1+）/ interval tree
    └─ 均匀键、O(1) 点查 ───────► hlist 哈希表
```

---

### 一、⭐ 先理解内核容器的第一原则：**侵入式（intrusive）**

用户态容器（`std::list<T>`、`Vec<T>`）是**容器拥有元素**；内核容器是**元素拥有容器节点**。

```c
struct my_object {
	int			value;
	struct list_head	list;      /* ⭐ 节点嵌在对象里 */
	struct rb_node		rb;        /* ⭐ 同一个对象可挂多条结构 */
};
```

| | 用户态容器 | 内核容器 |
|---|----------|---------|
| 谁分配节点 | 容器分配（每次 insert 一次 `malloc`） | ⭐ **零额外分配** —— 节点就在对象里 |
| 元素能否同时在多个容器 | 不能（除非存指针拷贝） | ⭐ **能**（多个 `list_head` 成员） |
| 从节点找回对象 | 不需要 | ⭐ `container_of()` / `list_entry()` |
| 删除的复杂度 | O(1)（容器知道节点） | O(1)（只需 `head` + 节点本身） |
| 内存开销 | 每元素一次分配（含 malloc 头） | ⭐ 每节点 16 字节，无分配器开销 |

> ⭐ **对 HFT 的直接意义**：用户态热路径上，`std::map<K,V>` 每次插入都 `malloc` 一个节点——**这点常被忽略，但它意味着不可预测的延迟尖峰和 cache 碎片**。内核的侵入式思路平移过来就是"预分配对象池 + 侵入式链表"，这正是很多低延迟订单簿的做法。

---

### 二、`list_head` vs `hlist`：头的 8 字节之差

v6.6 `include/linux/types.h`：

```c
struct list_head {
	struct list_head *next, *prev;
};

struct hlist_head {
	struct hlist_node *first;
};

struct hlist_node {
	struct hlist_node *next, **pprev;
};
```

| | `list_head`（双向循环） | `hlist_head`（单指针头 + 双向节点） |
|---|------------------------|-----------------------------------|
| **头节点大小** | 16 字节（`next`+`prev`） | ⭐ **8 字节**（只有 `first`） |
| 节点大小 | 16 字节 | 16 字节（`next`+`pprev`） |
| 是否循环 | 是（空表时头指向自己） | 否（空表 `first == NULL`） |
| 典型用途 | 少量链表（进程链表、设备链表） | ⭐ **大量桶的哈希表**（PID 哈希、dentry 哈希） |

> ⭐ 省 8 字节在小规模时无所谓，但一个 **262144 桶**的哈希表就是 **2 MB** 的差别。这就是 `hlist` 存在的理由。

#### ⭐ `pprev` 为什么是**二级指针**

```c
struct hlist_node {
	struct hlist_node *next, **pprev;   /* ← pprev 是 struct hlist_node ** */
};
```

| 节点位置 | `pprev` 指向 |
|---------|-------------|
| 桶内**第一个**节点 | `&head->first` |
| 其他节点 | `&前一个节点->next` |

于是删除操作可以写成**统一的**两行，**不需要判断"是不是第一个"**：

```c
static inline void __hlist_del(struct hlist_node *n)
{
	struct hlist_node *next = n->next;
	struct hlist_node **pprev = n->pprev;

	WRITE_ONCE(*pprev, next);              /* 前驱的"指向我的字段"改指 next */
	if (next)
		WRITE_ONCE(next->pprev, pprev);   /* next 的 pprev 接管 */
}
```

对比普通双向链表必须写：
```c
if (node->prev == head) head->first = node->next;   /* 边界判断 */
else node->prev->next = node->next;
```

> ⭐ **"用二级指针消除边界判断"** 是内核里反复出现的技巧（同样手法见 `plist`、`rb_node` 的 parent 编码、radix tree 的 slot 操作）。少一个分支 = 少一次分支预测失败。

---

### 三、`llist`：无锁单链表的**并发操作矩阵**

v6.6 `include/linux/llist.h` 的头部注释直接给出了一张并发兼容表：

```c
/*
 * Lock-less NULL terminated single linked list
 *
 * Cases where locking is not needed:
 * If there are multiple producers and multiple consumers, llist_add can be
 * used in producers and llist_del_all can be used in consumers simultaneously
 * without locking. ...
 *
 * This can be summarized as follows:
 *
 *           |   add    | del_first |  del_all
 * add       |    -     |     -     |     -
 * del_first |          |     L     |     L
 * del_all   |          |           |     -
 *
 * Where, a particular row's operation can happen concurrently with a column's
 * operation, with "-" being no lock needed, while "L" being lock is needed.
 */
```

| 操作组合 | 需要锁？ |
|---------|---------|
| `llist_add` × `llist_add` | ⭐ 否 |
| `llist_add` × `llist_del_first` | ⭐ 否 |
| ⭐ **`llist_add` × `llist_del_all`** | ⭐ **否 → MPMC 无锁** |
| `llist_del_first` × `llist_del_first` | **是** |
| `llist_del_first` × `llist_del_all` | **是** |
| `llist_del_all` × `llist_del_all` | ⭐ 否（各自 `xchg` 拿走整条链） |

> ⭐⭐ **核心结论**：**`llist_add` + `llist_del_all` 的组合是多生产者多消费者（MPMC）无锁的**。
> 这正是内核里 `llist` 的典型用法——**多个 CPU 往里塞（add），一个线程一次性收割（del_all）**，例如 `irq_work`、`vm_area` 的惰性释放、`task_struct` 的延迟 RCU 释放队列。

**为什么 `del_first` 需要锁**（注释原文）：

> "llist_del_first depends on list->first->next not changing, but without lock protection, there's no way to be sure about that if a preemption happens in the middle of the delete operation..."

即：`del_first` 的 `cmpxchg` 依赖"第一个节点的 `next` 在我操作期间不变"。如果被抢占，另一个消费者可能已经改过链表，导致 **ABA 问题**——`cmpxchg` 成功但语义错误。

---

### 四、`kfifo`：SPSC 无锁环

| 特性 | 说明 |
|------|------|
| 结构 | 固定大小环形缓冲，`in` / `out` 两个单调递增值 |
| 无锁前提 | ⭐ **严格 SPSC**（单生产者 + 单消费者） |
| 为何无锁 | 生产者只写 `in`，消费者只写 `out`；各自读对方的值作边界判断 |
| 内存屏障 | 需要 `smp_store_release` / `smp_load_acquire` 配对（见 [Ch 10 排序和屏障](../../chapter-10-sync-methods/notes/section-10.10-排序和屏障.md)） |
| 大小要求 | 容量为 **2 的幂**（用 `& (size-1)` 代替 `%` 取模） |

```
        out                    in
         │                      │
    ┌────┴──────────────────────┴─────┐
    │    │ 已消费 │   待消费数据  │ 空 │
    └─────────────────────────────────┘
    0                                size-1

  生产者：写 buffer[in & mask]，smp_store_release(&in, in+1)
  消费者：smp_load_acquire(&out) 与 in 比较，读 buffer[out & mask]，
          再 smp_store_release(&out, out+1)
```

> ⭐ **HFT 直接对应**：行情网关收包 → 策略线程，**就是标准 SPSC**。用户态实现（如 Disruptor / boost::lockfree::spsc_queue）与 `kfifo` 是同一个模型。注意：**容量必须是 2 的幂**，否则取模开销会吃掉收益。

---

### 五、⭐ XArray：用「指针低位打标」把值直接塞进指针

XArray 是 v6.6 的**默认稀疏数组/映射**选择，page cache、IDR、很多子系统都用它。

v6.6 `include/linux/xarray.h:23-42`：

```c
/*
 * The bottom two bits of the entry determine how the XArray interprets
 * the contents:
 *
 * 00: Pointer entry
 * 10: Internal entry
 * x1: Value entry or tagged pointer
 *
 * Attempting to store internal entries in the XArray is a bug.
 *
 * Most internal entries are pointers to the next node in the tree.
 * The following internal entries have a special meaning:
 *
 * 0-62: Sibling entries
 * 256: Retry entry
 * 257: Zero entry
 *
 * Errors are also represented as internal entries, but use the negative
 * space (-4094 to -2).  They're never stored in the slots array; only
 * returned by the normal API.
 */
```

#### ⭐⭐ 低位打标：一个指针位图当三种东西用

| 低 2 位 | 含义 | 说明 |
|--------|------|------|
| `00` | **Pointer entry** | 普通指针（对齐保证最低 2 位为 0） |
| `10` | **Internal entry** | 树节点指针 / 特殊标记（0-62 兄弟、256 retry、257 zero） |
| `x1` | ⭐ **Value entry** | **直接把整数存在"指针"里，零内存分配** |

```c
#define BITS_PER_XA_VALUE	(BITS_PER_LONG - 1)     /* 64 位机上 = 63 位 */
```

> ⭐⭐ **Value entry 的价值**：存一个小整数（如引用计数、小 ID）时**完全不需要分配节点**。
> `xa_mk_value(v)` 把 `v` 左移 1 位并置低位 1；`xa_to_value(e)` 反向。
> 对于"稀疏 ID → 小整数"的场景（如 epoll 的 fd 位图、很多计数器），这省掉了**每一次**的内存分配。

#### 错误码也走内部条目

> "Errors are also represented as internal entries, but use the negative space (-4094 to -2). They're never stored in the slots array; only returned by the normal API."

⭐ 这与 [5.2 §5 讲过的 `MAX_ERRNO = 4095`](../../chapter-05-system-calls/notes/section-5.2-系统调用基础.md) 是同一套编码体系：错误码用 `-4094 ~ -2` 的负空间，**永不存入树**，只在 API 返回值里出现。

#### 版本断崖

| 版本 | `include/linux/xarray.h` | 状态 |
|------|------------------------|------|
| v4.17 及更早 | 无 | — |
| **v4.18 / v4.19** | **818 字节** | ⭐ **占位骨架**：只有 `xa_trylock` 等几个宏，**没有 `struct xarray`** |
| **v4.20** | **45558 字节**（50 处 `struct xarray`） | ⭐ **完整实现落地** |

准确说法：**XArray 骨架于 v4.18 引入，完整实现在 v4.20 落地**。

---

### 六、⭐ 一个反直觉的事实：v6.6 的 `idr` 底层就是 XArray

教材常说"IDR 被 XArray 取代了"。**v6.6 的实际情况更有意思**——IDR 没有消失，它变成了 XArray 的薄封装。

v6.6 `include/linux/idr.h:14-24`：

```c
#include <linux/radix-tree.h>
#include <linux/gfp.h>
#include <linux/percpu.h>

struct idr {
	struct radix_tree_root	idr_rt;
	unsigned int		idr_base;
	unsigned int		idr_next;
};
```

看起来还在用 `radix_tree`？再看 v6.6 `include/linux/radix-tree.h:10-28`：

```c
#include <linux/xarray.h>
#include <linux/local_lock.h>

/* Keep unconverted code working */
#define radix_tree_root		xarray
#define radix_tree_node		xa_node
```

⭐⭐ **`radix_tree_root` 已经被 `#define` 成 `xarray` 了！** 注释原文：

> `/* Keep unconverted code working */`

所以真实的继承链是：

```
struct idr
   └─ struct radix_tree_root idr_rt
        └─ = struct xarray          （radix-tree.h 的 #define 兼容层）
```

| 说法 | 是否正确 | 实际 |
|------|---------|------|
| "IDR 被 XArray 取代，IDR 没了" | ❌ | IDR 的 API 仍在（`idr_alloc`、`idr_find`…） |
| "IDR 和 XArray 是两套独立实现" | ❌ | ⭐ IDR 底层**就是** XArray |
| ⭐ "IDR 是建立在 XArray 之上的薄封装" | ✅ | 通过 `radix-tree.h` 的 typedef 兼容层 |

> ⭐ **为什么保留兼容层**：内核里有几十个子系统用 `radix_tree_*` API，一次性全改会造成巨大 churn。用 `#define` 让老名字映射到新类型，**新老代码可以共存**，逐步迁移。这是内核处理"大规模 API 迁移"的标准手法。

---

### 七、⭐ Maple tree：VMA 从 rbtree 迁过来的真正理由

v6.6 `include/linux/maple_tree.h:5-8`：

```c
/*
 * Maple Tree - An RCU-safe adaptive tree for storing ranges
 * Copyright (c) 2018-2022 Oracle
 * Authors:     Liam R. Howlett, Matthew Wilcox
 */
```

官方文档 `Documentation/core-api/maple_tree.rst:11-27`：

```
The Maple Tree is a B-Tree data type which is optimized for storing
non-overlapping ranges, including ranges of size 1.  The tree was designed to
be simple to use and does not require a user written search method.  It
supports iterating over a range of entries and going to the previous or next
entry in a cache-efficient manner.  The tree can also be put into an RCU-safe
mode of operation which allows reading and writing concurrently.  Writers must
synchronize on a lock, which can be the default spinlock, or the user can set
the lock to an external lock of a different type.

The Maple Tree maintains a small memory footprint and was designed to use
modern processor cache efficiently.  The majority of the users will be able to
use the normal API.  An advanced API exists for more complex scenarios.  The
most important usage of the Maple Tree is the tracking of the virtual memory
areas.
```

#### ⭐⭐ 三个关键词拆解

| 关键词 | 含义 | 对比 rbtree |
|--------|------|------------|
| ⭐ **B-Tree** | 节点内是**数组**，分支因子高（16~31） | rbtree 是**二叉**，每次比较一次指针追逐 |
| ⭐ **RCU-safe** | ⭐ **读侧完全无锁** | rbtree 的 rebalance 会改结构，RCU 读不安全 |
| **ranges** | 原生存区间（size-1 区间 = 点） | rbtree 只存点，VMA 区间要额外簿记 |

> ⭐⭐ **最关键的是 B-Tree vs 二叉树的 cache 差异**：
> - rbtree：深度 ≈ `log2(n)`。100 万个 VMA ≈ 20 层，**20 次指针追逐**，每次大概率 cache miss
> - maple tree：64 位下 `MAPLE_RANGE64_SLOTS = 16`，深度 ≈ `log16(n)`。100 万个 ≈ **5 层**，且每层节点内是连续数组（可预取）
>
> 这是"同样 O(log n)，常数差 4 倍 + cache 行为完全不同"的典型案例。

#### ⭐ 节点布局：256 字节对齐带来的三个技巧

```c
#if defined(CONFIG_64BIT) || defined(BUILD_VDSO32_64)
/* 64bit sizes */
#define MAPLE_NODE_SLOTS	31	/* 256 bytes including ->parent */
#define MAPLE_RANGE64_SLOTS	16	/* 256 bytes */
#define MAPLE_ARANGE64_SLOTS	10	/* 240 bytes */
#define MAPLE_ALLOC_SLOTS	(MAPLE_NODE_SLOTS - 1)
#else
/* 32bit sizes */
#define MAPLE_NODE_SLOTS	63	/* 256 bytes including ->parent */
...
#endif
```

**技巧 1 —— parent 指针的低位当标志位**

```c
 * Nodes in the tree point to their parent unless bit 0 is set.
```

节点 256 字节对齐 → `parent` 指针的**低 8 位永远是 0** → 可以借用。跟 XArray 的低 2 位打标是同一套思路。

**技巧 2 —— 已删除节点的 parent 指向自己**

```c
 * Removed nodes have their ->parent set to point to themselves.  RCU readers
 * check ->parent before relying on the value that they loaded from the
 * slots array.
```

RCU 读者在读到一个 slot 值后，检查 `parent` 是否自指 —— 若是，说明节点已删除，重试。这是一个**零成本的失效检测**（不需要额外原子操作）。

**技巧 3 —— ⭐ 复用 slots 数组当 RCU head**

```c
 * This lets us reuse the slots array for the RCU head.
```

节点被删除后，`slots[]` 数组不再有语义。与其额外加一个 `struct rcu_head` 字段（16 字节），**直接把 RCU 回调链表节点覆写在 slots 数组上**——节点大小一点没变。

> ⭐ 这是内核里"复用已死字段"的经典手法：数据结构在不同生命周期阶段，同一块内存可以承载不同语义。

#### rbtree 其实也用了低位打标

```c
#define rb_parent(r)   ((struct rb_node *)((r)->__rb_parent_color & ~3))
```

`__rb_parent_color` 把 **parent 指针 + 颜色**打包在一个 word 里（低 2 位存颜色）。同样的技巧，不同的结构。

#### rbtree 的"无通用 API"是刻意设计

v6.6 `include/linux/rbtree.h:6-13`：

```c
  To use rbtrees you'll have to implement your own insert and search cores.
  This will avoid us to use callbacks and to drop drammatically performances.
  I know it's not the cleaner way,  but in C (not in C++) to get
  performances and genericity...
```

> ⭐ 内核**故意不提供** `rb_insert(root, key, cmp_fn)` 这种带回调的通用 API —— 回调无法内联，每次比较都是间接调用。代价是每个使用者都要自己写比较逻辑，收益是**零抽象开销**。

#### 对比总表

| 维度 | `rbtree` | `maple tree` |
|------|---------|-------------|
| 树型 | 二叉搜索树 | ⭐ **B-Tree**（节点内数组） |
| 分支因子 | 2 | ⭐ **16（range64） / 31（node）** |
| 100 万元素深度 | ≈ 20 | ⭐ ≈ 5 |
| 缓存行为 | 每步一次指针追逐 | ⭐ 节点内连续，可预取 |
| ⭐ RCU 安全读 | ❌（只有两个 RCU 变体，无通用查找） | ⭐ **原生支持 RCU-safe 模式** |
| 区间语义 | 只存点 | ⭐ **原生存区间**（含 size-1） |
| 通用 API | ❌ 需自己写 search core | ⭐ 提供 `mtree_store/load/...` |
| 节点大小 | 24 字节（parent+color, left, right） | 256 字节（对齐，低 8 位可借用） |
| 引入版本 | 1999（Andrea Arcangeli） | ⭐ **v6.1** |
| v6.6 状态 | 仍广泛使用（CFS、timerfd、epoll…） | ⭐ **VMA 的主力**（官方点名） |

---

### 八、其他该知道的结构

| 结构 | 场景 | 关键点 |
|------|------|--------|
| **`hlist`** | 大哈希表的桶链 | 头节点只有 8 字节；`pprev` 二级指针消除边界判断 |
| **`llist`** | 多生产者批量收割 | ⭐ `add` + `del_all` 是 MPMC 无锁 |
| **`kfifo`** | SPSC FIFO | 容量必须 2 的幂；严格 SPSC 才无锁 |
| **`rhashtable`** | 需要自动扩容的哈希表 | 免手写扩容逻辑，RCU 友好 |
| **`interval tree`** | 区间重叠查询 | 与 maple tree 用途部分重叠 |
| **`bitmap`** | 密集小整数集合 | `alloc_percpu` 后每 CPU 无锁 |
| **`xarray`** | 稀疏整数索引 + tag | ⭐ 值条目零分配；锁内置 |
| **`maple tree`** | 区间 + RCU 读 | ⭐ v6.1+，VMA 主力 |

---

### 九、组合使用

| 组合 | 现实例子 |
|------|----------|
| list + rbtree | 同一任务既在 **全局链表** 又在 **CFS 树** |
| kfifo + workqueue | 中断 **in**，线程 **out** 处理 |
| xarray + tag | page cache：页索引树 + dirty/writeback 标记位 |
| maple tree + per-VMA lock | VMA 管理（v6.1+/v6.4+，见 [15.4](../../chapter-15-process-address-space/notes/section-15.4-内存区域的链表与树.md)） |
| ⭐ `llist` + `del_all` | 中断/多 CPU 惰性释放队列（`irq_work`、RCU 回调批处理） |

---

### 十、⭐ 选型的四个维度（不只是复杂度）

复杂度只是入场券。按重要性**从高到低**：

#### 维度 1：并发形状（最容易被忽略）

| 并发形状 | 首选 |
|----------|------|
| 单 CPU 独占数据 | 任何结构——**per-CPU 化**直接消灭共享（`alloc_percpu`，见 [12.10](../../chapter-12-memory-management/notes/section-12.10-每个-CPU-的分配.md)） |
| SPSC 跨上下文（中断→线程） | kfifo 无锁环 |
| ⭐ **MPMC + 批量收割** | `llist`（`add` + `del_all` 无锁） |
| 读极多写极少 | RCU 保护的任何结构（读侧零开销，见 [Ch 9](../../chapter-09-kernel-sync-intro/)） |
| 多写者有序键 | rbtree/xarray + spinlock；或 maple tree |
| ⭐ **需要 RCU 无锁读 + 区间** | **只能**是 maple tree |

#### 维度 2：渐进复杂度

| 需求 | 结构 | 复杂度 |
|------|------|--------|
| 全量遍历 | list | O(n) 但常数极小 |
| 点查（均匀键） | 哈希表 | O(1) |
| 有序 / 最值 / 前驱后继 | rbtree | O(log n) |
| 稀疏整数索引 | xarray | O(log n) 但常数小 |
| 区间查询 | maple tree | O(log n) |

#### 维度 3：⭐ 缓存布局（同样 O(log n)，常数差 4 倍）

| 结构 | 100 万元素的查找深度 | 每步 cache 行为 |
|------|---------------------|----------------|
| rbtree | ≈ **20** 层 | 每层一次随机指针追逐，大概率 cache miss |
| maple tree | ≈ **5** 层 | 节点内连续数组，可硬件预取 |
| 哈希表 | 1~2 次 | 取决于桶是否命中（可能跨页） |

> ⭐ 这是 [6.7 算法复杂度](./section-6.7-算法复杂度.md) 会展开的主题：**大 O 相同不代表性能相同，cache miss 数量常常才是主导项。**

#### 维度 4：内存开销

| 结构 | 每节点开销 | 备注 |
|------|-----------|------|
| `list_head` | 16 字节 | 头节点也是 16 字节 |
| `hlist_node` | 16 字节 | ⭐ 但**头节点只要 8 字节** |
| `rb_node` | 24 字节 | parent+color / left / right |
| maple 节点 | **256 字节** | 对齐，但分支因子高（摊薄下来更省） |
| xarray 节点 | 约 576 字节（64 槽） | 但值条目**零分配** |

> 原文那句总结仍是最好的：**先想并发形状，再想复杂度，最后想缓存布局。**（此处补第四维：内存开销。）

---

### 十一、反面教材：选错结构的代价史

| 案例 | 教训 | 版本 |
|------|------|------|
| 早期 O(n) 调度器 | 进程一多调度延迟不可预测 → O(1) 调度器（位图）→ CFS（rbtree）→ maple 时代仍在演化（见 [4.1](../../chapter-04-process-scheduling/notes/section-4.1-多任务与调度器演进.md)） | v2.4 → v2.6 → v2.6.23 |
| 2.6 VMA 链表+红黑树双簿记 | 每次增删两份维护 → v6.1 maple tree 一份搞定（见 [15.4](../../chapter-15-process-address-space/notes/section-15.4-内存区域的链表与树.md)） | ⭐ **v6.1** |
| radix tree + 外挂锁 | page cache 改 xarray：锁与 tag 进结构（见 [6.4](./section-6.4-映射.md)） | ⭐ **v4.20** |
| ⭐ IDR 与 radix tree 双份实现 | 统一到 XArray，`radix_tree_*` 降级为 `#define` 兼容层 | ⭐ **v4.20** |
| rbtree 的 RCU 读不安全 | VMA 查找无法做到读侧无锁 → 成为 per-VMA lock 的瓶颈 → maple tree | ⭐ **v6.1** |

**HFT：** 选型逻辑直接平移到用户态——行情网关收包→策略用 SPSC 环（对应 kfifo）；订单簿按价格有序用红黑树/跳表（对应 rbtree + [6.7](./section-6.7-算法复杂度.md) 的缓存维度）；fd 式会话句柄用稀疏数组/句柄表（对应 idr）。**先想并发形状，再想复杂度，最后想缓存布局。**

> ⭐ **用户态平移时最该抄的三条内核经验**：
> 1. **侵入式容器**（对象里嵌节点）消灭每次 insert 的 `malloc`
> 2. **SPSC 环的容量取 2 的幂**，用 `& mask` 代替 `%`
> 3. **B 树优于二叉搜索树**——同样的 O(log n)，深度差 4 倍，cache 行为天壤之别（这正是很多高性能 KV 存储用 B+ 树而非红黑树的原因）

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** 内核中遍历所有进程用什么数据结构？查找特定 PID 用什么？

<details><summary>答案</summary>

遍历所有进程：task_struct 通过 tasks 链表串联，list_for_each 遍历。查找特定 PID：用 PID 哈希表（`find_task_by_vpid()`），O(1) 查找。如果只有链表，查找 PID 需要 O(n) 遍历所有进程，系统中千个进程时太慢。

<details><summary>按 v6.6 补充</summary>

补充结构层面的细节：

**① 遍历用的是 `list_head` 侵入式链表**

```c
struct task_struct {
	...
	struct list_head	tasks;      /* 全局进程链表节点，嵌在对象里 */
	...
};
```

`for_each_process(p)` 本质就是 `list_for_each_entry(p, &init_task.tasks, tasks)`。

**② PID 查找用的是 `hlist` 哈希表**（不是 `list_head`）

因为哈希表有**很多桶**，桶头节点的 8 字节 vs 16 字节之差在大规模下很可观：

```c
struct hlist_head {
	struct hlist_node *first;        /* 8 字节 */
};
struct hlist_node {
	struct hlist_node *next, **pprev;
};
```

**③ ⭐ 同一个 `task_struct` 同时挂在多个结构上** —— 这正是侵入式设计的价值：

| 成员 | 结构 | 用途 |
|------|------|------|
| `tasks` | `list_head` | 全量遍历 |
| `pid_links[PIDTYPE_MAX]` | `hlist_node` | 按 PID 哈希查找 |
| `sibling` / `children` | `list_head` | 进程树关系 |
| `run_node` | `rb_node` | CFS 红黑树 |
| `thread_group` 等 | `list_head` | 线程组 |

一个对象、零额外分配、同时属于五种结构——用户态 `std::list<Task>` 做不到这点。

</details>
</details>

**Q2.** HFT 限价单簿应该用什么数据结构？

<details><summary>答案</summary>

限价单簿需要：按价格排序（找最优价 O(1)）、按价格查找（O(log n)）、插入/取消订单（O(log n)）。红黑树满足：找最优价 = 最左/最右节点，插入/删除 O(log n)。补充：同一价位多笔订单用链表挂在红黑树节点上。高频场景可用数组+堆优化（完全二叉树 cache 友好）。

<details><summary>按 v6.6 补充（内核视角的三条经验）</summary>

这个答案是对的，但补三条从内核数据结构能直接抄的经验：

**① 大 O 相同不代表性能相同——要看树型和缓存**

这正是 §7 讲的 rbtree vs maple tree 的教训：

| | 红黑树（二叉） | B 树（maple tree 型） |
|---|---|---|
| 100 万档位深度 | ≈ 20 层 | ≈ 5 层（分支因子 16） |
| 每步 cache 行为 | 随机指针追逐 | 节点内连续数组，可预取 |

⭐ **内核在 v6.1 把 VMA 从 rbtree 迁到 maple tree，主要理由就是这个**。订单簿完全可以照抄：价格档位用 **B+ 树**而非红黑树，深度和 cache miss 都会大幅下降。

**② "同一价位挂链表"就是内核的组合用法**

与 §9 的 "list + rbtree" 组合同构——一个 price level 对象同时是 B 树节点和订单链表的头。**用侵入式容器实现，零额外分配。**

**③ ⭐ 取消订单要 O(1)，不能只靠树**

订单簿的真实需求是"按 order_id 取消"，这需要**第二份索引**：

```
   B 树（按价格有序）          哈希表（按 order_id）
   ┌──────────────┐          ┌──────────────┐
   │ price_level  │          │ order_id → 节点│
   └──────────────┘          └──────────────┘
           └──────── 同一个 Order 对象 ────────┘
                    （侵入式：两个节点成员）
```

对应的内核手法就是 §1 讲的"一个对象嵌多个容器节点"。

**④ 价格用整数（ticks）而非浮点**

内核里绝不会用浮点数做树的键——原因不只是精度，还有**比较开销和确定性**。订单簿同理：`price_in_ticks = round(price * 10000)`。

</details>
</details>

**Q3.** 内核容器为什么都是"侵入式"的？用户态容器有什么不同？

<details><summary>答案</summary>

**侵入式（intrusive）** = 容器节点**嵌在对象里**，而不是容器为元素分配节点。

```c
struct my_object {
	int			value;
	struct list_head	list;      /* 节点嵌在对象里 */
	struct rb_node		rb;        /* 同一个对象可挂多条结构 */
};
```

| | 用户态容器（`std::list<T>`） | 内核容器 |
|---|------------------------|---------|
| 谁分配节点 | 容器分配（每次 insert 一次 `malloc`） | ⭐ **零额外分配** |
| 元素能否同时在多个容器 | 不能（除非存指针拷贝） | ⭐ **能**（多个节点成员） |
| 从节点找回对象 | 不需要 | ⭐ `container_of()` |
| 删除 | O(1) | O(1)（只需 head + 节点本身） |
| 内存开销 | 每元素一次分配（含 malloc 头 16B） | ⭐ 每节点 16 字节，无分配器开销 |

**三个理由：**

1. ⭐ **零分配**：内核路径上很多地方不能睡眠（原子上下文），不能 `kmalloc`。侵入式让 insert 变成纯指针操作。
2. ⭐ **一个对象属于多个结构**：见 Q1 的 `task_struct`——同时挂在全局链表、PID 哈希、进程树、CFS 红黑树上。
3. ⭐ **延迟可预测**：没有分配器介入，insert/remove 的最坏延迟是确定的常数。

**HFT 平移**：`std::map<K,V>` 每次 insert 都 `malloc`——这是**常被忽略的延迟尖峰来源**。低延迟订单簿的标准做法是"预分配对象池 + 侵入式链表/树"。

</details>

**Q4.** `hlist` 的 `pprev` 为什么是二级指针？

<details><summary>答案</summary>

```c
struct hlist_head {
	struct hlist_node *first;
};
struct hlist_node {
	struct hlist_node *next, **pprev;   /* ← pprev 是 struct hlist_node ** */
};
```

| 节点位置 | `pprev` 指向 |
|---------|-------------|
| 桶内**第一个**节点 | `&head->first` |
| 其他节点 | `&前一个节点->next` |

**收益：删除操作不需要边界判断**

```c
static inline void __hlist_del(struct hlist_node *n)
{
	struct hlist_node *next = n->next;
	struct hlist_node **pprev = n->pprev;

	WRITE_ONCE(*pprev, next);              /* 前驱"指向我的字段"改指 next */
	if (next)
		WRITE_ONCE(next->pprev, pprev);   /* next 的 pprev 接管 */
}
```

对比普通双向链表必须写的边界判断：
```c
if (node->prev == head) head->first = node->next;   /* 是不是第一个？ */
else node->prev->next = node->next;
```

⭐ **少一个分支 = 少一次分支预测失败**。这是内核里反复出现的技巧（同样手法见 `plist`、`rbtree` 的 parent 编码、radix tree 的 slot 操作）。

**另一个收益是头节点只要 8 字节**：`list_head` 的头是 16 字节（`next`+`prev`），`hlist_head` 只有 8 字节（只有 `first`）。大哈希表的桶数常常是几万到几十万，**每个桶省 8 字节**就是几十 KB 到几 MB。

</details>

**Q5.** `llist` 在哪些操作组合下不需要加锁？

<details><summary>答案</summary>

v6.6 `include/linux/llist.h` 头部注释直接给出并发兼容矩阵：

```
           |   add    | del_first |  del_all
 add       |    -     |     -     |     -
 del_first |          |     L     |     L
 del_all   |          |           |     -
```

（`-` = 无需加锁，`L` = 需要加锁）

| 操作组合 | 需要锁？ |
|---------|---------|
| `llist_add` × `llist_add` | ⭐ 否 |
| `llist_add` × `llist_del_first` | ⭐ 否 |
| ⭐ **`llist_add` × `llist_del_all`** | ⭐ **否 → MPMC 无锁** |
| `llist_del_first` × `llist_del_first` | **是** |
| `llist_del_first` × `llist_del_all` | **是** |
| `llist_del_all` × `llist_del_all` | ⭐ 否（各自 `xchg` 拿走整条链） |

⭐⭐ **核心结论**：**`llist_add` + `llist_del_all` 是 MPMC 无锁的**——多生产者多消费者，零锁。这是 `llist` 的典型用法（`irq_work`、惰性释放队列、RCU 回调批处理）。

**为什么 `del_first` 需要锁**（注释原文）：

> "`llist_del_first` depends on `list->first->next` not changing, but without lock protection, there's no way to be sure about that if a preemption happens in the middle of the delete operation and on being preempted back, the `list->first` is the same as before causing the `cmpxchg` in `llist_del_first` to succeed."

即典型的 **ABA 问题**：`del_first` 的 `cmpxchg` 检查的是"头指针没变"，但被抢占期间另一个消费者可能已经改过链表——头指针看起来一样，语义却完全不同。

</details>

**Q6.** XArray 怎么用指针的低 2 位？"值条目"有什么用？

<details><summary>答案</summary>

v6.6 `include/linux/xarray.h:23-42`：

```
 * The bottom two bits of the entry determine how the XArray interprets
 * the contents:
 *
 * 00: Pointer entry
 * 10: Internal entry
 * x1: Value entry or tagged pointer
 *
 * 0-62: Sibling entries
 * 256: Retry entry
 * 257: Zero entry
 *
 * Errors are also represented as internal entries, but use the negative
 * space (-4094 to -2).  They're never stored in the slots array; only
 * returned by the normal API.
```

| 低 2 位 | 含义 |
|--------|------|
| `00` | 普通指针（对齐保证最低 2 位为 0） |
| `10` | 内部条目：树节点指针 / 特殊标记（0-62 兄弟、256 retry、257 zero） |
| `x1` | ⭐ **值条目**：整数直接存在"指针"里 |

⭐⭐ **值条目的价值**：

```c
#define BITS_PER_XA_VALUE	(BITS_PER_LONG - 1)     /* 64 位机 = 63 位 */
```

存一个小整数时**完全不需要分配节点** —— `xa_mk_value(v)` 把 `v` 左移 1 位并置低位 1，`xa_to_value(e)` 反向。

对于"稀疏 ID → 小整数"的场景（计数器、标志、小 ID），这**省掉每一次的内存分配**。

**错误码的编码**（呼应 [5.2 的 `MAX_ERRNO`](../../chapter-05-system-calls/notes/section-5.2-系统调用基础.md)）：

错误码用负空间 `-4094 ~ -2`，与 `MAX_ERRNO = 4095` 是同一套体系。且注释强调：

> "They're **never stored** in the slots array; only returned by the normal API."

即错误只出现在返回值里，不会污染树的内容。

**版本断崖**：

| 版本 | `xarray.h` 大小 | 状态 |
|------|---------------|------|
| v4.18 / v4.19 | 818 字节 | 占位骨架（无 `struct xarray`） |
| ⭐ **v4.20** | **45558 字节**（50 处 `struct xarray`） | 完整实现落地 |

</details>

**Q7.** v6.6 里 IDR 和 XArray 是什么关系？

<details><summary>答案</summary>

⭐ **反直觉的事实：IDR 没有"被取代而消失"，它底层就是 XArray。**

v6.6 `include/linux/idr.h:14-24` 看起来还在用 radix tree：

```c
#include <linux/radix-tree.h>
...
struct idr {
	struct radix_tree_root	idr_rt;
	unsigned int		idr_base;
	unsigned int		idr_next;
};
```

但看 v6.6 `include/linux/radix-tree.h:10-28`：

```c
#include <linux/xarray.h>
#include <linux/local_lock.h>

/* Keep unconverted code working */
#define radix_tree_root		xarray
#define radix_tree_node		xa_node
```

⭐⭐ **`radix_tree_root` 已经被 `#define` 成 `xarray` 了**，注释原文 `/* Keep unconverted code working */`。

真实的继承链：

```
struct idr
   └─ struct radix_tree_root idr_rt
        └─ = struct xarray          （radix-tree.h 的 #define 兼容层）
```

| 常见说法 | 对不对 | 实际 |
|---------|-------|------|
| "IDR 被 XArray 取代，IDR 没了" | ❌ | IDR 的 API 仍在（`idr_alloc`、`idr_find`…） |
| "IDR 和 XArray 是两套独立实现" | ❌ | ⭐ IDR 底层**就是** XArray |
| ⭐ "IDR 是 XArray 之上的薄封装" | ✅ | 经 `radix-tree.h` 的 typedef 兼容层 |

**为什么保留兼容层**：内核里有几十个子系统用 `radix_tree_*` API，一次性全改会造成巨大 churn。用 `#define` 让老名字映射到新类型，**新老代码共存，逐步迁移**。这是内核处理"大规模 API 迁移"的标准手法。

</details>

**Q8.** 为什么内核在 v6.1 把 VMA 从红黑树迁到 maple tree？

<details><summary>答案</summary>

官方文档 `Documentation/core-api/maple_tree.rst` 的定义：

> "The Maple Tree is a **B-Tree** data type which is optimized for storing **non-overlapping ranges**... It supports iterating over a range of entries and going to the previous or next entry in a **cache-efficient** manner. The tree can also be put into an **RCU-safe** mode of operation which allows reading and writing concurrently."
>
> "**The most important usage of the Maple Tree is the tracking of the virtual memory areas.**"

#### ⭐⭐ 三个核心理由

**① B-Tree vs 二叉树 —— 同样 O(log n)，深度差 4 倍**

| | rbtree（二叉） | maple tree（B-Tree） |
|---|---|---|
| 分支因子 | 2 | ⭐ **16**（`MAPLE_RANGE64_SLOTS`） |
| 100 万元素深度 | ≈ 20 层 | ⭐ ≈ 5 层 |
| 每层 cache 行为 | 随机指针追逐 | ⭐ 节点内连续数组，可硬件预取 |

节点 256 字节对齐，64 位下 `MAPLE_RANGE64_SLOTS = 16`、`MAPLE_NODE_SLOTS = 31`。

**② ⭐ RCU-safe —— rbtree 的根本短板**

rbtree 只有 `rb_replace_node_rcu()` 和 `rb_link_node_rcu()` 两个 RCU 变体，**没有通用的 RCU 安全查找**。因为 rebalance 会改变树结构，RCU 读者可能读到正在被旋转的节点。

而 per-VMA lock（v6.4+）要求 **VMA 查找的读侧无锁**——rbtree 做不到，maple tree 原生支持。

**③ 原生区间语义**

VMA 是**区间**（`[vm_start, vm_end)`），不是点。rbtree 只存点，需要额外的链表做双簿记；maple tree 原生存区间，"size 1 的区间"就是点。

#### ⭐ 附：maple tree 的三个实现技巧

1. **parent 指针低位当标志**：节点 256B 对齐 → `parent` 低 8 位恒为 0 → bit 0 借作标志（同 XArray 的低位打标）
2. **已删除节点的 parent 指向自己**：RCU 读者读 slot 后检查 `parent` 是否自指，若是则重试——**零成本的失效检测**
3. ⭐ **复用 slots 数组当 RCU head**：节点删除后 slots 数组失去语义，直接把 `rcu_head` 覆写在上面，**节点大小一点没变**

#### 补充：rbtree 其实也用了低位打标

```c
#define rb_parent(r)   ((struct rb_node *)((r)->__rb_parent_color & ~3))
```

`__rb_parent_color` 把 parent 指针 + 颜色打包进一个 word（低 2 位存颜色）。同样的技巧，不同的结构。

**版本**：maple tree 引入于 **v6.1**（`include/linux/maple_tree.h`：v6.0 = 14 字节 404 残片 / v6.1 = 23073 字节）。

</details>

**Q9.** 内核为什么不给 rbtree 提供带回调的通用 insert/search API？

<details><summary>答案</summary>

v6.6 `include/linux/rbtree.h:6-13` 的作者注释直接回答了：

```
  To use rbtrees you'll have to implement your own insert and search cores.
  This will avoid us to use callbacks and to drop drammatically performances.
  I know it's not the cleaner way,  but in C (not in C++) to get
  performances and genericity...
```

⭐ **原因：回调无法内联，每次比较都是一次间接调用。**

| 方案 | 比较成本 | 灵活性 |
|------|---------|-------|
| 通用 API + 函数指针回调 | 每次比较 = 一次间接调用（不能被内联、不能被常量传播优化） | 高 |
| ⭐ 内核做法：用户自己写比较逻辑 | 比较被完全内联，编译器可跨比较优化 | 低（每个使用者写一份） |

在 C++ 里这可以用模板解决（`std::map<K,V,Compare>` 的 Compare 是编译期绑定的，可内联）。C 语言没有模板，内核选择**牺牲易用性换零抽象开销**。

**代价**：每个 rbtree 使用者都要自己写一份 `search` / `insert` 核心（内核里有几十份几乎一样的 rbtree 搜索代码）。

**对比**：新的 maple tree 提供了通用 `mtree_store/load/...` API——因为它不需要用户提供的比较函数（键就是 `unsigned long` 区间），**回调问题根本不存在**。这也是它被官方称为 "designed to be simple to use and does not require a user written search method" 的原因。

</details>

**Q10.** 选型时应该按哪几个维度考虑？为什么"复杂度"不是第一位的？

<details><summary>答案</summary>

按重要性**从高到低**四个维度：

#### 维度 1：⭐ 并发形状（最容易被忽略，也最容易致命）

| 并发形状 | 首选 |
|----------|------|
| 单 CPU 独占 | per-CPU 化，直接消灭共享 |
| SPSC 跨上下文 | `kfifo` 无锁环 |
| ⭐ MPMC + 批量收割 | `llist`（`add` + `del_all` 无锁） |
| 读极多写极少 | RCU 保护的结构（读侧零开销） |
| ⭐ RCU 无锁读 + 区间 | **只能**是 maple tree |

**为什么它第一**：并发形状决定了"能不能无锁 / 要不要阻塞"。一个需要持锁的结构，即使 O(1)，在争用下也会退化成 O(n) 的等待；而一个无锁的 O(log n) 结构反而更快。

#### 维度 2：渐进复杂度

| 需求 | 结构 |
|------|------|
| 全量遍历 | list（O(n) 但常数极小） |
| 点查（均匀键） | 哈希表 O(1) |
| 有序 / 最值 | rbtree O(log n) |
| 稀疏整数索引 | xarray O(log n) 常数小 |
| 区间查询 | maple tree O(log n) |

#### 维度 3：⭐ 缓存布局

| 结构 | 100 万元素深度 | 每步 cache 行为 |
|------|-------------|---------------|
| rbtree | ≈ 20 层 | 随机指针追逐 |
| maple tree | ≈ 5 层 | 节点内连续数组，可预取 |

⭐ **大 O 相同不代表性能相同**——v6.1 把 VMA 从 rbtree 迁到 maple tree 就是这个维度驱动的。

#### 维度 4：内存开销

| 结构 | 每节点 | 备注 |
|------|-------|------|
| `list_head` | 16 B | 头也是 16 B |
| `hlist_node` | 16 B | ⭐ **头只要 8 B** |
| `rb_node` | 24 B | |
| maple 节点 | 256 B | 对齐，但分支因子高，摊薄后更省 |
| xarray 节点 | ~576 B（64 槽） | ⭐ 但值条目零分配 |

---

原文那句总结仍是最好的：

> **先想并发形状，再想复杂度，最后想缓存布局。**

（本笔记补充第四维：内存开销。）

**HFT 平移时最该抄的三条内核经验：**

1. **侵入式容器**（对象里嵌节点）消灭每次 insert 的 `malloc`
2. **SPSC 环容量取 2 的幂**，用 `& mask` 代替 `%`
3. ⭐ **B 树优于二叉搜索树**——同样 O(log n)，深度差 4 倍，cache 行为天壤之别（这正是很多高性能 KV 存储用 B+ 树而非红黑树的原因）

</details>

</details>

---

→ [6.7 算法复杂度](./section-6.7-算法复杂度.md) · [Ch 9 内核同步](../../chapter-09-kernel-sync-intro/) · [Ch 15.4 VMA 的链表与树](../../chapter-15-process-address-space/notes/section-15.4-内存区域的链表与树.md) · [Ch 10 排序和屏障](../../chapter-10-sync-methods/notes/section-10.10-排序和屏障.md)
