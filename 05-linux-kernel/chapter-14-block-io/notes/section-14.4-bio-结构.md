## ④ bio 结构 · `struct bio`

2.6 引入 — **轻量级**、表示 **in-flight 块 I/O**。

| 特性 | 说明 |
|------|------|
| **`bio_vec` 数组** | 一个 I/O 可含 **多段内存** |
| **分散-聚集（scatter-gather）** | 内存 **不连续** · 磁盘上 **连续** |
| **高端内存** | 支持 HIGHMEM 映射 |
| **可分割** | RAID 等把 **一个大 bio** 分到多盘 |

```
一次读 4KB 文件块：
  bio
    └── bio_vec[0] → 页 A 中 2KB
    └── bio_vec[1] → 页 B 中 2KB     （内存散，磁盘连续）
```

### `struct bio` 字段解剖（v6.6，blk_types.h）

```c
struct bio {
	struct bio	   *bi_next;	/* 请求队列串联 */
	struct block_device *bi_bdev;
	blk_opf_t	    bi_opf;	/* 低位 REQ_OP_*，高位 req_flags */
	unsigned short	    bi_flags;	/* BIO_* 标志 */
	unsigned short	    bi_ioprio;	/* I/O 优先级（ioprio） */
	blk_status_t	    bi_status;	/* 完成状态（errno 的块层版本） */
	atomic_t	    __bi_remaining;	/* split 后剩余分片计数 */

	struct bvec_iter    bi_iter;	/* ★ 游标：这次 IO 走到哪了 */

	blk_qc_t	    bi_cookie;	/* 轮询（poll）用的句柄 */
	bio_end_io_t	   *bi_end_io;	/* 完成回调 */
	void		   *bi_private;
	/* CONFIG_BLK_CGROUP：bi_blkg / bi_issue / bi_iocost_cost */
	/* CONFIG_BLK_INLINE_ENCRYPTION：bi_crypt_context */
	/* CONFIG_BLK_DEV_INTEGRITY：bi_integrity */

	unsigned short	    bi_vcnt;	/* 当前已用 vec 数 */
	/* ↓ 以下（含 bi_max_vecs）bio_reset() 会保留 */
	unsigned short	    bi_max_vecs;/* 容量上限 */
	atomic_t	    __bi_cnt;	/* 引用计数（pin count） */
	struct bio_vec	   *bi_io_vec;	/* 真正的 vec 数组 */
	struct bio_set	   *bi_pool;	/* 来自哪个 slab 池 */
	struct bio_vec	    bi_inline_vecs[];	/* ★ 内联 vec，必须在最后 */
};
```

### `bi_iter` 是「游标」，不是「数据」——最容易被误解的一点

bio 同时携带两样东西：**数据在哪**（`bi_io_vec` 数组）和**走到哪了**（`bi_iter`）。

```c
struct bvec_iter {
	sector_t     bi_sector;		/* 设备地址，单位恒为 512 字节扇区 */
	unsigned int bi_size;		/* ★ 剩余未完成字节数（不是总量！） */
	unsigned int bi_idx;		/* 当前在 bi_io_vec 中的下标 */
	unsigned int bi_bvec_done;	/* 当前 bvec 已完成字节数 */
} __packed;
```

| 要点 | 说明 |
|---|---|
| `bi_size` 是**剩余量** | 块层每推进一段就递减它；归零即 IO 完成。很多人误以为是「本次 IO 总长度」 |
| **bio split 的本质** | 把一个 `bi_iter` 劈成两半 → 两个 bio 各自持有一段游标。底层 vec 数组可以共享，不需要拷贝数据 |
| `bi_sector` 单位是 **512 字节扇区** | 即使 4Kn 盘也一样。这呼应 14.2：LBA 寻址单位恒为 512，`logical_block_size` 只是决定了**读写的最小粒度** |

```
原始 bio:  bi_iter = { sector=1000, size=128K }
              │
              │ blk_queue_split（超过 max_sectors 或撞上 RAID 条带边界）
              ▼
   bio A: { sector=1000, size=64K }  ─┐
   bio B: { sector=1128, size=64K }  ─┘ 各自独立下发、独立完成
```

### 内联 vec：给小 IO 免掉一次分配

结构体末尾的柔性数组 `bi_inline_vecs[]`，源码注释写得很直白：

```
 * We can inline a number of vecs at the end of the bio, to avoid
 * double allocations for a small number of bio_vecs. This member
 * MUST obviously be kept at the very end of the bio.
```

段数少（典型 1~4 段）时，`bio` 本体和它的 vec 数组在**同一次 slab 分配**里一起拿到 → 省掉第二次分配，也省掉一次指针解引用。这是典型的「为常见情形优化分配路径」手法，与 13.6 里 fdtable 内嵌 64 个 fd 槽是同一个思路。

段数超过内联容量时，`bi_io_vec` 才指向独立分配的数组。

### `bi_opf`：操作码爆炸（版本断崖）

LKD 时代的 `bi_rw` 只有读/写加几个标志位。v6.6 里 `REQ_OP_LAST = 36`：

| 类别 | 操作码 |
|---|---|
| 基础 | `REQ_OP_READ` / `WRITE` / `FLUSH` |
| 空间回收 | `DISCARD` / `SECURE_ERASE` / `WRITE_ZEROES` |
| **ZBD 分区盘** | `ZONE_OPEN` / `CLOSE` / `FINISH` / `APPEND` / `RESET` / `RESET_ALL` |
| 驱动私有 | `DRV_IN` / `DRV_OUT` |

`bi_opf` 一个字段塞两类信息：**低位 = 做什么**（上表），**高位 = 怎么做**（`req_flags`）：

