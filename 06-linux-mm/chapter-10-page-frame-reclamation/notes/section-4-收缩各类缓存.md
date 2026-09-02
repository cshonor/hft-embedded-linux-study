# Ch 10 §4 收缩各类缓存 (`shrink_caches`)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**
> 源码核验：Linux **v6.6**（`include/linux/shrinker.h` 的 `struct shrinker`）

---

## 本节讲什么

回收不止缩 page cache 和匿名页——**slab、dentry、inode** 这些「非 LRU」缓存也得缩。本节讲：

1. shrinker 回调机制是什么？谁注册、谁调用？
2. `struct shrinker` 的 `seeks` 字段为什么决定了回收优先级？
3. 「级联效应」——回收小对象为什么能腾出大内存？

---

## 1. 内存告急时不止缩 page cache

```
shrink_node (概念)
    ├─ shrink_lruvec → 缩 page cache / 匿名页（LRU 链，§1-§3）
    └─ shrink_slab → 缩 slab / dcache / icache（shrinker 回调，本节）
```

LRU 链只覆盖「用户可见的页」（文件页、匿名页）。但内核还有大量**内核自己分配**的内存——slab 对象、目录项缓存（dcache）、inode 缓存（icache）、磁盘配额缓存（dqcache）。它们不进 LRU，回收走另一条路：**shrinker 回调**。

---

## 2. `struct shrinker` 真身（v6.6 `shrinker.h:63`）

```c
struct shrinker {
    unsigned long (*count_objects)(struct shrinker *,
                                   struct shrink_control *sc);
    unsigned long (*scan_objects)(struct shrinker *,
                                  struct shrink_control *sc);
    long batch;   /* 回收批量，0 = 默认 */
    int seeks;    /* 重建一个对象的代价（seek 次数） */
    unsigned flags;
    atomic_long_t *nr_deferred;  /* per-node 延迟删除计数 */
};
```

- **`count_objects()`**：报告「当前有多少对象可回收」（廉价计数）
- **`scan_objects()`**：实际回收对象（真正干活）
- **`seeks`**：重建这个对象的**代价**，用「磁盘 seek 次数」度量

各缓存子系统启动时 `register_shrinker()` 注册自己的 shrinker：dcache 注册 dentry shrinker、icache 注册 inode shrinker、slab 各 cache 注册 slab shrinker。回收器在 `shrink_slab()` 里遍历所有已注册的 shrinker，逐个问「能收多少、收给我看」。

---

## 3. `seeks` 决定优先级

回收是「**用最小代价腾最多内存**」。`seeks` 量化「踢掉这个对象后再要用它，得付出多大代价」：

| `seeks` | 含义 | 示例 |
|---------|------|------|
| 小（如 1） | 重建便宜，优先回收 | dentry（再 lookup 一次就行） |
| 大（如 16） | 重建贵，尽量留 | 某些需重算的 slab 对象 |

`shrink_slab()` 里用 `seeks` 加权，让「便宜重建」的缓存多收、「昂贵重建」的少收——**按重建成本排序的回收**。

---

## 4. 级联效应：小对象腾大内存

dentry / inode **对象本身很小**（几十到几百字节），但释放它们会**连带**释放其关联的 buffer head、page cache 页：

```
回收一个 dentry（~200B）
    → 触发其 inode 释放
        → 触发其关联的 page cache 页释放（每页 4KiB）
            → 物理页大量回落 Buddy
```

所以「回收小对象」的价值不在对象本身，而在**间接腾出的大页**。这也是为什么内存压力下 dcache/icache 常被先扫。

---

## 5. HFT / 嵌入式关联

| 现象 | 本节机制的兑现 |
|------|----------------|
| 频繁 open/close 文件 | dcache/icache 累积，压力下被 shrink，下次 open 重新 lookup（延迟） |
| 回收风暴 | slab shrinker 被大量调用，`count_objects/scan_objects` 频繁执行 |
| 内核态内存压力 | slab 不可直接 free，只能靠 shrinker 有选择地收 |

---

## 6. 衔接

- 上节 [§3 LRU 链表管理](./section-3-LRU-链表管理.md)：LRU 链的回收
- [§5 换出进程页面](./section-5-换出进程页面.md)：匿名页 swap out
- 前置：[Ch 8 Slab 分配器](../../chapter-08-slab-allocator/)（slab shrinker 的注册方）

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：为什么 slab/dcache 不走 LRU，而要单独搞 shrinker？**
A：LRU 是给「用户可见的页」设计的（按访问热度排队）。slab 对象、dentry 没有「访问热度」的简单概念，也不按页组织——它们按各自的数据结构管理。所以用**回调**让各子系统自己决定「收什么、收多少」，而不是内核统一排队。

**Q2：`count_objects` 和 `scan_objects` 为什么要分开？**
A：`count_objects` 是**廉价计数**（回答「能收多少」），`scan_objects` 是**昂贵回收**（真的去删）。回收器先问 count 决定「要不要让你收、让你收多少」，再调 scan 执行。分开后，count 可以被频繁调用（决策用），scan 只在真正需要时调。

**Q3：`seeks` 字段为什么用「seek 次数」而不是「字节数」度量？**
A：因为重建一个对象的代价，**主要不在 CPU/内存，而在磁盘 I/O**。dentry 重建 = 一次路径 lookup（可能 1 次磁盘 seek），某些 slab 对象重建 = 多次随机读（16 次 seek）。用 seek 次数能直接反映「踢掉它，之后要用它时有多痛」。

**Q4：`nr_deferred` 是干嘛的？**
A：**延迟删除**。有些对象回收要等 I/O 完成（如 inode 正在 writeback），不能立刻删。这些对象被记进 per-node 的 `nr_deferred`，等条件满足后再删。它让 shrinker 不必在回收路径上阻塞等 I/O。

**Q5：为什么说回收 dcache 是「级联腾内存」？**
A：dentry 本身才几百字节，但它引用 inode，inode 引用 page cache 页。删一个 dentry 可能触发 inode 释放，进而让关联的**一整片 page cache 页**（每页 4KiB）回落 Buddy。所以账面上「收了几百字节」，实际可能腾出几十 KB 到几 MB。

</details>

---
