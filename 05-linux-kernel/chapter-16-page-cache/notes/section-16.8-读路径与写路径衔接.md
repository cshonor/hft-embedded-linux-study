## 读路径与写路径（衔接）

```
read(path)
    ▼
VFS（Ch 13）─► 查 address_space / 页缓存
    ├─ 命中 ──► 拷贝到用户缓冲（零拷贝/mmap 可优化）
    └─ 未命中 ──► 读盘（Ch 14 bio）─► 填入页缓存

write(path)
    ▼
页缓存（可能 COW，Ch 3/15）─► 标脏 ──► flusher 异步写回
```

---

### 读路径：v6.6 真实函数链

LKD 时代的 `generic_file_buffered_read()` **在 v6.6 已经不存在了**，换成 `filemap_read()`：

```c
/* mm/filemap.c:2617 — v6.6 */
ssize_t filemap_read(struct kiocb *iocb, struct iov_iter *iter, ssize_t already_read)
{
	struct folio_batch fbatch;      /* ← 批量容器，不是单个 page */
	...
	folio_batch_init(&fbatch);
	do {
		...
		error = filemap_get_pages(iocb, iter->count, &fbatch, false);
		/* 然后逐 folio copy 到用户态 iter */
	} while (...);
```

**关键在 `filemap_get_pages()`（`filemap.c:2534`）的四步决策：**

```c
retry:
	/* ① 先批量从 xarray 里捞，能捞几个算几个 */
	filemap_get_read_batch(mapping, index, last_index - 1, fbatch);

	if (!folio_batch_count(fbatch)) {          /* ② 一个都没捞到 = 缓存 miss */
		if (iocb->ki_flags & IOCB_NOIO)
			return -EAGAIN;                /*    ← 不允许发起 IO，直接返回 */
		page_cache_sync_readahead(mapping, ra, filp, index,
					  last_index - index);   /* 同步预读（会读盘） */
		filemap_get_read_batch(mapping, index, last_index - 1, fbatch);  /* 再捞一次 */
	}
	if (!folio_batch_count(fbatch)) {          /* ③ 还是没有（预读被限流/失败） */
		if (iocb->ki_flags & (IOCB_NOWAIT | IOCB_WAITQ))
			return -EAGAIN;                /*    ← 不等待，交给 io_uring 异步重试 */
		err = filemap_create_folio(filp, mapping,
					   iocb->ki_pos >> PAGE_SHIFT, fbatch);  /* 分配页并发 IO */
		...
	}
	/* ④ 捞到了 → 顺手异步预读下一批（filemap_readahead → page_cache_async_ra） */
```

| 步骤 | 函数 | 是否阻塞 |
|------|------|---------|
| ① 批量捞缓存 | `filemap_get_read_batch()` | 否（xarray 查询） |
| ② miss → **同步预读** | `page_cache_sync_readahead()`（`filemap.c:2556`） | **是**（要等盘） |
| ③ 兜底建页发 IO | `filemap_create_folio()` | 是 |
| ④ 命中 → **异步预读** | `page_cache_async_ra()`（`filemap.c:2529`） | **否**（只提交，不等） |
| ⑤ 拷贝到用户态 | `copy_folio_to_iter()` 一族 | 否 |

> **`IOCB_NOWAIT` / `IOCB_NOIO` 这两条 `-EAGAIN` 分支是 io_uring 的灵魂**：
> 带 `RWF_NOWAIT` 的读如果会阻塞（缓存 miss），内核**不阻塞而是返回 `-EAGAIN`**，
> io_uring 收到后把它交给**异步 worker 线程**去做。策略线程因此**永不因缺页而卡在磁盘上**。
> 这是 HFT 里"读文件但不让 tick 线程停下"的标准做法。

---

### 预读（readahead）：读多少由谁定

```c
/* mm/readahead.c:141 — v6.6 */
ra->ra_pages = inode_to_bdi(mapping->host)->ra_pages;   /* 默认值来自块设备 */
```

```c
/* mm/readahead.c:337 — v6.6 原文注释，初始窗口的放大规则 */
/*
 * Set the initial window size, round to next power of 2 and square
 * for small size, x 4 for medium, and x 2 for large
 * for 128k (32 page) max ra
 * 1-2 page = 16k, 3-4 page 32k, 5-8 page = 64k, > 8 page = 128k initial
 */
static unsigned long get_init_ra_size(unsigned long size, unsigned long max)
```

| 你的请求大小 | 内核实际预读的初始窗口 |
|-------------|---------------------|
| 1~2 页 | 16 KB |
| 3~4 页 | 32 KB |
| 5~8 页 | 64 KB |
| > 8 页 | 128 KB（封顶于 `ra_pages`） |

