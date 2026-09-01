## ③ 缓冲区与缓冲区头 · buffer_head（历史）

块读入内存 → 放在 **缓冲区（buffer）** 中。

| 结构 | 作用 |
|------|------|
| **`struct buffer_head`** | 描述 **内存缓冲区 ↔ 磁盘物理块** 映射 |

| 2.6 前问题 | |
|------------|--|
| **笨重** | 基本 I/O 容器 |
| **拆分大 I/O** | 强迫内核把大请求拆成 **多个细碎 buffer_head** |

→ 被 **bio** 取代（④）— 但**只在数据路径上**，详见下文

### `struct buffer_head` 字段解剖

```c
struct buffer_head {
	unsigned long b_state;			/* 状态位图（BH_* 16 个标志） */
	struct buffer_head *b_this_page;	/* 页内块循环链表  ← 核心拓扑 */
	union {
		struct page  *b_page;		/* 所属页 */
		struct folio *b_folio;		/* v5.16+ folio 化，两者共用存储 */
	};
	sector_t b_blocknr;			/* 起始块号（磁盘侧） */
	size_t   b_size;			/* 映射大小 */
	char    *b_data;			/* 页内数据指针 */
	struct block_device *b_bdev;
	bh_end_io_t *b_end_io;			/* I/O 完成回调 */
	void    *b_private;
	struct list_head b_assoc_buffers;	/* 关联映射（writeback 用） */
	struct address_space *b_assoc_map;
	atomic_t b_count;
	spinlock_t b_uptodate_lock;		/* 页内首 bh 持有，
						   串行化同页其他 bh 的 IO 完成 */
};
```

两个字段值得单独拎出来：

* **`b_this_page`** —— bh 的全部意义就在这个循环链表上。它把「一个页里的所有块」串起来，使 page cache 能表达「这一页里第 2 块是脏的、其余是最新的」这种**页内粒度**状态。
* **`b_uptodate_lock`** —— 只有页内第一个 bh 会用。同页多个块并发 IO 完成时，靠它串行化状态位更新，避免 4 个块同时 `set_bit(BH_Uptodate)` 打架。

### `BH_*` 状态位（挑重点）

| 位 | 含义 | 谁用 |
|---|---|---|
| `BH_Uptodate` | 含有效数据（与磁盘一致） | 读路径：读完后置位，读者检查 |
| `BH_Dirty` | 脏，需回写 | writeback 扫描该位 |
| `BH_Lock` | 锁定中，IO 在飞 | 避免同一块并发 IO |
| `BH_Mapped` | 已建立磁盘映射（b_blocknr 有效） | get_block 成功后置位 |
| `BH_New` | 映射是刚创建的（还没分配盘上块） | 延迟分配 |
| `BH_Async_Read/Write` | 异步 IO 在飞 | 完成回调判别 |
| `BH_Delay` | 尚未在盘上分配（延迟分配中） | ext4 delalloc |
| `BH_Unwritten` | 已分配但未写入（fallocate 后） | DIO 写需要转换 |
| `BH_Meta` / `BH_Prio` | 含元数据 / 需高优先级提交 | 元数据 IO 优先 |

### 为什么被 bio 取代：四条根因

| bh 的局限 | 具体后果 | bio 的做法 |
|---|---|---|
| **1 bh = 1 块**（固定 512B/4K） | 1MB 顺序 IO = 256 个 bh 串成链表 | 一个 `bio` + `bio_vec` 数组 |
| **无法表达任意长度** | 只能按块粒度描述 | `bi_iter`（bvec_iter）可遍历任意字节区间 |
| **无 scatter-gather** | 内存必须物理连续 | `bio_vec` 天然支持内存不连续 |
| **每块一个 struct（约 100B）+ 链表指针** | 大 IO 的元数据开销线性增长、缓存不友好 | 与 IO 大小无关的常数开销 |

一句话：**bh 是「块视角」的容器，bio 是「段视角」的容器**。SSD/NVMe 时代按段描述才吻合硬件（PRP/SGL 本身就是 scatter-gather 列表）。

### 澄清一个常见误解：bh 没有被淘汰，是分工

「被 bio 取代」这句话**只在数据路径上成立**。现代内核（v6.6）里两者并存，各管一段：

