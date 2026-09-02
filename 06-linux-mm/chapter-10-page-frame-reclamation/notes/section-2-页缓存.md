# Ch 10 §2 页缓存 (Page Cache)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**
> 源码核验：Linux **v6.6**（`include/linux/fs.h` 的 `struct address_space`、`include/linux/mm_types.h` 的 `struct folio`）

---

## 本节讲什么

file 链上挂的页，绝大多数来自**页缓存**。本节回答：

1. 页缓存靠什么索引「同一文件同一偏移」的页？
2. v6.6 的索引结构从原书的「哈希表」换成了什么？
3. 页缓存和进程 RSS 为什么是两笔账？

---

## 1. 页缓存的本质

磁盘数据在 RAM 里的缓存，避免重复读盘。**页缓存是「按文件组织」的**——每个文件（inode）对应一个 `struct address_space`，文件内每个偏移对应一个 folio（页）。

| 页缓存中的页 | 来源 |
|-------------|------|
| 文件 mmap 缺页 | 映射文件读入 |
| `read()` 路径 | 预读（readahead）批量读入 |
| swap cache 匿名页 | 换出/换入过程中的匿名页 |
| 共享内存页 | shm / mmap shared（tmpfs 背后也是页缓存） |

---

## 2. 索引结构：`struct address_space`（v6.6 `fs.h:470`）

```c
struct address_space {
    struct inode *host;                 /* 归属哪个文件 */
    struct xarray i_pages;              /* ⭐ 页缓存索引：xarray */
    atomic_t i_mmap_writable;
    struct rb_root_cached i_mmap;       /* 映射此文件的所有 VMA（rmap 前向索引） */
    unsigned long nrpages;              /* 页数 */
    pgoff_t writeback_index;            /* 写回游标（循环写回脏页） */
    const struct address_space_operations *a_ops;  /* readpage/writepage 等回调 */
    /* ... */
} __attribute__((aligned(sizeof(long)))) __randomize_layout;

/* 页缓存的 dirty/writeback 标记，复用 XArray tags */
#define PAGECACHE_TAG_DIRTY     XA_MARK_0
#define PAGECACHE_TAG_WRITEBACK XA_MARK_1
```

**关键：索引从「哈希表」换成了 xarray。** 原书（2.4/2.6）用哈希表 + 桶定位页；现代内核（v4.20 起）用 **xarray**（`i_pages`）——一种按「文件内偏移（pgoff）」稀疏存储的基数树，支持**范围操作**和 **tags**。dirty/writeback 页直接打 xarray tag（`PAGECACHE_TAG_DIRTY`），回收器遍历脏页时**不用线性扫描整棵树**，按 tag 就能抓出来。

**`i_mmap` 红黑树：** 反过来，从「文件」找「所有映射了它的 VMA」——这是 rmap（§5）做「解映射」的前向索引，与 `struct page` 上的反向索引互补。

---

## 3. 页缓存的单元：folio

v6.6 里页缓存的单位从 `struct page` 迁到了 `struct folio`（`mm_types.h:293`）。大文件顺序读写时，一个 folio 可能覆盖多个连续页（order-2 = 8 页），减少 `mapping/index` 的重复管理开销。THP（透明大页）映射的文件页也是 folio。

---

## 4. 读路径与预读（readahead）

```
read(fd, buf, n)
  → 查 i_pages 命中？ 返回
  → miss → readahead 按顺序预读一批页进页缓存（folio）
  → 命中后续读 → 全内存，不碰盘
```

预读是页缓存性能的关键：顺序读时**提前把后面的页读进来**，把磁盘延迟摊薄。这也是为什么「大文件顺序读」比「随机读」快一个数量级。

---

## 5. HFT / 嵌入式关联

| 现象 | 本节机制的兑现 |
|------|----------------|
| 大行情文件 mmap 只读 | 占的是**页缓存**（file 链），与进程 RSS 分开统计 |
| `echo 3 > drop_caches` | 回收**干净**页缓存，不碰 dirty（要先 writeback）和 mlock 页 |
| 内存看似被"吃满" | `free` 里 buff/cache 就是页缓存——**可回收**，不是泄漏 |
| 回放引擎随机读 | 随机读打不中预读，页缓存命中率低 → 延迟高 |

---

## 6. 衔接

- 上节 [§1 页面替换策略](./section-1-页面替换策略.md)：页缓存页住在 file 链
- [§3 LRU 链表管理](./section-3-LRU-链表管理.md)：file 链怎么被扫描
- [§5 换出进程页面](./section-5-换出进程页面.md)：文件页回收 vs 匿名页 swap
- 前置：[Ch 2 §3 struct page](../../chapter-02-describing-physical-memory/notes/section-3-物理页框.md)（folio/mapping）

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：为什么页缓存索引从哈希表换成了 xarray？**
A：哈希表按偏移散列，做不了**高效的范围操作**（「把 offset 100~200 的页全清掉」要逐项哈希）；xarray 按偏移有序稀疏存储，天然支持范围查询、范围失效（`invalidate_mapping_pages`）、以及用 tags 快速标记/查找 dirty 页。回收器要「找所有脏页」，xarray tag 扫一遍即可，不用遍历全部页。

**Q2：`i_pages`（xarray）和 `i_mmap`（红黑树）分别索引什么？**
A：`i_pages` 索引「文件偏移 → folio」（**正向**：从文件找数据页）；`i_mmap` 索引「文件 → 映射它的所有 VMA」（**反向**：从文件找谁映射了我）。回收时要解映射，先走 `i_mmap` 找到所有 VMA，再逐个清 PTE。两者配合才构成完整的文件页回收链路。

**Q3：页缓存「吃满内存」是坏事吗？**
A：不是。`free` 里的 buff/cache 是**可回收**的页缓存——内存紧张时回收器优先踢干净的文件页。真正该警惕的是**匿名页**（RSS）膨胀和 **dirty 页积压**（要 writeback 才能回收，且写回慢）。HFT 里大 mmap 文件吃页缓存是正常的，别误判成泄漏去 `drop_caches`（那反而触发后续读盘抖动）。

**Q4：`drop_caches` 能回收所有页缓存吗？**
A：不能。它只回收**干净**页（`echo 3` 还包含 reclaimable slab）。dirty 页要先经过 writeback 才能清；被 mlock 的页在 unevictable 链上，`drop_caches` 也碰不到。所以 `drop_caches` 不是「一键清空」，而是「清掉可安全丢弃的部分」。

**Q5：为什么页缓存用 folio 而不是单个 `struct page`？**
A：大文件顺序读/写时，连续 8 页（甚至更多）共享同一个 `mapping`、`index` 等信息。folio 把这些元数据**只存一份**，`struct page` 瘦身成薄包装，省内存、也减少 `mapping/index` 的重复读写。THP 映射的文件也是 folio，一个 order-9 folio 代表 2MiB。

</details>

---
