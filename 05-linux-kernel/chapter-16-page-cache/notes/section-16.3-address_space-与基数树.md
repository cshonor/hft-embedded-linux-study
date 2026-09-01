## ③ address_space 与基数树

页缓存 **通用** — 不只缓存「文件」，还缓存任何 **基于页的对象**（含部分 mmap 路径）。

| 结构 | 角色 |
|------|------|
| **`address_space`** | 管理 **缓存条目 + 页 I/O** — 可视为 VMA 的 **「物理页侧」对应物** |
| 关联 | 常挂在 **inode** 上（`inode->i_mapping`） |

#### 版本断崖：radix tree 已被 **xarray** 取代（v4.20）

```c
/* include/linux/fs.h — v6.6 struct address_space（节选，逐字段注释） */
struct address_space {
	struct inode		*host;          /* 宿主 inode（swap 时可能指别人） */
	struct xarray		i_pages;       /* ← 2.6 时代是 radix tree，现在是 xarray */
	struct rw_semaphore	invalidate_lock; /* 截断 vs 缺页的互斥门 */
	gfp_t			gfp_mask;       /* 该文件页的分配掩码（如 GFP_NOFS） */
	atomic_t		i_mmap_writable;/* 有多少个可写映射（>0 说明可能被用户态改脏） */
	struct rb_root_cached	i_mmap;         /* 映射了本文件的所有 VMA（红黑树） */
	unsigned long		nrpages;        /* 缓存页数 */
	pgoff_t			writeback_index;/* 上次写回到哪儿了（循环写回游标） */
	const struct address_space_operations *a_ops; /* 文件系统填的操作表 */
	errseq_t		wb_err;         /* 写回错误的"代"，fsync 靠它判断有没有新错误 */
	spinlock_t		private_lock;
	struct list_head	private_list;   /* 文件系统私有（如 ext4 的 io_end） */
	void			*private_data;
};
```

| 代际 | 结构 | 内核版本 | 淘汰原因 |
|------|------|---------|---------|
| 1 | **全局哈希表** | <2.4 | 一把全局锁，多文件并发读写时严重争用 |
| 2 | **radix tree** | 2.4 → v4.19 | 无全局锁、支持 tag，但 API 别扭（多槽位/预分配语义混乱） |
| 3 | **xarray** | **v4.20+（现役）** | 自带 `xa_lock`、API 干净、支持 multi-index 条目（为 THP/folio 铺路） |

**语义没变**：仍然是「文件偏移 → 内存页」的索引。变的只是实现和 API 名字。

---

#### 三个 tag 位：为什么 xarray 比"树 + 链表"更合适

xarray 给**每个条目**留了 3 个标记位，页缓存正好用来记状态：

```c
/* include/linux/fs.h — v6.6 */
#define PAGECACHE_TAG_DIRTY	XA_MARK_0   /* 这页脏了，需要写回 */
#define PAGECACHE_TAG_WRITEBACK	XA_MARK_1   /* 正在写回中（别碰） */
#define PAGECACHE_TAG_TOWRITE	XA_MARK_2   /* 本轮待写（防止活锁） */

static inline bool mapping_tagged(struct address_space *mapping, xa_mark_t tag)
{
	return xa_marked(&mapping->i_pages, tag);   /* O(1) 回答「有没有」 */
}
```

| 好处 | 说明 |
|------|------|
| **O(1) 判断"有没有脏页"** | `mapping_tagged(mapping, PAGECACHE_TAG_DIRTY)` —— 老设计要遍历 dirty 链表 |
| **状态跟着页走** | 页被回收、迁移、拆分时状态自动跟随，不用额外维护链表成员 |
| **批量扫描高效** | `xa_find_tagged()` 直接跳到下一个带 tag 的条目，跳过中间所有干净页 |

> 这解释了 16.1 的版本断崖：既然 tag 已经是"页的属性"，就没必要再在 `address_space` 里维护 `dirty_pages` 链表了——**链表被 tag 取代**，剩下的"哪些 inode 是脏的"才需要链表，于是挪到了 `bdi_writeback`。

---

#### struct page → struct folio（v5.16+，v6.6 页缓存已全面 folio 化）

LKD 通篇讲 `struct page`。现代页缓存代码里你看到的是 **folio**：

| | `struct page` | `struct folio` |
|---|---|---|
| 语义 | 一个 4KB 页 | **一组物理连续的页**（1 页 / 2 / 4 / … / THP） |
| 复合页处理 | 要判断 `PageHead`/`PageTail`，极易写错 | folio 天然就是"整体"，head/tail 细节被藏起来 |
| 页缓存 API | `find_get_page()` | `filemap_get_folio()` / `mapping_get_folios()` |
| 批量传递 | 无 | `struct folio_batch`（一次搬 15~31 个 folio） |

