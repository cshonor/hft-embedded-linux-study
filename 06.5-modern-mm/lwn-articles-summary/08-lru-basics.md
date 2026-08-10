# LRU 页回收基础

> **原文:** [LRU and reclaim](https://lwn.net/Articles/845171/) (LWN, 2021)
> **对标旧书:** ULK3 Ch17 / LKD3 Ch18 (页帧回收)

---

## 核心观点

本文回顾传统 LRU 页回收机制，作为理解 MGLRU 的基础。

### LRU 双链表设计

```
每个 zone 维护 5 个 LRU 链表:
  LRU_INACTIVE_ANON — 不活跃匿名页 (可 swap)
  LRU_ACTIVE_ANON   — 活跃匿名页
  LRU_INACTIVE_FILE — 不活跃文件页 (可丢弃)
  LRU_ACTIVE_FILE   — 活跃文件页
  LRU_UNEVICTABLE   — 不可回收 (mlock)

页在链表间的迁移:
  新分配 → INACTIVE
  第二次访问 → ACTIVE (晋升)
  长期未访问 → INACTIVE (降级)
  回收从 INACTIVE 尾部开始
```

### kswapd 回收流程

```c
// 源码路径: mm/vmscan.c
// kswapd 线程: 空闲页低于 watermark 时唤醒

// 1. 计算需要回收的页数
nr_to_reclaim = SWAP_CLUSTER_MAX (32 页);

// 2. 扫描 inactive 链表尾部
while (nr_reclaimed < nr_to_reclaim) {
    page = lru_to_unevictable(lruvec);  // 从尾部取
    if (page_is_file(page) && !page_dirty(page))
        __free_page(page);           // clean file page: 直接丢弃
    else if (page_is_file(page) && page_dirty(page))
        writepage(page);             // dirty file page: 写回后丢弃
    else  // anon page
        swap_page(page);             // 写入 swap 后丢弃
}
```

### active/inactive 平衡

```c
// 源码路径: mm/vmscan.c
// 通过 active/inactive 比率控制
// 当 inactive 过少时，从 active 降级一些页到 inactive

// /proc/sys/vm/active_ratio 相关参数
// 默认: inactive 占 anon 总量的 1/3, file 总量的 2/5
```

---

## 与旧书差异

| ULK3 / LKD3 讲的 | 现代实现 |
|-------------------|---------|
| per-zone LRU | per-node LRU + per-memcg LRU (5.x+) |
| `mmap_sem` | `mmap_lock` |
| 手动 LRU 操作 | LRU 自动管理 + MGLRU (6.1+) |

---

## HFT 关联

HFT 应禁用 swap + mlockall，使 kswapd 不接触交易相关页。但仍需注意：kswapd 扫描本身会持有锁，可能间接影响延迟。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么文件页和匿名页用不同的 LRU 链表？

> 回收代价不同：clean file page 可以直接丢弃（零成本），dirty file page 需要写回磁盘（I/O），anon page 需要 swap（I/O + 压缩）。分开管理让回收算法优先回收低成本页（clean file），保护高成本页（anon）。

**Q2:** kswapd 和 direct reclaim 的区别是什么？哪个对 HFT 影响更大？

> kswapd 是后台线程，在空闲页低于 watermark 时异步回收。direct reclaim 是分配路径上同步回收（当 kswapd 来不及或被禁用时）。direct reclaim 对 HFT 影响更大——它在 `alloc_pages()` 调用中同步执行，导致微秒到毫秒级停顿。HFT 应通过预留足够内存 + mlockall 避免 direct reclaim。

</details>
