# Ch 11 §4 交换缓存 (Swap Cache) — **核心**

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**
> 源码核验：Linux **v6.6**（`mm/swap_state.c` / `include/linux/swap.h`）

---

## 本节讲什么

本节回答：**换出/换入进行中的那个"中间态"，页放在哪、由谁管？**

答案是 **swap cache**——页缓存的特殊形式，`address_space` 为 `swapper_spaces[]`。原书讲清了它的角色（防更新丢失），v6.6 里它变成了 **folio + xarray + 每 swap type 一个 address_space**。本节落到源码。

---

## 1. 为什么需要 swap cache？

**换出**是个多步过程：分配 slot → 写盘 → 更新 PTE → 释放物理页。这中间存在**竞态窗口**：

```
换出中：
  T1（kswapd）: 分配 slot → 准备写盘
  T2（进程）:   访问这个页 → 想修改它
  ── 若 T2 直接改，T1 写盘时可能写到「改了一半」的数据 → 更新丢失 ──
```

**swap cache 的作用**：换出期间把页"锁定"在 swap cache 里，其他访问者发现它在 cache 就**等待写盘完成**或**复用同一页**，而不是各自为政。

换入也有对称问题：**多个进程共享同一匿名页**，换出后它们各自 fault 同一个 swap entry。若不缓存，每个进程都会**各自读一次盘**——swap cache 让它们**只读一次**，之后共享。

---

## 2. 本质：`swapper_spaces[]` 的页缓存

```c
/* mm/swap_state.c:39 */
struct address_space *swapper_spaces[MAX_SWAPFILES] __read_mostly;
```

| 对比 | 普通页缓存 | swap cache |
|------|-----------|------------|
| `address_space` | 文件的 `inode->i_mapping` | **`swapper_spaces[type]`**（每个 swap type 一个） |
| 索引键 | 文件偏移（`index`） | **swap entry 的 `offset`** |
| 页来源 | 读文件 | 换出的匿名页 |
| 标志 | `PG_uptodate` 等 | **`SwapCache`** 标志 + `folio->swap` |

**关键演进**：原书是**单一** `swapper_space`（全局一个）；v6.6 拆成 `swapper_spaces[MAX_SWAPFILES]`——**每个 swap 区一个 `address_space`**（`swap_state.c:39` 注释：`swapper_space` 是"虚构的，为了简化路径保留"）。好处是并发隔离 + NUMA 局部性。

---

## 3. `add_to_swap_cache`（`swap_state.c:86`）

```c
int add_to_swap_cache(struct folio *folio, swp_entry_t entry, gfp_t gfp, void **shadowp)
{
    struct address_space *address_space = swap_address_space(entry);  /* :89 */
    pgoff_t idx = swp_offset(entry);              /* :90 索引用 offset */
    XA_STATE_ORDER(xas, &address_space->i_pages, idx, folio_order(folio));
    ...
    folio_ref_add(folio, nr);                     /* :101 增加引用 */
    folio_set_swapcache(folio);                   /* :102 打 SwapCache 标记 */
    folio->swap = entry;                          /* :103 页记录自己的去向 */

    do {
        xas_lock_irq(&xas);
        ...
        xas_store(&xas, folio);                   /* :117 存入 xarray（i_pages） */
        address_space->nrpages += nr;             /* :120 */
        __lruvec_stat_mod_folio(folio, NR_SWAPCACHE, nr);  /* :122 统计 */
    } while (xas_nomem(&xas, gfp));
    ...
}
```

| 要点 | 说明 |
|------|------|
| **索引 = `swp_offset(entry)`** | swap cache 的"页偏移"就是 slot 编号，和文件页缓存的"文件偏移"对应 |
| **xarray 存储** | 现代页缓存用 xarray（`i_pages`）取代旧哈希表，swap cache 同款 |
| **`folio->swap = entry`** | 页自己记住"我在哪个 slot"（§2.5），供后续反查 |
| **`NR_SWAPCACHE`** | 专门的 vmstat 计数，`/proc/meminfo` 的 `SwapCached` 来源 |

---

## 4. 换出/换入中的 cache 生命周期

```
换出 (swap out):
  alloc slot → swp_entry(type, offset)
  add_to_swap_cache(folio, entry)     ← 进 cache，SWAP_HAS_CACHE 置位
  swap_writepage()                    ← 写盘（期间页被 cache 锁定）
  写盘完成 → 更新所有 PTE → swap entry
  引用计数归零 → folio 释放（离开 cache）

换入 (swap in):
  fault → __read_swap_cache_async(entry)
      ├─ cache 命中 → 复用已有 folio（不读盘！）
      └─ cache 未命中 → 真正 swap_readpage 读盘 → add_to_swap_cache
  返回 folio → 更新 PTE present
```

