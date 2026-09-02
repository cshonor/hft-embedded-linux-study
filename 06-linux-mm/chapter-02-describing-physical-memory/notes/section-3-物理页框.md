# Ch 2 §3 物理页框 (Pages)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**
> 源码核验：Linux **v6.6**（`include/linux/mm_types.h`）

---

## 本节讲什么

物理内存的最小管理单元是**页框（page frame）**，每个页框对应一个 `struct page`。本节回答：

1. `struct page` 为什么是内核里**最省内存的结构**——字段复用怎么玩？
2. v6.6 里 `struct page` 真身长什么样，和 `struct folio` 什么关系？
3. 页标志（page flags）怎么和回收/写回/swap 联动？

原书 2.6 语境强调 `mapping / count / flags` 三字段；v6.6 的 `struct page`（`mm_types.h:74`）已大量瘦身，把字段搬进 `struct folio`，但「一页一描述、字段复用、低位借位」三大技巧完全保留。

---

## 1. 为什么 `struct page` 必须极致省内存

一台 32GiB 机器、4KiB 页框 → **800 万个页框**，每个都要一份 `struct page`。`struct page` 每增大 1 字节，全系统多耗 8MiB 内存（而且是从「内存」里扣内存，讽刺的自我吞噬）。所以内核让它：

1. **字段按用途复用**（一个 union 塞进 page cache 页、匿名页、slab 页、网络页池……多种身份）
2. **低位借位**（指针/整型没用的低位拿来存标志）

这与 [09-DMA §12.3 `scatterlist.page_link` 低 2 位复用](../../../09-device-drivers-dt/modern-driver-practice/chapter-12-dma/12.3-scatter-gather.md) 是**同一族技巧**。

---

## 2. `struct page` 真身（v6.6 `mm_types.h:74`）

```c
struct page {
    unsigned long flags;    /* 原子标志，部分可被异步更新 */

    /* 5 个 word（20/40 字节）的复用 union —— 核心 */
    union {
        struct {            /* ① page cache / 匿名页 */
            union {
                struct list_head lru;         /* active/inactive LRU 链 */
                struct list_head buddy_list;  /* 空闲时挂 buddy 链表 */
                struct list_head pcp_list;    /* 空闲时挂 per-CPU 链 */
            };
            struct address_space *mapping;    /* 所属文件 / anon_vma */
            union {
                pgoff_t index;                /* 文件内偏移（页单位） */
                unsigned long share;          /* fsdax 共享计数 */
            };
            unsigned long private;            /* buffer_head / swp_entry / buddy order */
        };
        struct {            /* ② 网络栈 page_pool */
            unsigned long pp_magic;
            struct page_pool *pp;
            unsigned long dma_addr;           /* 页的 DMA 地址 */
        };
        struct {            /* ③ 复合页的尾页 */
            unsigned long compound_head;      /* bit0 = 1 表示是尾页 */
        };
        struct {            /* ④ ZONE_DEVICE */
            struct dev_pagemap *pgmap;
            void *zone_device_data;
        };
        struct rcu_head rcu_head;             /* ⑤ RCU 释放 */
    };

    union {                 /* 4 字节 */
        atomic_t _mapcount;        /* 被用户页表引用的次数 */
        unsigned int page_type;    /* 非映射页的类型（PageSlab 等） */
    };

    atomic_t _refcount;    /* 引用计数，勿直接用，走 page_ref_*() */
    /* ... */
} _struct_page_alignment;
```

**关键注释（`mm_types.h:80`）：union 第一个 word 的 bit0 被 `PageTail()` 占用，其他成员一律不得用 bit0**——避免和「复合页尾页判定」冲突。这就是「低位借位」的活例。

---

## 3. 字段复用全景图

同一块内存，`struct page` 在不同「身份」下读出完全不同的字段：

```
                    struct page（同一个）
                          │
        ┌─────────────────┼──────────────────────┬──────────────────┐
        ▼                 ▼                       ▼                  ▼
 ① 文件/匿名页      ② 网络页池(page_pool)    ③ 复合页尾页       ④ ZONE_DEVICE
  lru/mapping        pp_magic/pp/dma_addr     compound_head       pgmap
  index/private                                    (bit0=1)
        │
        ▼
  空闲时又变回 buddy_list / pcp_list（挂在 free_area 或 per-CPU 链）
```

**什么时候是谁，由 `flags` 和上下文决定**：`PageBuddy()` 看 `private` 里的 order，`PageSlab()` 看 `page_type`，`PageTail()` 看 `compound_head` 的 bit0。没有多余的「type 字段」——**身份编码在 flags + 复用字段 + 低位 bit 里**。

---

## 4. `struct folio`：v6.6 的「新内存单元」

```c
/* mm_types.h:293 */
struct folio { ... };
```

**背景：** 内核越来越频繁地管理**多个连续页**（透明大页 THP、页缓存大块读写）。旧内核用「compound page」——首页 + 一串尾页，`PageTail()` 标记尾页。这套东西到处是 `PageHead/PageTail/PageCompound` 判断，容易错。

**folio 的解法：** 把「一小组连续页」当作**一个一等公民**，`struct folio` 拥有原来散在 `struct page` 里的 `flags/lru/mapping/index/_refcount` 等字段；`struct page` 瘦身成「folio 的薄包装」。THP 就是「一个 order-9 的 folio」。

