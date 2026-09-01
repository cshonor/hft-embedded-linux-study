## ② 扇区与块 · Sectors and Blocks

| 术语 | 层级 | 典型大小 |
|------|------|----------|
| **扇区（sector）** | **硬件** 最小可寻址单元 | 常见 **512B**（4Kn 盘 4096B） |
| **块（block）** | **文件系统 / 内核** 逻辑最小单元 | 扇区的 **2^n 倍** · ≤ **一页**（512B / 1KB / **4KB**） |

```
磁盘扇区（512B）──► 多个扇区组成 FS「块」（如 4KB）
```

### 四个「块大小」——最容易混为一谈的一组概念

LKD 只讲了扇区和块，但现代内核里 `queue_limits` 里躺着**四个**粒度字段，含义各不相同。混用它们就会在 4Kn 盘上踩坑：

| 字段（`struct queue_limits`，blkdev.h:~297） | 谁定的 | 含义 | 典型值 |
|---|---|---|---|
| `logical_block_size` | 设备驱动 | **对外承诺**的最小可寻址/可写单元。内核保证不会发出小于它的 I/O | 512（512e）/ 4096（4Kn） |
| `physical_block_size` | 设备驱动 | 设备**内部真实**的最小物理写入单元 | 4Kn=4096；**512e 也是 4096** |
| `io_min` | 设备驱动 | 设备**偏好**的最小 I/O 粒度（小于它有性能惩罚但不报错） | RAID 条带宽度 / NVMe 常用 4096 |
| `io_opt` | 设备驱动 | **最优** I/O 粒度（再大无收益） | RAID 整条带 / NVMe AWUPF |

三者关系是**单调不降**的，内核在设置时强制维持这个不变量——`blk_queue_logical_block_size()`（blk-settings.c:309）：

```c
void blk_queue_logical_block_size(struct request_queue *q, unsigned int size)
{
	struct queue_limits *limits = &q->limits;
	limits->logical_block_size = size;
	if (limits->physical_block_size < size)      /* physical >= logical */
		limits->physical_block_size = size;
	if (limits->io_min < limits->physical_block_size)
		limits->io_min = limits->physical_block_size;   /* io_min >= physical */
	...
}
```

→ 驱动**不可能**只报 logical=512 而不带出 physical=4096；内核会自己抬上去。所以「512e 盘在内核眼里其实知道自己内部是 4K」。

### 512e vs 4Kn：read-modify-write 惩罚

| 形态 | logical | physical | 内核视角 | 写 512B 时固件干什么 |
|---|---|---|---|---|
| 传统 512n | 512 | 512 | 512 边界 | 直接写 |
| **512e**（Advanced Format） | **512** | **4096** | 以为是 512 盘 | **读 4K → 改 512B → 写 4K** |
| **4Kn**（Native 4K） | **4096** | 4096 | 知道是 4K 盘 | 直接写（不会发未对齐写） |

**512e 是性能陷阱**：设备对内核撒了个善意的谎（为了兼容老分区表和老 OS），代价是随机小写落到盘上会变成 **读-改-写** 三倍放大。HFT 落盘行情 tick（每条几十字节）时，512e 盘的 p99 写延迟方差会明显劣于 4Kn。

判断手上的盘是哪一种：

```bash
cat /sys/block/nvme0n1/queue/logical_block_size   # 512 → 512e
cat /sys/block/nvme0n1/queue/physical_block_size  # 4096 → 内部确实是 4K
cat /sys/block/nvme0n1/queue/minimum_io_size      # io_min
cat /sys/block/nvme0n1/queue/optimal_io_size      # io_opt
```

### `alignment_offset`：分区没对齐 = 全程 RMW

即便盘是 4Kn，如果分区起点没落在 4K 边界上，文件系统块的边界就与物理块**系统性错开半格**——每个块写都跨两个物理块：

```
物理块边界:  |===4K===|===4K===|===4K===|
MBR 分区起点 LBA 63 → 偏移 32256 字节（32256 % 4096 = 2048，错开半格）
FS 块:            |==4K==|==4K==|   ← 每块跨两个物理块，100% RMW
```

`queue_limits.alignment_offset` 就是内核记录这个偏差的字段，`misaligned` 是非对齐标志。现代 `fdisk`/`parted` 默认从 **LBA 2048**（= 1MiB）开始分区，正是为了让任何常见块大小都能整除。

### 块与页：原答案里的一个过度简化

> 常见说法：文件系统块 = 内存页 = 4KB。

**实际约束是「FS 块 ≤ 页」，不是「等于」。**

