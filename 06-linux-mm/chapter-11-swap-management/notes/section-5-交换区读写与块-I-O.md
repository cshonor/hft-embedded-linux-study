# Ch 11 §5 交换区读写与块 I/O

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**
> 源码核验：Linux **v6.6**（`mm/page_io.c` / `mm/swapfile.c`）

---

## 本节讲什么

本节回答：**swap 的读（换入）和写（换出）到底走什么 I/O 路径？**

原书给的统一入口是 `rw_swap_page()`。v6.6 里它**已改名为 `swap_readpage` / `swap_writepage`**，并且写路径按介质**分裂成三条**（文件系统 / 块设备同步 / 块设备异步）。本节沿 `__swap_writepage` 源码走一遍。

---

## 1. 原书 → v6.6 的函数名映射

| 原书（2.6） | v6.6 | 说明 |
|------------|------|------|
| `rw_swap_page()` | `swap_readpage()` / `swap_writepage()` | 拆成读写两个独立入口 |
| （块层直接 bio） | `__swap_writepage()` 三分支 | 按 `SWP_FS_OPS`/`SWP_SYNCHRONOUS_IO` 分派 |

---

## 2. 写路径：`__swap_writepage` 三分支（`mm/page_io.c:371`）

```c
void __swap_writepage(struct page *page, struct writeback_control *wbc)
{
    struct swap_info_struct *sis = page_swap_info(page);

    VM_BUG_ON_PAGE(!PageSwapCache(page), page);   /* 必须已在 swap cache */

    if (data_race(sis->flags & SWP_FS_OPS))                 /* :381 文件 swap */
        swap_writepage_fs(page, wbc);
    else if (sis->flags & SWP_SYNCHRONOUS_IO)               /* :383 zram 等 */
        swap_writepage_bdev_sync(page, wbc, sis);
    else                                                    /* :385 普通块设备 */
        swap_writepage_bdev_async(page, wbc, sis);
}
```

| 分支 | 触发条件（`SWP_*` 标志） | 场景 | 特点 |
|------|------------------------|------|------|
| `swap_writepage_fs` | `SWP_FS_OPS` | **文件 swap** | 走文件系统 `a_ops->swap_rw`（如 btrfs） |
| `swap_writepage_bdev_sync` | `SWP_SYNCHRONOUS_IO` | **zram**（内存压缩块设备） | `submit_bio_wait` 同步提交，省去异步开销 |
| `swap_writepage_bdev_async` | 其他 | **普通块设备/分区** | 异步 `submit_bio` + 回调 `end_swap_bio_write` |

**关键直觉**：写路径的**介质适配**全靠 `swap_info_struct->flags`（§1 的 `SWP_*`）。zram 这类"同步反而快"的设备（无真实磁盘延迟，异步排队纯属多余）走 sync 分支；普通磁盘走 async 分支用写回队列攒批。

---

## 3. 读路径：`swap_readpage`（`mm/page_io.c:493`）

```c
void swap_readpage(struct page *page, bool synchronous, struct swap_iocb **plug)
{
    ...
    if (synchronous || (sis->flags & SWP_SYNCHRONOUS_IO)) {
        /* 同步读：submit_bio_wait 直接等待完成 */
    } else {
        /* 异步读：submit_bio + 回调，期间可能 plug 攒批 */
    }
}
```

- **同步读**：缺页 fault 上下文里，进程本来就在等这个页，直接同步读更简单。
- **异步读**：预读（readahead）场景，一次提交多个 bio，用 `plug` 攒批减少调度。

---

## 4. slot → 磁盘 sector：`swap_page_sector`（`swapfile.c:230`）

无论读写，都要把"slot 号"翻译成"磁盘扇区号"。这靠 §7 的 `swap_extent`：

```c
sector_t swap_page_sector(struct page *page)
{
    struct swap_info_struct *sis = page_swap_info(page);
    struct swap_extent *se;
    pgoff_t offset;

    offset = __page_file_index(page);                    /* slot 号 */
    se = offset_to_swap_extent(sis, offset);             /* 红黑树查 extent */
    sector = se->start_block + (offset - se->start_page); /* extent 内偏移 */
    return sector << (PAGE_SHIFT - 9);                   /* 页号 → 512B 扇区号 */
}
```

```
swap slot (页号) ── offset_to_swap_extent ──► swap_extent
     offset                                     { start_page, nr_pages, start_block }
        └─ sector = start_block + (offset - start_page) ──► 磁盘扇区
```

