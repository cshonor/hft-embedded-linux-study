## ① 缓存策略与写回 · Write-back

Linux 对 **可缓存的页数据** 采用 **写回（write-back）** — 非 no-write、非 write-through。

| 策略 | 行为 | 谁在用 |
|------|------|--------|
| **no-write** | 写直接穿透，不进缓存 | 极罕见（某些 DMA 设备映射） |
| **write-through** | 每次写**同时**更缓存与磁盘 | 部分带电池保护的 RAID 卡缓存策略 |
| **write-back** | 写只进缓存 → 标脏 → **异步**写回 | **Linux 页缓存的默认策略** |

```
应用 write()
    ▼
页缓存（内存）— 通常立即返回，不碰磁盘
    ▼
（稍后）flusher 线程写回 ──► bio（Ch 14）──► 设备
```

> **关键认知：** write-back 下 `write()` 返回 ≠ 数据落盘。返回只意味着"内核已经把你的数据抄进页缓存"。
> 崩溃时**未回写的脏页全部丢失**；只有 `fsync`/`fdatasync`/`O_SYNC` 才提供落盘保证。

---

### 脏页住在哪里？—— 版本断崖

LKD3rd 的讲法是「脏页挂在 `address_space` 的 dirty 链表上」。**v6.6 已经不是这样**：

```c
/* include/linux/fs.h — v6.6 struct address_space（节选） */
struct address_space {
	struct inode		*host;
	struct xarray		i_pages;      /* ← 存页的地方（详见 16.3） */
	struct rw_semaphore	invalidate_lock;
	struct rb_root_cached	i_mmap;       /* 映射了该文件的 VMA 们 */
	unsigned long		nrpages;
	pgoff_t			writeback_index;
	const struct address_space_operations *a_ops;
	errseq_t		wb_err;       /* fsync 返回错误靠它 */
	...
};
```

`struct address_space` 里**根本没有任何链表字段**。脏页的「在哪儿」变成了两层：

| 层次 | 机制 | 源码位置 |
|------|------|---------|
| **页级标记** | xarray 的 3 个 tag 位：`PAGECACHE_TAG_DIRTY`(=`XA_MARK_0`)、`_WRITEBACK`(=`XA_MARK_1`)、`_TOWRITE`(=`XA_MARK_2`) | `include/linux/fs.h`（address_space 之后紧接定义） |
| **inode 级排队** | 脏 inode 挂到 `bdi_writeback` 的 `b_dirty` / `b_io` / `b_more_io` / `b_dirty_time` 四条链表 | `include/linux/backing-dev-defs.h` |

**为什么这么改？** 链表藏在 xarray 里意味着「这页脏不脏」是**页的属性**，而不再是「这个文件的私有链表成员」。好处是 `mapping_tagged()` 可以 O(1) 回答「这个文件有没有脏页」，不必扫链表；代价是理解成本上升。

---

### 四个旋钮（v6.6 数值，与 LKD 时代完全一致）

```c
/* mm/page-writeback.c — v6.6 原文 */
static int dirty_background_ratio = 10;           /* :74  */
static int vm_dirty_ratio = 20;                   /* :91  */
unsigned int dirty_writeback_interval = 5 * 100;  /* :102 厘秒 → 5 秒   */
unsigned int dirty_expire_interval = 30 * 100;    /* :109 厘秒 → 30 秒  */
static long ratelimit_pages = 32;                 /* :68  */
```

| sysctl | 默认 | 含义 |
|--------|------|------|
| `vm.dirty_background_ratio` | **10%** | 脏页占比超此值 → **后台**唤醒 flusher（不阻塞写者） |
| `vm.dirty_ratio` | **20%** | 脏页占比超此值 → **前台限速**（见下，会阻塞写者） |
| `vm.dirty_writeback_centisecs` | **500**（5s） | flusher 线程被唤醒的周期 |
| `vm.dirty_expire_centisecs` | **3000**（30s） | 脏页超过这个年纪**必须**写回 |
| `ratelimit_pages`（非 sysctl） | **32** | 每个 CPU 脏了 32 页才去检查一次阈值（摊薄开销） |

> **HFT 最该记住的一条：** 后台阈值（10%）只是唤醒，前台阈值（20%）才是**把你自己的写线程按在原地**。
> `balance_dirty_pages_ratelimited_flags()`（`page-writeback.c:1992`）会在脏页逼近上限时主动让当前写进程**睡眠等写回**，
> 这是 `write()` 偶发长尾的一个经典来源——看起来是"内存写"，实际在等磁盘。

