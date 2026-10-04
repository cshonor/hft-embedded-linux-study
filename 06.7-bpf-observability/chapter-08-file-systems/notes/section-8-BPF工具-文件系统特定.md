# 8.3 BPF 工具：文件系统特定与预读取（8.3.21–8.3.27）

> 底本：《BPF之巅》第 8 章 文件系统，8.3 节后半（印刷 p341–353）

| 工具 | 来源 | 一句话 |
|---|---|---|
| xfsslower | BCC | XFS read/write/open/fsync 超阈值逐事件 |
| xfsdist | BCC/BT | 同上操作的延迟直方图 |
| ext4dist | BCC/BT | ext4 版 xfsdist（含经典 kprobe 兼容案例） |
| {ext4,btrfs,zfs,nfs}{slower,dist} | BCC | 其他文件系统同款（其他工具节） |
| icstat | BT | （已见上节）inode 缓存 |
| bufgrow | BT | 哪个进程在涨缓冲缓存 |
| readahead | BT | 预读取页是否被用、用了多及时 |

## xfsslower

```
# xfsslower            # 默认 10ms
COMM  T    BYTES  OFF_KB  LAT(ms)  FILE
java  R    63559  360237  59.44    shuffle_2649280.data
```

跟踪文件系统自己的 file_operations 函数（fs/xfs/xfs_file.c）：xfs_file_read_iter / xfs_file_write_iter / xfs_file_open / xfs_file_fsync。跟踪点贴近应用 ⇒ 测到的延迟很可能直接影响应用（与 fileslower 同理）。读写频繁的负载即使无超阈事件也有不可忽视开销。选项 `-P PID`、`min_ms`（0=全打，慎用）。

## xfsdist —— 读直方图的双峰解读 🔴

```
operation='read'
usecs           count
0 -> 1          5492    █████████
8 -> 15         4384    █████████
16 -> 31        7429    █████████     ← 双峰之一
128 -> 255      163
32768 -> 65535  821                    ← 存储队列等待尾
```

- 0–7µs 与 16–31µs 两峰**都来自页缓存**（差异可能是 I/O 尺寸或代码路径不同）；65µs+ 的尾部涉及存储设备与队列等待。
- write 集中在 16–31µs = 写回缓存模式的典型特征。
- 开销原因同 xfsslower；选项 `-m`（毫秒桶）、`-p PID`、`interval [count]`。
- bpftrace 版：kprobe/kretprobe 四个 xfs_file_* 函数 + `@us[func] = hist(...)`，XFS 的函数映射干净利落——下一节 ext4 恰好是反例。

## ext4dist —— 一堂 kprobe 脆弱性现场教学 🔴

XFS 教科书式顺利，ext4 却踩了两个坑（Linux 4.8 实录）：

1. **通用函数污染**：4.8 的 ext4 file_operations 里 `.read_iter = generic_file_read_iter`——直接跟踪它会混入所有使用该通用函数的文件系统。解决：跟踪通用函数但检查参数 `iocb->ki_filp->f_op == ext4_file_operations`（结构体地址在启动时从 /proc/kallsyms 读取）来过滤。代价：所有 generic 调用都过一遍探针。
2. **内核一改，探针就换**：Linux 4.10 给 ext4 加回了 `ext4_file_read_iter` 专属函数，过滤技巧作废，直接跟踪新函数即可。

教训：kprobe 工具绑定内核版本是常态不是意外；本书同一工具给 4.8 前后两套写法。

bpftrace 版（4.10+）：kprobe ext4_file_read_iter/write_iter/open + ext4_sync_file（fsync 对应 ext4_sync_file 而非 ext4_file_fsync），示范输出延迟全在 1ms 内。

## icstat / bufgrow / readahead

**bufgrow**：kprobe `add_to_page_cache_lru`，过滤 `i_mode & 0x6000`（块设备页）后 `@kb[comm] = sum(4)`。书例 dd 写块设备使缓冲缓存涨 ~100MB，free -wm 的 buffers 列同步印证。

**readahead** 🔴（Netflix SSD 生产问题的配套工具，详见 9 章 biosnoop）：

```
Readahead unused pages: 128
Readahead used page age (ms):
[2,4]  8424 | [8,16] 7680 | ... 大部分 <32ms
```

- unused pages：预读了但没人读——纯浪费的 I/O。
- used page age：预读页**多久后被真正读到**。若达秒级 ⇒ 预读过于激进，该调（读太早了页还可能被逐出）。
- SSD 上预读收益小于机械盘甚至为负（随机读快，预读挤占带宽）。
- 实现：do_page_cache_readahead 进/出设置线程本地标志；page_cache_alloc 返回值（页指针）为键记出生时间戳；mark_page_accessed 时算 age 入直方图；unused = 预读分配数 − 使用数。
- 开销：页缓存函数极高频 + 每页存元数据，繁忙系统可达 **30%**，仅限短期分析。

## 其他工具（8.3.27）

ext4slower、btrfsslower/btrfsdist、zfsslower/zfsdist、nfsslower/nfsdist（NFSv3/v4）——全部是 xfsslower/xfsdist 的同族移植。**先看延迟分布（dist），再下钻慢事件（slower）**的套路对所有文件系统通用。

## HFT 关联

- 延迟敏感盘选文件系统：xfsdist/ext4dist 直方图是选型依据（重点看 fsync 分布，订单落盘路径的 P99）。
- readahead 工具论证了"SSD 上盲目预读有害"——交易机数据盘可考虑调小 read_ahead_kb 并用本工具验证。

## 常见陷阱

- ext4dist 类工具换内核要重验函数名（4.8→4.10 案例即证明）。
- xfsdist 读双峰都在页缓存内，不要把 16–31µs 峰当成磁盘证据。

<details>
<summary>自测</summary>

1. Linux 4.8 上跟踪 ext4 读为什么要检查 f_op 指针？4.10 后为何不需要了？
2. readahead 的 used page age 直方图若集中在秒级，说明什么？
3. dist 与 slower 两个工具如何配合使用？
</details>