**行为：** 顺序读被识别后，预读窗口会**逐次放大**（16K → 32K → 64K → 128K），直到封顶；
一旦检测到**随机访问**（连续两次读不相邻），窗口**立即收缩**。

| 调优 | 场景 |
|------|------|
| `blockdev --setra N /dev/nvme0n1` | 顺序扫描为主的行情回放，可放大到 512~1024 页 |
| `posix_fadvise(POSIX_FADV_SEQUENTIAL)` | 明确告诉内核"我要顺序读"，直接把窗口拉满 |
| `posix_fadvise(POSIX_FADV_RANDOM)` | 明确"随机读"，**关掉预读**（避免读无用的页浪费带宽） |
| `posix_fadvise(POSIX_FADV_DONTNEED)` | 读完就扔，不污染页缓存（回放场景必用） |
| `readahead()` / `madvise(MADV_WILLNEED)` | 提前把数据拉进缓存（**启动预热**用） |

---

### 写路径：从 `write()` 到磁盘的完整链条

```
write(fd, buf, n)
  │
  ├─ ① 在页缓存里找到/分配页，把用户数据拷进去
  ├─ ② folio_mark_dirty()  →  设置 xarray 的 PAGECACHE_TAG_DIRTY 标记
  ├─ ③ __mark_inode_dirty()  (fs/fs-writeback.c:2403)
  │       └─ 把 inode 挂到 bdi_writeback->b_dirty 链表
  ├─ ④ 返回（此时数据只在内存）
  │
  ╎  …… 稍后（后台阈值 / 30 秒过期 / fsync）……
  │
  ├─ ⑤ flusher 唤醒 → b_dirty 的 inode 挪到 b_io
  ├─ ⑥ mapping->a_ops->writepages()  ← 文件系统干活（ext4 要写 journal）
  ├─ ⑦ 构造 bio（Ch 14.4）→ submit_bio → blk-mq → 驱动
  └─ ⑧ IO 完成 → 清 dirty tag → 页变干净（可回收）
```

---

### 成本量级对照（NVMe 服务器，4KB 读）

| 情形 | 大致延迟 | 说明 |
|------|---------|------|
| 页缓存**命中** | **~1-5 μs** | xarray 查找 + `copy_to_user` |
| 页缓存 miss，需要读盘 | **~50-100 μs** | 走 bio → NVMe → 中断 → 完成 |
| 页缓存 miss + **队列拥塞** | **~1-10 ms** | 排队等盘，尾延迟主来源 |
| mmap 缺页（首次访问） | 同上 + 建页表开销 | 之后访问就是纯内存 |
| `fsync`（NVMe） | **~100-500 μs** | 数据 + 元数据 + FLUSH 命令 |
| `fsync`（机械盘） | **~5-20 ms** | 寻道 + 旋转 |

> **相差 10~1000 倍。** 这就是为什么"能不能让数据待在页缓存里"是 HFT 存储的第一性问题。

---

### 四种"绕开"手段的真实作用点（常被混为一谈）

| 手段 | 作用层 | 真实效果 | 代价 |
|------|--------|---------|------|
| **`mmap`** | 省掉 `copy_to_user` | 仍走页缓存、仍会缺页 | 缺页延迟不可控（需 `mlock` 预热） |
| **`O_DIRECT`** | **跳过页缓存** | 用户缓冲直接 DMA，无脏页、无 flusher | 必须**三对齐**（偏移/长度/缓冲，见 Ch 14.2）；丢预读与合并 |
| **`O_SYNC`** | 不跳页缓存 | 每次 write 自带落盘语义 | 每次都等盘，**最慢** |
| **`io_uring` + `RWF_NOWAIT`** | 不跳页缓存 | miss 时返回 `-EAGAIN` 转异步，**调用线程不阻塞** | 需要异步编程模型 |

> **注意：** `O_DIRECT` 和 `io_uring` **都不绕过块层**（Ch 14.7 已澄清）。
> 想真正绕过内核块层要上 SPDK/用户态驱动，代价是**独占整块设备**、失去文件系统。

### HFT 组合拳（按场景）

| 场景 | 推荐组合 |
|------|---------|
| 盘前加载历史行情 | `mmap` + `madvise(WILLNEED)` 预热 → `mlock` 钉住 → 之后读是纯内存 |
| 一次性大文件回放 | `posix_fadvise(SEQUENTIAL)` 放大预读 + 读完 `DONTNEED` 丢弃 |
| 交易日志落盘 | 独立 IO 线程 + `fsync`（或 `O_DIRECT` + 自管缓冲） + 专用 NVMe 盘 |
| 低延迟配置读取 | 启动时全量读进内存，运行时**只读内存**，永不碰文件 |



