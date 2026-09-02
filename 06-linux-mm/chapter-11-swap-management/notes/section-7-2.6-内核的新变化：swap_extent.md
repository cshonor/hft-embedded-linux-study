# Ch 11 §7 2.6 内核的新变化 → v6.6：`swap_extent`

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**
> 源码核验：Linux **v6.6**（`include/linux/swap.h` / `mm/swapfile.c`）

---

## 本节讲什么

本节收尾：原书 2.6 引入的 **`swap_extent`**（文件 swap 的块映射），在 v6.6 里进化成了什么？

答案是：**一棵红黑树 + 合并逻辑 + 文件系统 hook**。它解决的核心问题不变——"文件 swap 的磁盘块不连续，slot 号不能线性映射到扇区"——但实现从"简单列表"升级成"有序红黑树 + 相邻合并"。

---

## 1. 问题：文件 swap 的块不连续

```
分区 swap（/dev/sda2）：块天然连续
    slot 0 → block 0，slot 1 → block 1，...（线性映射）

文件 swap（/swapfile）：块散布在文件系统各处
    slot 0 → block 1024，slot 1 → block 4096，slot 2 → block 4097，...
    可能一段连续、一段跳跃，无法线性算扇区
```

原书 2.6 的答案是 `swap_extent`——记录"**连续 slot 范围 ↔ 连续磁盘块范围**"的映射。v6.6 里这个结构**保留并升级**。

---

## 2. `struct swap_extent`（`swap.h:193`）

```c
/* include/linux/swap.h:193 */
struct swap_extent {
    struct rb_node rb_node;   /* 红黑树节点（按 start_page 排序） */
    pgoff_t   start_page;     /* 起始 slot 号（逻辑页号） */
    pgoff_t   nr_pages;       /* 覆盖的页数 */
    sector_t  start_block;    /* 起始磁盘块号（sector） */
};
```

| 字段 | 作用 |
|------|------|
| `start_page` | 这段连续映射的**起始 slot 号** |
| `nr_pages` | 覆盖几个页（一个 extent 通常 1–4MB，注释 `:2247`） |
| `start_block` | 对应的**起始磁盘扇区号** |
| `rb_node` | 红黑树节点，按 `start_page` 排序 |

**映射公式**：`sector = start_block + (offset - start_page)`，`offset` 是要访问的 slot 号（§5 `swap_page_sector`）。

---

## 3. 红黑树组织 + 相邻合并（`swapfile.c:2184`）

```c
int add_swap_extent(struct swap_info_struct *sis, unsigned long start_page,
                    unsigned long nr_pages, sector_t start_block)
{
    /* 按升序插入红黑树最右（调用方按 page 升序遍历） */
    ...
    if (parent) {
        se = rb_entry(parent, struct swap_extent, rb_node);
        BUG_ON(se->start_page + se->nr_pages != start_page);  /* 必须连续 */
        if (se->start_block + se->nr_pages == start_block) {
            se->nr_pages += nr_pages;    /* 磁盘也连续 → 合并！ */
            return 0;
        }
    }
    /* 不连续 → 新建一个 extent 节点 */
    new_se = kmalloc(sizeof(*se), GFP_KERNEL);
    ...
    rb_insert_color(&new_se->rb_node, &sis->swap_extent_root);
}
```

**关键设计**：两个相邻的 extent，如果**磁盘块也连续**（`start_block + nr_pages == 新 start_block`），就**合并成一个**——减少 extent 数量，加快查找。这是文件系统 extent（如 ext4 的 extent 树）的同款思想。

查找走 `offset_to_swap_extent`（标准红黑树查找）：

```c
while (rb) {
    se = rb_entry(rb, struct swap_extent, rb_node);
    if (offset < se->start_page)
        rb = rb->rb_left;
    else if (offset >= se->start_page + se->nr_pages)
        rb = rb->rb_right;
    else
        return se;                       /* 命中 */
}
BUG();                                   /* 必须命中，否则是 bug */
```

---

## 4. 块设备 vs 文件：统一成 extent

`setup_swap_extents`（`:2251`）的注释（`:2224-2246`）是整个设计的精髓：

| swap 类型 | extent 构建方式 |
|-----------|----------------|
| **块设备（S_ISBLK）** | **单个 extent**：`add_swap_extent(sis, 0, sis->max, 0)`——块连续，从头到尾一条 |
| **普通文件（S_ISREG）** | walk 文件所有块，解析成红黑树（PAGE_SIZE 块）；不对齐的杂块**直接丢弃**（`:2240`） |

**为什么块设备也套 extent？** 注释 `:2230-2231`：让 `S_ISBLK` 和 `S_ISREG` **在 swapon 之后被完全相同地处理**——主操作代码（`swap_page_sector`）不需要区分"块设备还是文件"，统一查 extent 红黑树即可。

另外，swapon 期间会给 inode 打 `S_SWAPFILE` 标记（`:2244`），**阻止用户写 swap 设备**（那会毁掉内存数据）。

---

## 5. v6.6 演进：文件系统 hook

原书 2.6 的 `swap_extent` 靠"遍历文件块"构建；v6.6 进一步把构建交给文件系统：