**HFT：** tick 路径 **不应依赖** 写回完成；关键持久化用 **`fsync`** / 独立日志盘 / **`O_DIRECT`** 自管缓存。

#### 落盘保证的四种写法（语义严格递增）

| 手段 | 语义 | 代价 |
|------|------|------|
| `write()` | 只保证进页缓存 | 最快，崩溃丢数据 |
| `fdatasync(fd)` | 该文件的**数据**脏页落盘（不保证元数据如 mtime） | 一次写盘 |
| `fsync(fd)` | 数据 **+ 元数据**都落盘 | 比 fdatasync 多一次元数据 IO |
| `O_SYNC`（open 时） | 每次 `write` 自带 fdatasync 语义 | 每次写都等盘，**最慢** |

> 注意 `sync()` 是**全系统**刷盘，会把其他文件、其他盘的脏页一起带下去——HFT 里误用 `sync()` 能造成**几百毫秒级**的长尾，绝不能进热路径。

→ [06.6 SysPerf Ch8 FS](../../../06.6-systems-performance/chapter-08-file-systems/) · [Ch7 `vm.dirty_*`](../../../06.6-systems-performance/chapter-07-memory/notes/section-7.6-调优指南.md) · [Ch 14 块 I/O](../../chapter-14-block-io/)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** write-back 和 write-through 的区别？Linux 为什么选 write-back？

<details><summary>答案</summary>

write-through：写同时更新缓存和磁盘 → 数据安全但慢。write-back：只写缓存，标记脏页，后台异步写回磁盘 → 快但断电可能丢数据。Linux 选 write-back 因为：1) 多次写合并为一次 IO（减少磁盘操作）；2) 延迟写让 IO 调度器合并排序。HFT 交易日志不能丢数据 → 用 O_SYNC 或 fsync() 强制写回。

</details>

**Q2.** 进程调用 `write()` 成功返回后立刻断电，数据会丢吗？如果会，最多丢多少？

<details><summary>答案</summary>

会丢。`write()` 返回只表示数据已进入**页缓存**，脏页要到 flusher 线程写回后才真正落盘。最坏情况下丢失窗口 = `vm.dirty_expire_centisecs`（默认 30 秒）——这是脏页被强制写回的最长停留时间；若脏页总量一直没超过 10% 后台阈值，就全靠这 30 秒的过期机制兜底。

要消除这个窗口只能显式同步：`fdatasync()`（只要数据）、`fsync()`（数据+元数据）或打开时带 `O_SYNC`。代价是每次调用都要等一次真实磁盘 IO（NVMe ~50-100μs，SATA SSD ~1ms，机械盘 ~10ms）。

反过来，如果**接受**丢最后若干秒（例如行情快照文件，重算比保数据便宜），那就让 write-back 自己去跑，避免同步开销。这是 HFT 里「持久化成本 vs 数据价值」的具体权衡点。

</details>

**Q3.** 已经把 `vm.dirty_background_ratio` 调大了，为什么业务线程还是会被 `write()` 卡住？

<details><summary>答案</summary>

因为**卡人的不是后台阈值，是前台阈值**。`dirty_background_ratio`（10%）只负责唤醒 flusher 线程在后台写回，完全不阻塞写者；真正会阻塞的是 `vm.dirty_ratio`（20%）——脏页占比一旦突破它，`balance_dirty_pages_ratelimited_flags()`（`mm/page-writeback.c:1992`）会让**正在写脏页的进程直接进入睡眠**，等写回腾出额度再醒。

所以调大 background 反而会让脏页攒得更多、更快撞上前台阈值，卡得更狠。

正确做法：
1. **调小** `vm.dirty_ratio`（如 5%~10%）让前台限速更早介入，把"一次性大停顿"摊成"多次小停顿"，降低 p99；
2. 或者**在写者侧主动 `fsync`**，把节奏拿回自己手里；
3. 更好的办法是让脏页根本产生不了那么多——`O_DIRECT` 或 `io_uring` 直接提交 IO，绕开页缓存写路径。
4. 观测确认：`/proc/vmstat` 里 `nr_dirty`、`nr_writeback` 两个计数器，配合 `bcc/biosnoop` 看是不是真的在等盘。

</details>

</details>
---