| flag | 含义 | HFT 相关性 |
|---|---|---|
| `REQ_SYNC` | 同步 IO，暗示尽快下发 | 影响调度器插入位置 |
| `REQ_META` | 元数据 | 调度器/优先级判定 |
| `REQ_PRIO` | 高优先级 | mq-deadline 优先队列 |
| `REQ_FUA` | Force Unit Access，绕过盘上写缓存 | 持久化语义 |
| `REQ_PREFLUSH` | 提交前刷写缓存 | `fsync`/barrier |
| `REQ_IDLE` | 提交后空转等待，提升同向 IO 命中 | 吞吐优先，延迟有害 |
| **`REQ_NOWAIT`** | **不想阻塞就返回 `-EAGAIN`** | **io_uring / 无阻塞落盘的地基** |
| `REQ_INTEGRITY` | 带保护信息（T10 DIF） | 企业级盘 |

### bio 的完整生命周期

```
submit_bio(bio)                        ← 文件系统/页缓存调用
   │
   ├─ 校验（op 合法性、分区边界）
   ▼
submit_bio_noacct()                    ← 记账前的公共层：
   │                                      · bio split（超 max_sectors / seg 边界）
   │                                      · 分区偏移重映射（part->start_sect）
   │                                      · cgroup io 计费、blk-iocost
   │                                      · iostat / blktrace 打点
   ▼
blk_mq_submit_bio()                    ← 进入 blk-mq（见 14.5）：
   │                                      · 取 tag、选 hw queue / ctx
   │                                      · 有调度器 → 入队；无 → 直接下发
   ▼
驱动 q->mq_ops->queue_rq(hctx, rq)     ← NVMe/SCSI 真正下发到硬件
   │
   ▼
完成中断 → blk_mq_complete_request()
   → bio_endio(bio) → bi_end_io(bio)   ← 回调：唤醒等待者 / io_uring CQE
   → bio_put() → 归还 bi_pool slab
```

### 与 `buffer_head` 的对照（回看 14.3）

| 维度 | `buffer_head` | `bio` |
|---|---|---|
| 描述对象 | 一个**块** | 一次**IO** |
| 长度 | 固定（块大小） | 任意（`bi_iter.bi_size`） |
| 内存连续性 | 必须连续 | scatter-gather |
| 拆分 | 靠链表串多个 bh | 靠 split 劈 `bi_iter` |
| 进度跟踪 | 无（只有状态位） | `bi_iter` 游标 |
| 主战场 | **元数据** IO | **数据** IO |

→ **Ch 12** 页 · **Ch 16** 页缓存提交 bio · **Ch 14.5** bio 如何变成 request



<details>
<summary>自测题（点击展开）</summary>

**Q1.** bio_vec 的作用是什么？为什么需要 scatter-gather？

<details><summary>答案</summary>

bio_vec 描述一个物理段（page + offset + length）。一个 bio 包含多个 bio_vec，可以表示不连续物理内存的 IO。scatter-gather 让 DMA 一次传输多个物理段，减少 DMA 次数。NVMe 原生支持 PRP/SGL，一个命令传输 4MB 数据。这就是为什么 NVMe 比 SATA 快——不仅仅是带宽，更是 IO 效率。

（严格说，单次 NVMe 命令能传多少取决于 MDTS 控制器参数，4MB 只是常见上限量级；核心点是「一次命令多段」而非「一次一段」。）

</details>

**Q2.** `struct bio` 里 `bi_iter.bi_size` 是「本次 IO 的总长度」吗？bio split 到底劈的是什么？

<details><summary>答案</summary>

**不是总量，是剩余未完成量。** `struct bvec_iter`（bvec.h）四个字段：`bi_sector` 设备地址（单位恒为 512 字节扇区）、`bi_size` 剩余字节数、`bi_idx` 当前 vec 下标、`bi_bvec_done` 当前 vec 已完成字节数。块层每推进一段就递减 `bi_size`，归零即完成。

bio split 劈的正是这个**游标**，不是数据。超过 `max_sectors`、撞上 RAID 条带边界或分区边界时，内核把一个 `bi_iter` 切成两段，生成两个 bio 各自持有一段游标、独立下发独立完成——底层 `bi_io_vec` 数组可以共享，**不需要拷贝任何数据**。这也是为什么 split 虽然增加请求数但并不昂贵。

</details>

**Q3.** 想让落盘 IO「拿不到资源就立刻返回，绝不阻塞」，该用哪个标志？它的作用边界在哪？

<details><summary>答案</summary>

`REQ_NOWAIT`（`bi_opf` 高位 req_flags 之一）。置上它之后，块层在**可能阻塞**的地方（如 blk-mq 拿不到 tag、调度器队列满、需要等待 request 分配）会直接返回 `-EAGAIN`，而不是睡眠等待。

作用边界要清楚：它只覆盖**块层内部的资源等待**。如果 IO 最终仍要落到需要睡眠的路径（例如文件系统要分配块、要做日志提交、要拿 inode 锁），`REQ_NOWAIT` 管不到。因此用户态要真正获得「绝不阻塞」语义，通常要配合：

- `O_NONBLOCK` + `O_DIRECT`（避免 page cache 的读-改-写和锁竞争）
- 文件系统支持（ext4 在 `dioread_nolock` 等配置下效果最好；否则仍可能因为元数据日志而阻塞）
- **io_uring** + `IOSQE_ASYNC` / 或用 SQPOLL 内核线程提交

这也是 io_uring 能成为 HFT 低延迟落盘首选的底层原因之一：它把「提交」这件事从系统调用里挪了出去，而块层这侧靠 `REQ_NOWAIT` 保证不会偷偷睡掉。

</details>

</details>
---
