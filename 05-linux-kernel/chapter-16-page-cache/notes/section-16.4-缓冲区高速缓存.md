## ④ 缓冲区高速缓存 · Buffer Cache

| 历史 | 磁盘块经 **buffer_head** 映射到页（Ch 14） |
|------|---------------------------------------------|
| **2.4+** | 独立 **buffer cache** 与 **page cache** **统一** |
| 效果 | 块 **直接缓存在页缓存** — 无双重拷贝、无重复占用 |

```
今天：read 文件块 ──► 页缓存中的一页（或 folio）──► 需要时用 bh/bio 描述块偏移 ──► 写盘（Ch 14）
```

---

### "buffer cache 没了"是对的吗？—— 一半对

| 层面 | 现状 | 说明 |
|------|------|------|
| **独立的 buffer cache** | ✅ 确实没了 | 2.4 起不再有按磁盘块（512B）单独组织的一套缓存 |
| **`struct buffer_head`** | ❌ **还在，且仍是主力** | `fs/buffer.c` 里活得好好的，元数据 IO 全靠它 |
| **块设备的页缓存** | ❌ **还在** | `/dev/sda` 也有 `inode->i_mapping`，元数据读写走的就是它 |

**两者的分工：**

```
                     ┌──── 数据路径 ──────────────────────────────────┐
                     │  文件内容 → 页缓存（folio）→ iomap / bio      │
   一次磁盘访问      │                                              │
                     ├──── 元数据路径 ────────────────────────────────┤
                     │  superblock / inode bitmap / 间接块           │
                     │    → 块设备页缓存 + buffer_head（sb_bread）   │
                     └───────────────────────────────────────────────┘
```

> **为什么元数据还用 bh？** 因为元数据是**小块、随机、强一致**的：一次改一个 inode 位图里的几个字节，
> 用 buffer_head 描述"这一页里的第 N 个 1KB 块"最直接，而且提交时要求顺序精确（journaling 依赖这个）。
> 反过来，数据路径追求**大块、顺序、高吞吐**，用 folio + iomap 直接映射整段更划算。

---

### 什么时候一个页会挂多个 buffer_head

当**块大小 < 页大小**时（例如 4KB 页 + 1KB 文件系统块），一个页被切成 4 份，每份一个 bh：

```c
/* include/linux/buffer_head.h — v6.6（节选） */
struct buffer_head {
	unsigned long b_state;        /* BH_Uptodate / BH_Dirty / BH_Lock / BH_Mapped ... */
	struct buffer_head *b_this_page;  /* ← 同一页内的下一个 bh，环形链表 */
	struct page *b_page;              /* 所属的页 */
	sector_t b_blocknr;               /* 逻辑块号 */
	size_t b_size;                    /* 块大小 */
	char *b_data;                     /* 数据在页内的位置 */
	struct block_device *b_bdev;
	bh_end_io_t *b_end_io;
	...
};
```

| 常见组合 | 一个页挂几个 bh |
|---------|----------------|
| 4KB 页 + 4KB 块（**最常见，ext4/xfs 默认**） | **1 个**（`b_this_page` 指向自己） |
| 4KB 页 + 1KB 块（小文件或老 FS） | 4 个 |
| 4KB 页 + 512B 块（FAT/老式） | 8 个 |

> 这也是 `/proc/meminfo` 里那个 `Buffers:` 字段的由来（`fs/proc/meminfo.c` 里 `show_val_kb(m, "Buffers: ", i.bufferram)`）——
> 它统计的就是这类"作为块缓冲区被映射着的页"。在只有 ext4/xfs 且块大小=页大小的现代服务器上，这个值通常**很小**。

---

### 观测

| 指标 | 怎么看 | 健康值 |
|------|--------|--------|
| `Buffers` | `grep ^Buffers /proc/meminfo` | 现代 ext4/xfs 服务器上通常只有几十 MB |
| `Cached` | `grep ^Cached /proc/meminfo` | 这是页缓存主体（**不含** SwapCached） |
| `free` 的 `buff/cache` 列 | 上面两者**之和** | 高不代表有问题——页缓存本来就该吃满空闲内存 |
| bh 用量 | `slabtop \| grep buffer_head` | 元数据密集操作（大量 create/unlink）时会涨 |

> **HFT 实操提示：** 看到 `buff/cache` 很大不要慌，那是内核在用空闲内存做缓存，不是泄漏。
> 真正要关心的是 `MemAvailable`（已扣除不可回收部分）。只有在确认缓存命中率暴跌
> （`cachestat`/`cachetop`）时，缓存才值得调优。

