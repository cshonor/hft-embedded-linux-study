## 与上下章衔接

```
read() / write()
    ▼
VFS（Ch 13）            kiocb + iov_iter
    ▼
页缓存（Ch 16）— 命中则不经块层（folio）
    ▼
未命中 / 回写 ──► 文件系统 ──► bio（bio_vec 段）
    ▼
blk-mq：ctx（per-CPU，无锁）──► hctx ──► [调度器，可选] ──► 驱动
                                                            ▼
                                                     NVMe / SCSI 硬件
```

> 注：上图中「调度器」带方括号——多队列设备（NVMe）默认**不挂**调度器，见 14.5 / 14.6。

### 每一层的「货币单位」——理解分层的关键

把 Ch 13 / 14 / 16 串起来的最好方式，是看**每一层用什么单位描述数据**，以及**在哪一点上兑换成下一种货币**：

| 层 | 数据单位 | 核心结构 | 入口函数 | 归属章 |
|---|---|---|---|---|
| 用户态 | 文件描述符 + 缓冲区 | `fd`, `void *buf` | `read()` / `pread()` | 03 模块 Ch4 |
| **VFS** | 字节流 / iovec | `file`, `kiocb`, `iov_iter` | `vfs_read()` | **Ch 13** |
| **页缓存** | 页（v5.16+ **folio**） | `folio`, `address_space` | `filemap_read()` | **Ch 16** |
| **文件系统** | 块 / extent | `inode`, extent 映射表 | `->read_folio` / iomap | Ch 13 / 16 |
| **块层** | **段（scatter-gather）** | `bio`, `bio_vec` | `submit_bio()` | **Ch 14.4** |
| **blk-mq** | **request + tag** | `request`, `sbitmap` | `blk_mq_submit_bio()` | **Ch 14.5** |
| **驱动** | 硬件命令 | NVMe SQE / SCSI CDB | `queue_rq()` | Ch 17 |
| **硬件** | 512B 扇区 | PRP / SGL 描述符 | DMA | — |

**兑换点**就是排障的观察点：字节 → 页（页缓存）→ 块（文件系统映射）→ 段（bio）→ 请求（tag）→ 硬件命令。想知道「慢在哪一层」，就在这条链路上逐段打点（`blktrace` 的 Q/G/I/D/C 正好对应后几段）。

### 「哪一层会阻塞」——HFT 排障要的第一张表

| 层 | 可能阻塞的原因 | 表现 |
|---|---|---|
| VFS | inode 锁（`inode_lock`）、`rwsem` 竞争 | 多线程读写同一文件互相等 |
| 页缓存 | **folio 锁**（正在回写的页）、内存回收 | `read` 卡住等 `PG_locked` |
| 文件系统 | 日志提交（jbd2）、延迟分配、extent 树分裂 | 写突然抖动 |
| 块层 | **tag 耗尽**、`queue_limits` split | 请求排队 |
| blk-mq | hctx 锁竞争（CPU 多于 hw queue 时） | 多核扩展性不线性 |
| 驱动/硬件 | 队列满、GC、磨损均衡 | 设备级长尾 |

### 澄清一个流传很广的误解：O_DIRECT / io_uring **不绕过块层**

很多资料说「O_DIRECT 绕过内核，直接写盘」——**这是错的**。准确的作用边界：

| 手段 | **绕过了什么** | **没有绕过什么** |
|---|---|---|
| **`O_DIRECT`** | **页缓存**（无 page cache 拷贝、无 read-ahead） | VFS、文件系统、块层、blk-mq、驱动 —— **全都还在**；且要求三对齐（见 14.2），否则 `EINVAL` |
| **`io_uring`** | **系统调用提交/回收开销**（批量提交 + 可选 SQPOLL 内核线程免 syscall） | 块层、文件系统、页缓存（除非同时用 `O_DIRECT`）；`IOSQE_ASYNC` 才强制异步 |
| **`mmap`** | `read()` 的那次**内核→用户拷贝** | **不绕过页缓存**；首次访问触发缺页，读盘时是 major fault，反而可能更慢 |
| **`SPDK` / 用户态 NVMe 驱动** | **整个内核块层**（vfio 映射，用户态直接提交 SQE） | 需要把设备从内核 `nvme` 驱动上**解绑独占**，该盘不能再给文件系统用 |

→ 想在 HFT 里真正拿到「接近硬件」的延迟，只有最后一行（SPDK）算数，代价是放弃文件系统。前面三项都是在**内核 I/O 栈内部**做减法。