| 场景 | 用什么 | 为什么 |
|---|---|---|
| 文件**数据** IO（块大小 = 页、4K 对齐） | **bio**（经 iomap / mpage 提交） | 大块、连续、要 scatter-gather |
| 文件**元数据** IO（inode/bitmap/间接块/目录块） | **buffer_head**（`sb_bread()`） | 固定块大小，需要 bh 状态位跟踪每块 |
| **FS 块 < 页** 时的页内块状态 | **buffer_head** | page cache 只有页级脏位，表达不了「页内第 2 块脏」 |
| FS 块 = 页 且走 iomap | **完全不碰 bh** | 页状态 == 块状态，bh 纯属多余 |

```
块大小 1KB，页 4KB：
  page ──► bh[0] ⇄ bh[1] ⇄ bh[2] ⇄ bh[3]   （b_this_page 循环链表）
           每块独立持有 Uptodate / Dirty / Lock
           → 页级 dirty 没法表达「只写了第 2 块」

块大小 4KB，页 4KB：
  page ──► 页状态即块状态，bh 完全不参与数据路径
```

### iomap：去 bh 的第二代改造

v4.x 起内核新增 `fs/iomap/` 层，给文件系统一个统一的「查询映射 + 提交 IO」框架，目标是从数据路径里彻底抹掉 bh：

| 文件系统 | 状态 |
|---|---|
| XFS | 最早全量切换到 iomap |
| ext4 | **DIO 路径已切 iomap**；buffered 写在 v6.x 仍部分依赖 bh（延迟分配 `BH_Delay`、dioread_nolock 需要页内块跟踪） |

所以准确说法是：**bio 取代了 bh 作为 IO 容器的地位，iomap 正在取代 bh 作为映射层的地位，而 bh 目前仍是元数据 IO 的主力。**

**HFT 实操含义**：`mkfs.ext4 -b 1024`（-b 小块）看似给海量小文件省空间，实际代价是每 4K 页要挂 4 个 bh + 链表遍历。数据密集场景一律用 **`-b 4096`**（= 页大小），让数据路径彻底绕开 bh 这一层。



<details>
<summary>自测题（点击展开）</summary>

**Q1.** buffer_head 为什么被 bio 取代？

<details><summary>答案</summary>

buffer_head 是 2.4 时代的块 IO 表示，每个 buffer_head 对应一个块（512B/4KB），大 IO 需要链式多个 bh → 开销大、不灵活。bio（2.6+）用 bio_vec 数组表示任意大小的 IO，一个 bio 可以包含多个不连续的物理段（scatter-gather），更适合现代 DMA。bh 仍保留用于元数据 IO。

四条根因：① 1 bh 只能描述 1 个固定大小的块，1MB IO 要 256 个 bh 链起来；② 无法表达任意字节长度；③ 内存必须物理连续，无 scatter-gather；④ 每块一个约 100 字节的 struct，开销随 IO 大小线性增长。

</details>

**Q2.** 「buffer_head 已被 bio 完全取代」——这句话对吗？

<details><summary>答案</summary>

**不对，是分工而非取代。**

- **数据路径**：块大小 = 页时确实不碰 bh，bio 经 iomap / mpage 直接提交——这部分 bh 确实退场了（v4.x 起的 iomap 层正在把 ext4 的 buffered 写也迁走，XFS 早已全切）。
- **元数据路径**：bh 至今是主力。inode 表、块位图、间接块、目录块全靠 `sb_bread()` 读出来的 buffer_head，因为它需要逐块的 `BH_Uptodate/Dirty/Lock` 状态位。
- **块 < 页时的数据路径**：一页装多个 FS 块，page cache 只有页级脏位，表达不了「页内第 2 块是脏的」，必须挂 bh（`b_this_page` 循环链表）来跟踪页内块。

所以 bh 在 v6.6 里活得好好的，只是不再承担大块数据 IO 了。

</details>

**Q3.** 为什么把文件系统块设成小于页（如 `mkfs.ext4 -b 1024`）会拖慢数据 IO？

<details><summary>答案</summary>

块 1KB、页 4KB 时，一个 page cache 页容纳 4 个 FS 块。page cache 的脏/最新标记是**页级**的，无法表达「这一页里只有第 2 块脏」。于是内核必须给该页挂上 4 个 `buffer_head`，用 `b_this_page` 串成循环链表，逐块记录 `BH_Uptodate/Dirty/Lock`。

代价三层：① 每个 bh 约 100 字节 + 链表指针，内存开销；② 读写要先遍历链表找对的 bh，多一层间接；③ 页内多块并发 IO 完成时，还要用页内首 bh 的 `b_uptodate_lock` 串行化状态位更新。

块 = 页（4KB/4KB）时页状态即块状态，这三层全部消失。数据密集场景应统一用 `-b 4096`。

</details>

</details>
---