```c
/* setup_swap_extents :2264 */
if (mapping->a_ops->swap_activate) {
    ret = mapping->a_ops->swap_activate(sis, swap_file, span);  /* fs 自己建 extent */
    sis->flags |= SWP_ACTIVATED;
    if ((sis->flags & SWP_FS_OPS) && sio_pool_init() != 0) {   /* 分配 swap I/O pool */
        destroy_swap_extents(sis);
        return -ENOMEM;
    }
}
```

| 演进点 | 说明 |
|--------|------|
| **`swap_activate` hook** | 文件系统（如 btrfs/xfs）**自己**负责构建 extent——它们更懂自己的块布局，能预分配、能优化 |
| **`SWP_FS_OPS`** | 标记"走文件系统 I/O"，触发 §5 的 `swap_writepage_fs` 路径 |
| **`sio_pool_init`** | 为文件 swap 预分配 swap I/O 请求池（`swap_iocb`），避免 I/O 路径上临时分配 |

**直觉**：extent 从"内核通用代码遍历块"演进成"文件系统自己负责"——这是**职责下沉**：谁最懂块布局，谁建 extent。

---

## 6. Swap 全链路简图（收尾）

```
匿名页 (进程私有)
    │
    ├─ 内存充足 → 常驻 RAM
    │
    └─ Ch 10 回收选中（kswapd / direct reclaim）
           ├─ alloc swap slot：get_swap_pages → scan_swap_map_slots（§3）
           │     （HDD 顺序簇 / SSD per-CPU 簇）
           ├─ add_to_swap_cache → swap cache（§4）
           ├─ __swap_writepage（§5，三路径）
           │     └─ swap_page_sector → 查 swap_extent 红黑树（§7）
           ├─ PTE := swp_entry(type, offset)（§2）
           └─ free physical page → 回 Buddy

再次访问 VA
    → page fault（PTE !present + swap entry）
    → __read_swap_cache_async（§4，cache 命中则不读盘）
    → swap_readpage（§5）→ 新页 → PTE present
    → 用户指令重试（毫秒级延迟）
```

---

## 7. HFT 精读 checklist

| 手段 | 目的 |
|------|------|
| **`mlock` / `mlockall(MCL_CURRENT\|MCL_FUTURE)`** | 匿名 RSS **不被换出**（唯一根治手段） |
| **足够 RAM + 监控** | `si/so`（vmstat）、`pswpin/pswpout`、`pgmajfault` |
| **`vm.swappiness=1` 或 0** | 降低匿名页被 swap 倾向（**不替代 mlock**） |
| **不用 swap 作「额外内存」** | swap 是**回收手段**，不是 HFT 堆扩展 |
| **避免 swapoff 在线** | `try_to_unuse` 全表扫描 + 强制换入风暴 |
| **若必须用 swap 文件** | 预分配（`fallocate`）+ 快设备（NVMe），别用稀疏文件 |

**与 Ch 10 闭合**：kswapd/direct reclaim 选中 victim → **本章**完成 slot + swap cache + I/O + PTE 编码 → fault 路径读回。整个匿名页换出/换入闭环在此合拢。

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：`swap_extent` 解决的核心问题是什么？为什么分区 swap 不需要它？**
A：核心问题是**文件 swap 的磁盘块不连续**——slot 号是连续逻辑页号，但文件数据块散布各处，无法线性算扇区。分区 swap 的块天然连续，本来一个 extent（`add_swap_extent(sis, 0, sis->max, 0)`）就够；但为了统一处理，块设备也套 extent，让 `swap_page_sector` 不用区分块设备还是文件。

**Q2：`add_swap_extent` 里的合并逻辑在什么条件下触发？为什么合并？**
A：当新 extent 的 `start_page` 紧接上一个 extent 末尾、且**磁盘块也连续**（`start_block + nr_pages == 新 start_block`）时，就把 `nr_pages` 累加到前一个节点上（`:2203-2207`）。合并能减少红黑树节点数（extent 数量少 → 查找快、内存省），是文件系统 extent 的标准优化。

**Q3：`offset_to_swap_extent` 是标准的红黑树查找，它的比较条件是什么？**
A：`offset < start_page` 走左子树，`offset >= start_page + nr_pages` 走右子树，否则命中返回。这是"区间树"查找：每个节点不是单点而是 `[start_page, start_page+nr_pages)` 区间。最后 `BUG()` 表示"必然命中"，若没找到说明 swap_map/extent 数据不一致，是内核 bug。

**Q4：v6.6 里 extent 的构建为什么下沉给文件系统（`swap_activate`）？**
A：因为**文件系统最懂自己的块布局**。通用代码只能盲目遍历文件块；文件系统的 `swap_activate` hook 能做预分配、锁定、优化 extent 布局。这是"谁最懂谁负责"的职责下沉，也让 `SWP_FS_OPS` 能配套触发文件系统的 swap I/O 路径（§5 `swap_writepage_fs`）。

**Q5：`S_SWAPFILE` 标记的作用是什么？**
A：swapon 期间给 swap 文件/设备的 inode 打 `S_SWAPFILE` 标记（注释 `:2244`），**阻止用户进程写这个设备**。因为 swap 设备的内容是内存的换出镜像，用户误写会**直接毁掉内存数据**（换入时读到脏数据）。这是 swap 生命周期内的一层保护。

</details>