---

### 第二代改造：iomap

`buffer_head` 的问题是**页与块的耦合**。iomap 把"文件偏移 → 磁盘偏移"抽成一层映射查询：

| | 老方式（bh） | iomap |
|---|---|---|
| 文件系统要提供 | `get_block()` 逐块回调 | `iomap_begin()` 返回一整段 **extent** |
| 大文件顺序 IO | 每块回调一次 | 一次映射覆盖 MB 级 |
| 用户 | ext2/minix/老 ext3 | **XFS（全部）、ext4（部分路径）、btrfs、gfs2** |

→ 详见 [Ch 14.3 buffer_head](../../chapter-14-block-io/notes/section-14.3-缓冲区与缓冲区头.md) 的字段解剖与状态位表



<details>
<summary>自测题（点击展开）</summary>

**Q1.** buffer cache 和 page cache 的关系？现代内核还有 buffer cache 吗？

<details><summary>答案</summary>

2.4 之前 buffer cache 缓存磁盘块（512B），page cache 缓存文件页（4KB），两者重复缓存同一数据。2.4 后统一：buffer cache 合并到 page cache 中，buffer_head 只作为页内块偏移的描述符。`free` 命令中的 "buff/cache" 就是 page cache（含 buffer）。现代内核已无独立 buffer cache。

</details>

**Q2.** 既然 buffer cache 已经合并进 page cache，为什么 `struct buffer_head` 还活着？

<details><summary>答案</summary>

因为合并掉的是**独立的缓存实体**，不是 `buffer_head` 这个**描述符**。

- **消失的**：一套按 512B/1KB 磁盘块单独组织的缓存，与页缓存重复存放同一份数据（双重内存占用 + 双重拷贝 + 一致性维护）。
- **留下的**：`struct buffer_head` 作为"这一页里第 N 个块"的描述符，被文件系统用来读写**元数据**。

元数据路径为什么还离不开 bh：
1. 元数据是**小而随机**的改动（改一个 inode 位图里的几字节），bh 的"页内某块"语义最直接；
2. 日志（journaling）要求写入顺序严格可控，bh 的 `BH_Dirty`/`BH_Lock`/`BH_Uptodate` 状态机提供了这种精确控制；
3. 块设备本身（`/dev/sda`）也有 `bd_inode->i_mapping`，`sb_bread()` 就是"读块设备的那一页 + 取对应 bh"。

所以现代分工是：**数据路径走页缓存 + iomap/bio，元数据路径走块设备页缓存 + buffer_head**。
在 ext4/xfs 且块大小 = 页大小的典型配置下，一个页只挂 1 个 bh，`Buffers` 字段也就很小。

</details>

**Q3.** 一台服务器上 `free` 显示 buff/cache 占了 40GB，是不是内存泄漏？要不要清？

<details><summary>答案</summary>

**大概率不是泄漏，不要清。**

页缓存的设计原则就是"空闲内存不闲着"——进程不用它就拿来做磁盘缓存，需要时（内存压力）通过 LRU 回收，不会真的把进程挤死。判断内存是否紧张的指标是 **`MemAvailable`**，不是 `free` 那列。

真正需要怀疑缓存的场景，看这三个信号：
1. `MemAvailable` 持续很低 + `pgscan`/`pgsteal` 一直在涨（`/proc/vmstat`）→ 说明确实在频繁回收；
2. **缓存命中率暴跌**（`cachestat` 看 ratio，或 `cachetop` 按进程看 HITS/MISSES）→ 说明工作集大于可用缓存，或有"一次扫描"在污染（见 16.2）；
3. `Buffers` 异常大（几 GB 级）→ 说明有大量块设备直读或元数据密集操作，值得用 `slabtop` 看 `buffer_head` slab。

清除缓存（`echo 3 > /proc/sys/vm/drop_caches`）只应该在**基准测试前**做——它保证每次测量都从冷缓存开始，结果可复现。生产环境清缓存的代价是随后必然的批量冷启动 IO，对延迟敏感的系统是负面操作。

HFT 的正确做法不是清缓存，而是**控制工作集**：`mlockall` 钉住策略进程、给回放/日志进程单独 cgroup、对一次性大文件读完后 `madvise(MADV_DONTNEED)`。

</details>

</details>
---