这就是为什么文件 swap 需要 extent：**文件在磁盘上块不连续**，slot 号（连续的逻辑页号）不能直接线性映射到扇区，必须查 extent 表（§7）。

---

## 5. 换出/换入的完整 I/O 时序

```
换出 (swap out):
  kswapd/直接回收选中 folio
    → add_to_swap_cache（§4）
    → __swap_writepage
        ├─ swap_page_sector：slot → sector（查 swap_extent）
        └─ 三选一路径提交 bio
    → bio 完成回调 end_swap_bio_write → folio 标记 clean
    → rmap 更新 PTE → swap entry → 释放 folio

换入 (swap in):
  fault → __read_swap_cache_async（§4，先查 cache）
    → cache 未命中 → swap_readpage
        ├─ swap_page_sector：slot → sector
        └─ 读盘 → folio 标记 uptodate
    → 更新 PTE present → 进程继续
```

---

## 6. HFT / 嵌入式关联

| 场景 | 关联 |
|------|------|
| **`pswpin`/`pswpout` 监控** | `count_swpout_vm_event` 累计写回次数，对应 vmstat 的 `pswpin`/`pswpout`——**这两个数非零就是 swap 活动的铁证**，HFT 必须归零 |
| **zram 的 sync 路径** | 理解 `SWP_SYNCHRONOUS_IO` 才能明白：zram 做 swap 时**没有磁盘延迟**，延迟来自压缩/解压 CPU 开销，不是 I/O 排队 |
| **swap in fault 的代价链** | 换入 = 磁盘延迟 + bio 提交 + 页表更新 + TLB miss，是**毫秒级**尖刺——`pgmajfault` 上升要立即查 |

---

## 7. 衔接

- 下节 [§6 激活与停用交换区](./section-6-激活与停用交换区.md)：swapon/swapoff 怎么搭起/拆掉这套 I/O 通道
- slot→sector 的 extent：[§7 swap_extent](./section-7-2.6-内核的新变化：swap_extent.md)

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：`__swap_writepage` 的三条分支分别对应什么场景？判断依据是什么？**
A：依据是 `swap_info_struct->flags`：① `SWP_FS_OPS` → `swap_writepage_fs`（文件 swap，走文件系统 `swap_rw`）；② `SWP_SYNCHRONOUS_IO` → `swap_writepage_bdev_sync`（zram 等同步 I/O 更快的设备）；③ 其他 → `swap_writepage_bdev_async`（普通块设备，异步 bio）。这是"按介质特性选 I/O 策略"的典型分派。

**Q2：zram 为什么走 `swap_writepage_bdev_sync` 而不是 async？**
A：zram 是**内存压缩块设备**，没有真实磁盘的 seek/旋转延迟，异步 bio 排队 + 中断回调的**开销反而大于收益**。`SWP_SYNCHRONOUS_IO` 标记让写路径用 `submit_bio_wait` 同步提交，省掉异步机制。真正的开销在压缩/解压的 CPU 时间，不在 I/O 排队。

**Q3：`swap_page_sector` 为什么不能简单用 `offset * SECTORS_PER_PAGE` 算扇区？**
A：因为**文件 swap 在磁盘上块不连续**。slot 号是连续的逻辑页号，但文件的数据块散布在磁盘各处，必须通过 `swap_extent` 红黑树查到 `{start_page, start_block}`，再算 `start_block + (offset - start_page)`。只有**分区 swap**（块连续）才是简单的线性映射。

**Q4：原书的 `rw_swap_page()` 在 v6.6 变成了什么？**
A：拆成了 `swap_readpage()`（读/换入）和 `swap_writepage()`（写/换出）两个独立入口，写路径进一步由 `__swap_writepage` 按 `SWP_*` 标志三分支。函数粒度更细、介质适配更明确，但"统一走块 I/O 栈"的本质没变。

**Q5：`pswpin`/`pswpout` 和 `pgmajfault` 分别衡量什么？HFT 里怎么看？**
A：`pswpin`/`pswpout` 是**页级** swap 读写计数（`count_swpout_vm_event` 累计），非零说明有数据进出 swap；`pgmajfault` 是**major fault** 计数（缺页需读盘）。HFT 里三者都应**归零**——任何非零都意味着关键路径上出现了磁盘级延迟，要立即排查（通常是没 mlock 或内存配置不当）。

</details>