```c
/* mm/filemap.c:2617 — v6.6 现代读路径的骨架 */
ssize_t filemap_read(struct kiocb *iocb, struct iov_iter *iter, ssize_t already_read)
{
	struct folio_batch fbatch;          /* ← 批量容器，不是单个 page */
	...
	folio_batch_init(&fbatch);
	...
	error = filemap_get_pages(iocb, iter->count, &fbatch, false);
```

> **读代码时的实用技巧：** 看到 `folio` 先当成"一个页"读，不会错到哪去；只有碰到 `nr_pages`、THP、large folio 相关逻辑时才需要展开。

---

```
address_space
    ├── i_pages (xarray):  offset ──► folio *      [3 个 tag 位记脏/回写/待写]
    ├── i_mmap  (rbtree):  ──────────► VMA 们      [谁 mmap 了这个文件]
    └── a_ops:             ──────────► read_folio / writepages / dirty_folio ...
```

> **一个容易踩的点：** `i_mapping` 和 `i_data` 是**两个**字段。
> 普通文件两者相同（`i_mapping == &inode->i_data`）；但**块设备**走 `bdev->bd_inode->i_mapping`，
> 而 swap/某些特殊文件会让 `i_mapping` 指向别人的 `address_space`。写代码时一律用 `inode->i_mapping`，别直接用 `&inode->i_data`。

→ **Ch 15** VMA · **Ch 13** inode · [Ch 16.1 脏页住哪](./section-16.1-缓存策略与写回.md) · [Ch 6.4 映射（xarray 全景）](../../chapter-06-kernel-data-structures/notes/section-6.4-映射.md)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** address_space 和 page cache 的关系？

<details><summary>答案</summary>

address_space 是 page cache 的管理单位：每个 inode（文件）对应一个 address_space，其中包含该文件所有缓存页的基数树（radix tree / xarray）。查找文件 offset 对应的缓存页：address_space → xarray → page。mmap 文件时，VMA 的 vm_ops->fault 回调从 address_space 取页。

</details>

**Q2.** 内核怎么在不遍历整棵树的前提下，知道"这个文件有没有脏页"？

<details><summary>答案</summary>

靠 xarray 的 **tag 位**。每个页缓存条目带 3 个标记位（`include/linux/fs.h`）：

```c
#define PAGECACHE_TAG_DIRTY	XA_MARK_0   /* 脏 */
#define PAGECACHE_TAG_WRITEBACK	XA_MARK_1   /* 写回中 */
#define PAGECACHE_TAG_TOWRITE	XA_MARK_2   /* 本轮待写 */
```

判断有没有脏页：`mapping_tagged(mapping, PAGECACHE_TAG_DIRTY)` → 内部是 `xa_marked()`，xarray 在每一层节点上**汇总**了子树里是否有带该 tag 的条目，所以是 **O(1)**（准确说是 O(树高) 且常态第一步就返回）。

要**找出**所有脏页时，用 `xa_find_tagged()` 从上次位置继续，直接跳到下一个带 tag 的条目，中间的干净页全部跳过，不进缓存行。

这就是把"脏页链表"从 `address_space` 里拿掉的底气：链表的核心能力（有没有 / 下一个是谁）tag 都能提供，而且天然跟着页走，不会出现"页已经回收了但链表节点还在"这类不一致。

</details>

**Q3.** 读现代内核源码时到处是 `folio`，它和 `struct page` 是什么关系？为什么要引入？

<details><summary>答案</summary>

`struct folio` 表示**一组物理连续的页**（可以是 1 页，也可以是 THP 的 512 页）。设计动机是**消灭复合页的 head/tail 陷阱**：

- 旧代码里 `struct page` 可能是复合页的 head、tail、或普通单页，几乎每个操作前都要先判断，`PageHead()`/`PageTail()`/`compound_head()` 满天飞，写错就是内存破坏；
- folio 被定义为"所见即整体"——拿到 folio 就是拿到整组页，`folio->page` 才是第 0 页的 `struct page`。

对页缓存的意义：支持 **large folio**（一次缓存 64KB 甚至 2MB 的连续块），减少 xarray 条目数、减少缺页次数、提高预读和 DMA 的效率。

迁移对照（v6.6 常用 API）：
- `find_get_page()` → `filemap_get_folio()`
- `page_cache_release()` → `folio_put()`
- `SetPageDirty()` → `folio_mark_dirty()`
- 批量传递：`struct folio_batch`（默认 15 个条目，`mm/filemap.c` 读路径里用它一次搬一批）

阅读技巧：先按"一个 folio ≈ 一个页"理解，不影响把握算法；只有碰到 `nr_pages` 和 THP 相关分支时才需要展开成多页。

</details>

</details>
---