| 关系 | 后果 |
|---|---|
| FS 块 = 页（4KB/4KB） | 最理想：page cache 一页 ↔ 一个 FS 块，一一对应，无额外映射层 |
| FS 块 < 页（1KB/2KB） | 一页装 2~4 个块，page cache 仍能工作，但需要 **`buffer_head`** 记录「页内哪些块是最新的」→ 见 14.3 |
| FS 块 > 页 | **不可能**。page cache 以页为单位管理，页装不下一个块就无法缓存 |

ext4 允许 1024/2048/4096 三种块大小，`mkfs.ext4 -b 1024` 用于海量小文件场景。

### O_DIRECT 的对齐铁律（HFT 落盘必踩）

buffered I/O 下内核会默默帮你做读-改-写；**O_DIRECT 不会**——它会直接拒绝。门神是 `bdev_iter_is_aligned()`（blkdev.h:1320）：

```c
static inline bool bdev_iter_is_aligned(struct block_device *bdev,
					struct iov_iter *iter)
{
	return iov_iter_is_aligned(iter, bdev_dma_alignment(bdev),
				   bdev_logical_block_size(bdev) - 1);
}
```

三个东西**同时**对齐才算数，任一不满足 → `write()` 返回 `-EINVAL`：

| 需对齐项 | 对齐到 | 默认值 |
|---|---|---|
| 用户缓冲区**地址** | `dma_alignment`（`limits.dma_alignment`，无队列时兜底 511） | 512 |
| 传输**长度** | 同上 + `logical_block_size - 1` | 512 |
| 文件**偏移** | `logical_block_size` | 512 |

**HFT 实操**：用 `posix_memalign(&buf, 4096, size)` 而不是 `malloc`；结构体大小补齐到 4K；`pwrite` 的 offset 取 4096 整数倍。行情日志追加写用 `O_APPEND` + 4K 对齐缓冲，避免内核在热路径上替你做 RMW。

→ **Ch 12** 页 · **Ch 16** 页缓存（address_space）· **Ch 14.3** buffer_head 的页内块映射


<details>
<summary>自测题（点击展开）</summary>

**Q1.** 扇区(sector)、块(block)、页(page) 的关系和大小？

<details><summary>答案</summary>

扇区 = 硬件最小传输单元（通常 512B，现代 4K）。块 = 文件系统最小分配单元（通常 4KB = 8 sector）。页 = 内存管理最小单元（4KB）。文件系统块 = 内存页 = 4KB 不是巧合：VFS 设计为块大小 = PAGE_SIZE，使 page cache 和文件系统块一一对应。

**修正**：准确约束是「FS 块 ≤ 页」而非「= 页」。ext4 支持 1024/2048/4096；块小于页时一页容纳多个块，需要 buffer_head 做页内块映射（见 14.3）；块大于页则 page cache 无法缓存，因此不被允许。

</details>

**Q2.** 512e 盘上做 512 字节随机小写，为什么性能会崩？内核能感知到吗？

<details><summary>答案</summary>

512e（Advanced Format）设备对外报 `logical_block_size=512`，但内部 `physical_block_size=4096`。**内核是能感知的**——`queue_limits` 里两个字段分开存，且 `blk_queue_logical_block_size()`（blk-settings.c:309）会强制 `physical >= logical`，驱动不可能只报 512。

但内核**不会因此拒绝 512B 写**，因为设备承诺了 512 可寻址。于是设备固件收到 512B 写时被迫做 read-modify-write：读整个 4K 物理块 → 替换其中 512B → 写回整个 4K。写放大 8 倍，且延迟从「一次写」变成「读+写」串行，p99 方差暴涨。

4Kn 盘（logical 也报 4096）内核知道边界，不会发未对齐写，因此没有这个惩罚。这是 HFT 落盘选盘时应当确认 `logical_block_size` 而非只看「是不是 4K 盘」的原因。

</details>

**Q3.** O_DIRECT 写入返回 `-EINVAL`，buffered I/O 同样位置却正常。最可能的原因是什么？

<details><summary>答案</summary>

用户缓冲区地址、长度或文件偏移未对齐到 `logical_block_size`。门神是 `bdev_iter_is_aligned()`（blkdev.h:1320），它同时检查三项：缓冲区地址对齐到 `dma_alignment`（默认 511，即 512 字节）、长度对齐到 `dma_alignment | (logical_block_size - 1)`、偏移对齐到 `logical_block_size`。任一不满足即返回 EINVAL。

buffered I/O 正常是因为 page cache 会替你做读-改-写——对齐问题被缓存层吸收了，代价是热路径上多一次 RMW。

修法：`posix_memalign(&buf, 4096, size)` 分配缓冲区，结构体补齐到 4K 倍数，pwrite 的 offset 取 4096 整数倍。注意 `malloc` 只保证 16 字节对齐，一定不够。

</details>

</details>
---