| | 旧（compound page） | 新（folio） |
|---|---|---|
| 大页表示 | 首页 + N 个尾页（PageTail 链） | 一个 folio，`folio_nr_pages()` 给出页数 |
| 引用计数 | 首页 `_refcount` 代表整组 | `folio->_refcount` |
| 判断大页 | `PageCompound()` | `folio_test_large()` |

**对读原书的影响：** Gorman 原书没有 folio（2004 年），但你读 v6.6 源码时，页缓存、回收、THP 的入口基本都换成 folio 了。**概念层 `struct page` 的直觉不变，落地层记住「多页操作走 folio」即可。**

---

## 5. 页标志（Page Flags）与回收/写回联动

| 标志 | 含义 | 谁置位/清除 |
|------|------|-------------|
| `PG_locked` | 正被 I/O 锁定 | I/O 发起/完成 |
| `PG_dirty` | 内容已改，需写回 | 写后置位，写回完成清除 |
| `PG_active` / `PG_referenced` | 热页 / 被访问过 | LRU 活跃度跟踪 |
| `PG_uptodate` | 已从磁盘读入，内容有效 | 读完成置位 |
| `PG_slab` | 属于 slab 分配器（不是普通页） | slab 分配时 |
| `PG_compound` / `PageTail` | 复合页首页 / 尾页 | 大页分配时 |

回收器决定**踢哪一页**，就是看哪个 zone 的哪条 LRU、什么 flags——`PG_dirty` 要先写回、`PG_locked` 要等 I/O、`PG_referenced` 决定活跃度。Ch 10 回收、Ch 11 swap 全部建立在这些 bit 上。

---

## 6. 代码示例

```c
/* PFN 与 struct page 互转（FLATMEM 下就是指针算术） */
struct page *p = pfn_to_page(pfn);
unsigned long pfn = page_to_pfn(p);

/* 复合页：尾页 -> 首页 */
struct page *head = compound_head(page);

/* 引用计数：禁止直接 ++_refcount，走封装 */
get_page(page);   /* 引用 +1 */
put_page(page);   /* 引用 -1，归零触发释放 */
```

---

## 7. HFT / 嵌入式关联

| 现象 | 本节机制的兑现 |
|------|----------------|
| DPDK/网络栈收包页 | `struct page` 的 `② page_pool` 分支——同一结构为网络栈存 `dma_addr`，收包零拷贝的底座 |
| 大页/THP 内存池 | folio 表示 order-N 连续页，`mlock` 大页是钉住整个 folio |
| slab 对象池 | `PageSlab` 把页标记为「归 slab 管」，`page_type` 记录是哪个 slab cache |
| 伪共享/缓存行 | 页本身 `_struct_page_alignment` 对齐，配合 §5 的 zone padding 同族优化 |

---

## 8. 衔接

- 上节 [§2 内存区域](./section-2-内存区域.md)：`struct page` 挂在 zone 的 `free_area[]` 里
- [§4 高端内存](./section-4-高端内存.md)：32 位下部分页不能被内核直接寻址
- 分配/回收：[Ch 6 物理页分配](../../chapter-06-physical-page-allocation/) · [Ch 10 页框回收](../../chapter-10-page-frame-reclamation/)
- 字段复用同族技巧：[09-DMA 12.3 scatterlist](../../../09-device-drivers-dt/modern-driver-practice/chapter-12-dma/12.3-scatter-gather.md)

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：`struct page` 一个 union 塞 5 种身份，内核怎么知道当前是哪种？**
A：靠 `flags` + `page_type` + 复用字段里的特殊编码综合判断。例如 `PageBuddy()` 检查 `page_type` 是否为 buddy 类型且 `private` 存的是 order；`PageTail()` 直接看 union 第一个 word 的 bit0。没有单一「type 字段」，判定分散在 `page-flags.h` 的一组 `PageXXX()` 宏里。

**Q2：为什么 union 第一个 word 的 bit0 不许别人用？**
A：`PageTail()` 用 `compound_head & 1` 判尾页，而 `lru.next`、`mapping` 这些指针按对齐保证低几位恒为 0。一旦有人用了 bit0，就会和尾页判定冲突，把普通页误判成复合页尾页。这和 scatterlist 的 `page_link` 用低 2 位存 `SG_CHAIN/SG_END` 是同一个「对齐白送低位」的玩法。

**Q3：`_refcount` 注释说「不要直接用」，为什么？**
A：引用计数在 hot path 被频繁增减，直接 `atomic_inc` 会在多核上产生锁总线/cache line 争用。`page_ref_*()` 封装在 `CONFIG_DEBUG_PAGE_REF` 下加了溢出检查，正常路径用 per-cpu 偏置计数（`page_ref_unfreeze` 等）降低争用。

**Q4：folio 和 compound page 是同一回事吗？**
A：概念等价（都是「一组连续页」），但 folio 是**重构后的表示**：把 `struct page` 里的核心字段提升到 `struct folio`，`struct page` 退化为薄包装。THP 就是 order-9 的 folio，`folio_nr_pages()` 返回页数。读 v6.6 源码时页缓存/回收入口大多已是 folio。

**Q5：一个物理页同时被两个进程映射（如 fork 后未 COW），有几个 `struct page`？几个 `_mapcount`？**
A：**一个** `struct page`（物理页全局唯一），但 `_mapcount` 记录的是「被多少个用户页表项引用」——这里会是 2（两个进程的 PTE 都指它）。`_refcount` 则记录「内核有多少处持有它」（页缓存、rmap 等）。`_mapcount` 和 `_refcount` 是两个不同维度：前者是映射数，后者是引用数。

</details>

---