### 延迟量级参考（务必实测，切勿照抄）

| 路径 | 典型量级 |
|---|---|
| 页缓存命中（热数据） | 亚微秒 ~ 1μs |
| 走完整内核栈到 NVMe（4K 随机读） | 10μs 量级（软件栈）+ 设备时间 |
| NVMe 设备本身（PCIe Gen3/Gen4） | 数十 ~ 百微秒 |
| 机械 HDD 随机 4K | 5 ~ 10 ms |

量级的意义在于**判断瓶颈归属**：若实测 4K 读是几百微秒而设备标称几十微秒，问题在软件栈或队列深度，不在盘。

→ 上一章 [Ch 13 VFS](../../chapter-13-vfs/) · 下一章 [Ch 16 页缓存](../../chapter-16-page-cache/) · [03 模块 Ch4 文件 I/O](../../../03-linux-userspace-api/chapter-04-file-io-universal/)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** read() 到磁盘 IO 的完整路径经过哪些层？HFT 如何绕过？

<details><summary>答案</summary>

read() → VFS → page cache（miss）→ bio → IO 调度器 → 块设备驱动 → 硬件。HFT 绕过方法：1) O_DIRECT 跳过 page cache（直接 DMA 到用户态 buffer）；2) io_uring 异步提交（不阻塞等待）；3) mmap + madvise(MADV_WILLNEED) 预读。O_DIRECT 对 NVMe 特别有效——减少一次内核拷贝。

**注意「绕过」的准确边界**：这三条都**不绕过块层**。O_DIRECT 只跳页缓存，VFS/文件系统/块层/blk-mq/驱动全都在；io_uring 省的是系统调用开销；mmap 甚至不跳页缓存。真正绕过整个内核块层的只有 SPDK 这类用户态驱动，代价是要把设备从内核 nvme 驱动解绑独占。

</details>

**Q2.** 用了 `O_DIRECT` + `io_uring` 之后，I/O 还经过块层（bio / blk-mq）吗？

<details><summary>答案</summary>

**仍然经过。** 这是流传很广的一个误解。

- `O_DIRECT` 绕过的只有**页缓存**——不做 page cache 拷贝、不做 read-ahead。VFS → 文件系统 → 构造 bio → `submit_bio()` → `submit_bio_noacct()`（split/分区映射/计费）→ `blk_mq_submit_bio()`（取 tag）→ 驱动 `queue_rq()`，一个都不少。而且它还**新增**了约束：缓冲区地址、长度、偏移必须对齐到 `logical_block_size`，否则直接 `-EINVAL`（见 14.2）。
- `io_uring` 省的是**系统调用提交与回收的开销**（批量提交；开 SQPOLL 后连 syscall 都不用）。IO 栈本身没变。
- 想要「接近硬件」的延迟，只有 **SPDK / 用户态 NVMe 驱动**（vfio 映射，用户态直接填 SQE）才真正跳过整个内核块层——代价是设备被独占，不能再挂文件系统。

所以 HFT 调优的正确顺序是：先在**内核栈内部**做减法（页缓存 → 拷贝 → syscall），确认瓶颈确实在内核栈而不是设备，再考虑用户态驱动这种架构级改动。

</details>

**Q3.** 一次 4KB 的 `read()`，数据在各层分别以什么为单位被处理？为什么说「货币兑换点」是排障的观察点？

<details><summary>答案</summary>

| 层 | 单位 | 结构 |
|---|---|---|
| VFS | 字节流 / iovec | `file` + `kiocb` + `iov_iter` |
| 页缓存 | **页（folio）** | `folio`, `address_space` |
| 文件系统 | **块 / extent** | `inode` + extent 映射 |
| 块层 | **段（scatter-gather）** | `bio` + `bio_vec` |
| blk-mq | **request + tag** | `request`, `sbitmap` |
| 驱动 | **硬件命令** | NVMe SQE / SCSI CDB |
| 硬件 | **512B 扇区** | PRP / SGL |

因为**每兑换一次就多一层开销与一次排队机会**，而延迟就是在这些兑换点上累积的：页缓存未命中 → 文件系统查映射 → 构造 bio → 取 tag（可能耗尽）→ 进 hctx（可能等锁）→ 驱动下发（队列可能满）→ 硬件（可能 GC）。

`blktrace` 的 Q/G/I/D/C 打点正对应后几段，能直接看出时间花在哪一段。定位思路是自上而下逐段测量，而不是一上来就怀疑设备。

</details>

</details>
---