<details>
<summary>自测题（点击展开）</summary>

**Q1.** read() 命中 page cache 和 miss 的性能差异有多大？

<details><summary>答案</summary>

Hit：page cache → copy_to_user → ~1-5μs。Miss：page cache 未命中 → bio → IO 调度 → 磁盘 → DMA → page cache → copy_to_user → NVMe ~50-100μs，SATA ~1-10ms。HFT 启发：1) 预读（readahead/madvise WILLNEED）让数据提前进 cache；2) mmap 零拷贝跳过 copy_to_user；3) O_DIRECT 绕过 page cache（自管理 buffer）。

</details>

**Q2.** io_uring 的 `RWF_NOWAIT` 是怎么做到"读文件却不让调用线程阻塞"的？

<details><summary>答案</summary>

靠页缓存读路径里的两条 **`return -EAGAIN`** 分支（`mm/filemap.c` 的 `filemap_get_pages()`）：

```c
/* 缓存 miss 时 */
if (!folio_batch_count(fbatch)) {
	if (iocb->ki_flags & IOCB_NOIO)
		return -EAGAIN;                 /* ① 不允许发起 IO */
	page_cache_sync_readahead(...);
	filemap_get_read_batch(...);
}
if (!folio_batch_count(fbatch)) {
	if (iocb->ki_flags & (IOCB_NOWAIT | IOCB_WAITQ))
		return -EAGAIN;                 /* ② 允许 IO 但不允许等待 */
	...
}
```

逻辑很清楚：
1. 先尝试**不阻塞**地从 xarray 批量捞页（`filemap_get_read_batch`）；
2. 捞到了 → 直接拷贝返回，全程内存操作；
3. **一个都没捞到**（缓存 miss，必须读盘）→ 如果调用者带了 `RWF_NOWAIT`/`IOCB_NOIO`，**不阻塞，直接返回 `-EAGAIN`**；
4. io_uring 收到 `-EAGAIN` 后，把这个请求转交给**内核的异步 worker 线程池**去做，调用线程立刻返回去干别的，之后通过 CQ（完成队列）拿结果。

**对 HFT 的价值：** 这是"文件 IO 不进 tick 路径"的标准解法。策略线程发起读请求后继续执行策略逻辑，数据在 CQ 里等它，永远不会因为一次缺页被钉在磁盘 IO 上。

**注意它的局限：** `-EAGAIN` 本身不是免费的——如果**大部分读都 miss**，请求会频繁走"提交 → EAGAIN → 转 worker"这条路径，反而比直接阻塞读更慢。所以它适合"**绝大多数命中缓存，偶发 miss**"的场景。如果 miss 率本来就高，应该先解决缓存问题（预热、mlock、放大预读）。

</details>

**Q3.** 顺序读一个 200MB 的历史行情文件做回测，读完后发现系统里其他热数据被挤出了缓存，怎么办？

<details><summary>答案</summary>

这是 16.2 讲的"一次扫描污染"问题，在页缓存层面的标准解法是 **`posix_fadvise`** 三连：

```c
int fd = open("history.bin", O_RDONLY);

/* 1. 告诉内核这是顺序访问，把预读窗口拉满（默认最大 128KB 起步，会浪费在起始阶段） */
posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);

/* 2. 每读完一个区块（比如 64MB），立刻声明"这段我不要了" */
posix_fadvise(fd, offset, len, POSIX_FADV_DONTNEED);

/* 3. 若只扫一遍，还可以用 FADV_NOREUSE 提示"不会有二次访问" */
posix_fadvise(fd, 0, 0, POSIX_FADV_NOREUSE);
```

`POSIX_FADV_DONTNEED` 的作用是把指定范围的页**从页缓存里剔除**（干净页直接丢弃，脏页先写回），这样 200MB 的扫描数据就不会把策略进程的真热数据挤出 active 链表。

另外三个可选手段：
1. **放大设备预读**：`blockdev --setra 1024 /dev/nvme0n1`（默认通常 128~256 个扇区），顺序扫描时减少 IO 次数；
2. **`O_DIRECT` 直接绕开页缓存**：最彻底，但要自己做对齐（偏移、长度、缓冲区都按块大小对齐，见 Ch 14.2），且丢掉预读和合并收益——**回测这类吞吐优先、延迟不敏感的场景其实很合适**；
3. **cgroup 隔离**：把回测进程关进单独的 memory cgroup 并设 `memory.high`，让它的缓存增长受限，回收压力落在自己身上。

验证效果：用 `cachestat`（BCC）看命中率是否维持在预期水平，或 `cachetop` 按进程看 HITS/MISSES 分布，别只看总缓存量。

</details>

</details>
---