**`SWAP_HAS_CACHE` 位**（`swap.h:229`，0x40）：`swap_map[offset]` 里的一个 bit，标记"这个 slot 有页在 swap cache 里"。`scan_swap_map_slots` 分配新 slot 时看到这个位就知道"slot 虽占着，但可复用 cache-only 的页"（§3 `vm_swap_full()` 分支）。

---

## 5. 共享页的去重：只读一次盘

**这是 swap cache 最精巧的作用**——换入去重：

```
进程 A、B 共享匿名页 P，P 被换出（PTE_A、PTE_B 都变成 swap entry）
    A 访问 → fault → __read_swap_cache_async
         cache 未命中 → 读盘 → folio 进 cache
         A 的 PTE_A 改回 present + folio
    B 访问 → fault → __read_swap_cache_async
         cache 命中！→ 直接复用同一个 folio，不读盘
         B 的 PTE_B 改回 present + 同一 folio
```

**没有 swap cache** 的话，A、B 会各自读一次盘、各自拿一个物理页——浪费 I/O 又破坏共享。有了 cache，**一个 slot 同一时刻最多读一次盘**。

---

## 6. HFT / 嵌入式关联

| 场景 | 关联 |
|------|------|
| **"中间态锁定"思想** | swap cache 在换出期间锁定页、防更新丢失——HFT 里"订单状态机"的中间态（pending → filled）也要防并发改写的同样问题 |
| **换入去重** | 多消费者共享同一数据时"只取一次、缓存共享"——HFT 里多策略进程共享行情快照的广播去重同构 |
| **`SwapCached` 监控** | `/proc/meminfo` 的 `SwapCached` 反映"换出过但还留在内存"的页量——它高说明换出数据又被频繁访问，是内存压力信号 |

---

## 7. 衔接

- 下节 [§5 交换区读写与块 I/O](./section-5-交换区读写与块-I-O.md)：`swap_writepage`/`swap_readpage` 怎么落盘
- 页缓存本体：[Ch10 §2 页缓存](../../chapter-10-page-frame-reclamation/notes/section-2-页缓存.md)

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：swap cache 和普通页缓存最本质的相同点和不同点是什么？**
A：相同点：都是 `address_space` + xarray（`i_pages`）组织的页集合，共用页缓存的查找/锁定/写回框架。不同点：① `address_space` 是 `swapper_spaces[type]`（每个 swap 区一个），不是文件的 `inode->i_mapping`；② 索引键是 **swap entry 的 offset**，不是文件偏移；③ 页来自换出的匿名页，不是读文件。

**Q2：v6.6 为什么把原书的单一 `swapper_space` 拆成 `swapper_spaces[MAX_SWAPFILES]`？**
A：原书全局一个 `swapper_space`，所有 swap 区的 cache 都挤在一个 `address_space` 里——并发访问争同一把锁，且没有 NUMA 局部性。拆成每 swap type 一个后，不同区的换入/换出**互不争锁**，也便于按节点布局（`swap_state.c:39` 注释说 `swapper_space` 只是"为简化路径保留的虚构"）。

**Q3：`add_to_swap_cache` 里 `folio->swap = entry` 这行为什么关键？**
A：它让**页自己记住"我被换到哪了"**。后续 `delete_from_swap_cache`（释放 cache 时）、`try_to_unuse`（swapoff 反查时）、`page_swap_entry`（§2.5）都靠这个字段直接拿到 swap entry，不必每次反解 PTE。这是"页 → 磁盘位置"的权威记录。

**Q4：swap cache 怎么实现"共享页只读一次盘"？**
A：换入路径走 `__read_swap_cache_async`（`swap_state.c:412`），它**先查 swap cache**：命中就直接复用已有 folio（不读盘）；未命中才真正 `swap_readpage` 读盘并 `add_to_swap_cache`。这样多个进程 fault 同一 swap entry 时，第一个读盘、其余命中 cache，实现去重。

**Q5：`SWAP_HAS_CACHE` 位和 swap cache 是什么关系？**
A：`SWAP_HAS_CACHE`（`swap.h:229`，0x40）是 `swap_map[offset]` 里的一个 bit，标记"这个 slot 有页在 swap cache 里"。它让 slot 分配器（§3）知道：即使 slot 引用计数非零，也可能只是 cache-only（写盘后已无 PTE 引用），在内存紧张时可以被 `__try_to_reclaim_swap` 回收复用。

</details>
