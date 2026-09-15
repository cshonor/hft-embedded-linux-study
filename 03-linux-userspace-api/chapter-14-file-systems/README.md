# TLPI 第 14 章 — File Systems

**优先级**：🔴（路径解析、挂载、硬链接、持久化与 fsck/日志理解）
**前置**：[Ch13 File I/O Buffering](../chapter-13-file-io-buffering/README.md)（页缓存：本章的「磁盘」之下还有一层） · [Ch04 Universal I/O](../chapter-04-file-io-universal/README.md)（`read`/`write` 的语义） · [Ch12 `/proc`](../chapter-12-system-process-info/README.md)（伪文件的读法、命名空间隔离）
**后置**：[Ch15 File Attributes](../chapter-15-file-attributes/README.md)（`stat` 家族的完整字段） · [Ch18 Directories and Links](../chapter-18-directories-links/README.md)（目录数据块与 `link`/`unlink`） · [Ch19 Monitoring File Events](../chapter-19-monitoring-file-events/README.md)（inotify 为什么看不到某些改动）

---

有 13 节（与官方 TOC 一致），其中 **14.8 与 14.9 各带子编号**；按仓库约定，**同一节的子节收在同一篇里用 `###`**，不拆成多文件。

> ⚠️ **14.13 是单数 `Exercise`，全章只有一道题**。题面要求写一个「建 `NF` 个 1 字节文件 `xNNNNNN`、**随机序创建 + 递增序删除**」的程序，并在不同 `NF`（到 200000）与不同文件系统上试。本仓库的实现是 `code/ex14_1_file_churn.c`。

## 小节目录

- [14.1 Device Special Files 设备特殊文件](notes/14.1-device-special-files.md)
- [14.2 Disks and Partitions 磁盘与分区](notes/14.2-disks-and-partitions.md)
- [14.3 File Systems 文件系统](notes/14.3-file-systems.md)
- [14.4 I-nodes inode](notes/14.4-i-nodes.md)
- [14.5 The Virtual File System (VFS) 虚拟文件系统](notes/14.5-virtual-file-system-vfs.md)
- [14.6 Journaling File Systems 日志文件系统](notes/14.6-journaling-file-systems.md)
- [14.7 Single Directory Hierarchy and Mount Points 单一目录树与挂载点](notes/14.7-single-directory-hierarchy-mount-points.md)
- [14.8 Mounting and Unmounting File Systems 挂载与卸载](notes/14.8-mounting-and-unmounting.md)（含 14.8.1 `mount()` / 14.8.2 `umount()`）
- [14.9 Advanced Mount Features 高级挂载特性](notes/14.9-advanced-mount-features.md)（含 14.9.1~14.9.5）
- [14.10 A Virtual Memory File System: tmpfs 虚拟内存文件系统](notes/14.10-tmpfs.md)
- [14.11 Obtaining Information About a File System: statvfs() 获取文件系统信息](notes/14.11-statvfs.md)
- [14.12 Summary 小结](notes/14.12-summary.md)
- [14.13 Exercise 习题](notes/14.13-exercise.md)

---

## 章节目标

- **把「设备」与「文件系统」的分界线画出来**：`st_dev`（在哪个文件系统上）≠ `st_rdev`（是哪个设备）；`/proc/filesystems` 里带 `nodev` 前缀的文件系统（tmpfs/proc/sysfs）**不需要块设备**就能挂
- **`/proc/partitions` 的 `#blocks` 单位是 1 KiB**：依据不是「书上说」，是 `block/genhd.c:862` 的 `bdev_nr_sectors(part) >> 1`（扇区数右移一位）+ 数值自校验（`16777216 = 2^24` 只能是 16 GiB 的 KiB 数）
- **分区名的那个 `p` 是 `isdigit()` 决定的**：`block/partitions/core.c:349-352`。所以 `nvme0n1p1`、`mmcblk0p1`、`loop0p1` 同源，而 `sda1` 没有 —— 「NVMe 专有」是错的
- **inode 号必须配 `st_dev`**：本机 `/` 与 `/proc` 的 `st_ino` **都是 1**，但它们不是同一个对象
- **`unlink()` 不释放数据**：释放条件是 `nlink == 0` **且** 没有打开的 fd。本机 `/etc/passwd` 就是一个活着的 `nlink=0`（单文件 bind 自「已 unlink 的源」）
- **`st_size`（逻辑）≠ `st_blocks × 512`（物理）**：写 1 字节 → `st_size=1, st_blocks=8`（占 4096）；稀疏文件 → `st_size=1048577, st_blocks=8`
- **`struct file` 才是「一次打开」**：`open` 两次 → 偏移独立；`dup` → 偏移共享。判据是 `struct file`，**不是 inode**
- **日志 ≠ 数据持久**：`write()` 之后 jbd2 事务计数**不变**，`fsync()` 之后 **+1**（实测 590 → 590 → 591 → 592）
- **ext4 数据区小端、jbd2 日志区大端**（`Documentation/filesystems/ext4/overview.rst:15-17` 逐字）
- **`/proc/self/mountinfo` 是唯一权威判据**：不是 `/etc/mtab`（不存在）、不是 `/proc/mounts`（是符号链接且字段少）、不是「目录是否存在」
- **bind mount 的判据是 `root != /`**：本机 **16 条** bind，其中 `/lib` 与 `/usr/lib` 的 `st_ino` 相同（同一棵子树投两次）
- **`statvfs()` 不是系统调用**：它是 glibc 对 `statfs()` 的包装（`statvfs64.c:28-38`）。`f_flag = f_flags ^ 0x20`（`ST_VALID`），`f_favail == f_ffree`（glibc 源码里自认「不会算」）
- **纠正本仓库旧笔记的两处错误**：① 「`f_type` 只有 `statfs` 有」是错的（`bits/statvfs.h:54` 就有）；② 「`st_dev` 与 `mountinfo` 的 `major:minor` 语义不同、不能比较」是错的（实测同台机器两边都是 `259:2`）
- **诚实**：本章所有 glibc 2.39 / Linux v6.6 / man-pages 6.19 的坐标都是实读核准的；并明确标注 CE 沙箱**做不到**的事（无 `CAP_SYS_ADMIN` ⇒ `mount`/`umount2` 一律 `EPERM`；无块设备 ⇒ `dumpe2fs`/`fsck`/`mkfs` 全不可用；无 swap ⇒ 测不出 tmpfs 换出；`/sys` 未挂 sysfs）
- **溯源**：本章 **13 篇**笔记里的每一行实测输出都能在 CE 冻结日志 `tlpi-ch14-final.txt` 里定位

### 一条主线：本章只有「三层抽象」和它们各自的「身份证」

| 层 | 描述它的是 | 换一个说法 | 容易搞错的地方 |
|----|-----------|-----------|--------------|
| **设备** | `dev_t`（`major:minor`） | 「这块盘是几号」 | 同一个 major 上可能同时有整盘和分区（本机 259） |
| **文件对象** | `st_dev` + `st_ino` | 「这个文件住哪儿」 | 只看 `st_ino` 会误判（`/` 与 `/proc` 都是 1） |
| **文件系统实例** | `fsid`（`statfs`） | 「是不是同一个卷」 | `f_type` 相同 ≠ 同一个实例（`/`、`/dev`、`/tmp` 都是 tmpfs 但 fsid 不同） |
| **挂载关系** | `mountinfo` 的 `root` / optional fields | 「谁挂在哪儿、怎么挂的」 | 「目录存在」≠「已挂载」（`/sys`） |

一句话：**本章没有新概念，只有「同一个对象有四种身份证」这个事实，以及用错身份证带来的全部后果。**

---

## 原书示例清单（man7 官方按章文件列表）

man7.org 的 TLPI 页面在 Ch14 下**只分发 5 个文件**，全部在 `filesys/` 目录：

| 官方文件 | 出处 | 本仓库位置 | 说明 |
|---------|------|-----------|------|
| `t_mount.c` | **Listing 14-1, page 268** | `code/t_mount.c` | 把 `MS_*` 字母翻译成标志再调 `mount(2)` |
| `t_umount.c` | 未印刷（未进正文 Listing） | `code/t_umount.c` | `umount` / `umount2` 的最小演示 |
| `t_statfs.c` | 未印刷 | `code/t_statfs.c` | 逐字段打印 `struct statfs` |
| `t_statvfs.c` | 未印刷 | `code/t_statvfs.c` | 逐字段打印 `struct statvfs` |
| `overlayfs_example.sh` | 标注为 "supplementary file for Chapter 14" | `code/overlayfs_example.sh` | **本章唯一的非 C 文件**（shell 脚本） |

> ⚠️ 这 5 个文件用到 TLPI 的公共头 `lib/tlpi_hdr.h`（及其依赖 `ename.c.inc` / `get_num.c`）。本仓库用一个**替身** `code/tlpi_hdr.h` 代替，改动与差异逐条列在 [`code/README.md`](code/README.md) 的「替身差异」表里。
>
> ⚠️ **`t_mount.c` / `t_umount.c` 的输出全在 stderr**（`errExit()`/`usageErr()` 写 stderr）。本仓库的笔记里引用它们时，代码块的内容取自 **stderr**，用 `2>/dev/null` 什么都看不到。

---

## 易错清单

1. **把 `/proc/partitions` 的 `#blocks` 当扇区数** —— 单位是 **1 KiB**（`block/genhd.c:862` 的 `>> 1`）
2. **以为分区名带 `p` 是 NVMe 专有** —— 判据是「父设备名末字符是否数字」（`block/partitions/core.c:349`）
3. **以为「一个驱动器 = 一个 major」** —— 分区号超出 `disk->minors` 窗口会落到 `BLOCK_EXT_MAJOR`（实测 **259**）
4. **以为 `BLOCK_EXT_MAJOR` 只给分区用** —— 驱动不给 `major` 时（如 NVMe）**整盘也从这里取号**，本机 `nvme0n1 = 259:0`
5. **拿 `major:minor` 去 `/proc/partitions` 查不到就以为代码错** —— 容器可以 bind/换命名空间，查不到是正常的
6. **跨作业/跨宿主机比较设备号** —— 本轮是 `nvme`（259）、另一作业是 `xvda`（202）。**只在同一次运行内比较**
7. **以为「块」= 512 字节** —— **扇区**才是 512；文件系统的块是 `f_bsize`（ext4 常见 4096）
8. **以为 inode 表可以事后扩容** —— `f_files` 在 `mkfs` 时定死，`tune2fs` 只能看不能加
9. **单独用 `st_ino` 判断「是不是同一个文件」** —— 必须配 `st_dev`（本机 `/` 与 `/proc` 的 `st_ino` 都是 1）
10. **以为 `unlink()` 立刻释放空间** —— 要 `nlink == 0` **且** 无打开的 fd。这是「`df` 满而 `du` 不认」的第一嫌疑
11. **以为 `st_ctime` 是创建时间** —— 它是 inode 变更时间；真正的创建时间要 `statx` 的 `stx_btime`
12. **用 `ls -l` 的字节数估磁盘占用** —— 那是 `st_size`（逻辑）；物理占用看 `st_blocks × 512`
13. **以为 `st_blocks` 单位是 `st_blksize`** —— 单位固定 **512 字节**
14. **以为 `dup()` 会复制偏移** —— `dup` 后两个 fd 共享**同一个 `struct file`**，偏移一起动
15. **以为两次 `open()` 会共享偏移** —— 各自独立的 `struct file`
16. **以为同一 inode 的两次打开会合并** —— 判据是 `struct file`，不是 inode
17. **以为 `fcntl(F_GETFL)` 返回 0 就是没标志** —— `O_RDONLY` 就是 `0`。某机实测 `0x8000` 只有 `O_LARGEFILE`
18. **以为磁盘上有 dentry** —— dentry 是**纯内存**（dcache），磁盘上只有目录数据块里的 `(名字, inode号)`
19. **以为「有日志」=「数据不会丢」** —— 日志保**结构一致**；数据要 `fsync`/`fdatasync`
20. **以为 `write()` 会产生 jbd2 事务** —— 实测「未 `fsync` 时计数 +0」
21. **把 `/proc/fs/jbd2/*/info` 的绝对值当断言** —— 它是**整机累计**值（本轮 590、上一轮 348）。只看增量方向
22. **按小端解析 jbd2 的磁盘结构** —— **日志区是大端**（`ext4/overview.rst:15-17`）
23. **以为 ext4 必然有日志** —— `mke2fs -O ^has_journal` 可以没有。判据是 `/proc/fs/jbd2/<dev>-<n>/` 在不在
24. **用 `/etc/mtab` 判断挂载** —— 本容器**不存在**；`/proc/mounts` 是**指向 `self/mounts` 的符号链接**
25. **以为「目录存在」=「文件系统已挂」** —— `/sys` 存在但 sysfs 没挂（`/sys` 与 `/` 的 `fsid` 相同）
26. **拿 `mounts` 第 1 列当超级块设备号** —— 那是**源设备**号；bind mount 上两者不同
27. **以为「同一设备出现多次」就是 bind mount** —— 判据是 `root != /`
28. **以为卸载后挂载点原内容没了** —— 只是被**遮盖**，卸载后恢复
29. **以为 `source` 是个可用路径** —— 本机是 `/dev/root`，而 `/dev` 下没有块设备节点
30. **以为 `/dev` 与 `/dev/null` 是同一个文件系统** —— `/dev` 是 tmpfs、`/dev/null` 是 devtmpfs（`st_dev` 不同）
31. **以为容器里 UID 0 就能 `mount`** —— 看 `/proc/self/status` 的 `CapEff`（本机全 0）
32. **以为 `mount()` 报 `EPERM` 说明参数错了** —— 恰好相反：说明参数**已通过检查**（权限门在 fstype 解析之前）
33. **拿 `MS_NOSUID(2)` 去匹配 `MNT_NOSUID(0x01)`** —— 四套位值属于不同的层
34. **以为 `umount` 失败第一位是「目标不存在」** —— 常见第一位是 `EBUSY`（本容器是 `EPERM`）
35. **把 `MNT_FORCE` 当万能强制卸载** —— 只有部分 FS 支持且可能丢数据；要「先摘名后释放」用 `MNT_DETACH`
36. **以为 `nosuid`/`nodev`/`noexec` 是超级块属性** —— 它们是 **per-mount**；`size=`/`nr_inodes=` 才是 super 级
37. **以为 `mountinfo` 里只有一处 `ro`/`rw`** —— **两处**：per-mount 栏与 super 栏（本机 `/` 是 per-mount `ro` + super `rw`）
38. **拿 `/proc/mounts` 去还原挂载树** —— 它没有 `mount ID`/`parent ID`，画不了树
39. **把 `master:N` 的 N 写进断言** —— 运行时分配的组 ID，每次运行不同
40. **把 tmpfs 当磁盘估容量** —— 没写 `size=` 时上限是 `totalram_pages()/2`
41. **只看 `df` 判断 tmpfs 是否写满** —— **inode 也可能是瓶颈**（本机 `/tmp` 只有 100 个 inode）
42. **以为 `f_type` 相同就是同一个文件系统** —— 必须比 **`fsid`**
43. **以为 `statvfs()` 是独立系统调用** —— 它是 glibc 对 `statfs()` 的包装
44. **说「`f_type` 只有 `statfs` 有」** —— 错。`bits/statvfs.h:54` 就有 `unsigned int f_type`
45. **把 `statfs.f_flags` 直接当挂载标志** —— 它含 `ST_VALID(0x20)`；glibc 的 `f_flag` 是 `^ 0x20` 之后的结果
46. **拿 `f_flag & ST_VALID` 判断内核支持度** —— 恒为 0
47. **信 `f_favail`** —— Linux 上它 == `f_ffree`（glibc 自己写着「不会算」）。**`f_bavail` 才是真值**
48. **算容量用 `f_bsize`** —— 该用 **`f_frsize`**
49. **对 `/proc` 问「还剩多少空间」** —— procfs 的 `f_blocks`/`f_files` 都是 **0**
50. **把 `f_fsid` 当 `st_dev` 或持久标识** —— 它是「这一次挂载的随机标识」（procfs 的 `0x50` 是例外）

---

## 章节链路

```text
        应用 read()/write()/stat()
                │
                ▼
     ┌──────────────────────────┐
     │  VFS（14.5）             │  superblock / inode / dentry / file
     │  统一接口，四类对象       │  用户态投影：statfs / stat / fd
     └───────────┬──────────────┘
                 │  路径解析 → dentry → inode
                 ▼
     ┌──────────────────────────┐
     │  具体文件系统             │
     │  ext4（14.3/14.4/14.6）   │  超级块 + 块组 + inode 表 + 数据块
     │  tmpfs（14.10，nodev）    │  页缓存/匿名页，容量 = 内存的一半
     │  proc/sysfs/devtmpfs      │  「伪文件系统」，无容量语义
     └───────────┬──────────────┘
                 │  sb->s_dev
                 ▼
     ┌──────────────────────────┐
     │  块层 → 设备              │
     │  分区（14.2）             │  major:minor，blkext(259)
     │  字符设备（14.1）         │  st_rdev
     └──────────────────────────┘

  挂在哪儿、怎么挂的：14.7（单一目录树）/ 14.8（mount/umount）/ 14.9（bind/传播）
  问「还剩多少」：14.3（statfs）/ 14.11（statvfs = statfs 的包装）
```

---

## 双线提示

| 线 | 本章拿什么去用 |
|----|--------------|
| **HFT** | ①「日志盘与数据盘分开」的判据是 `st_dev` 不同，**不是目录名不同**；②一次 `fsync` = 一笔 jbd2 事务，所以「批量写 + 一次 `fsync`」永远优于「逐条 `fsync`」；③`df` 满而 `du` 不认 ⇒ 查「已被 unlink 但还有 fd 打开」的文件；④设备号在云上不可预测，任何硬编码 `/dev/sda1` 都会翻车；⑤inode 预算要在 `mkfs` 阶段按文件数反算 |
| **嵌入式** | ①最小 rootfs = 只读 squashfs + `/dev` devtmpfs + `/proc` + `/tmp`(tmpfs) + 一个可写分区；②`/tmp` 必须显式写 `size=`/`nr_inodes=`，否则写飞就 OOM；③eMMC 上块组布局被 FTL 打散，「集中元数据」的收益与机械盘不同；④精简内核若没编 `CONFIG_EFI_PARTITION`，GPT 盘会「看起来没有分区」；⑤`nodev` 文件系统不需要设备节点，日志与数据分盘的做法要按 `st_dev` 判定 |
| **两条线共同的坑** | `mount` 只在启动阶段做（要 `CAP_SYS_ADMIN`）；`/proc` 下的一切都**没有命名空间隔离**（`/proc/partitions`、`/proc/devices` 看到的是宿主）；所有「设备号 / 容量 / 计数 / 耗时」都是**环境相关**，只能在同一进程的同一次运行内比较 |

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | `st_dev` = 在哪个 FS 上；`st_rdev` = 是哪个设备 |
| 2 | `/proc/partitions` 的 `#blocks` 单位是 **1 KiB** |
| 3 | 分区名的 `p`：父设备名末字符是数字就插 |
| 4 | `BLOCK_EXT_MAJOR` = **259**（`blkext`），整盘与溢出分区共用 |
| 5 | `nodev` 文件系统（tmpfs/proc/sysfs）不需要块设备 |
| 6 | 超级块 → `statfs`；inode → `stat`；dentry 只在内存 |
| 7 | inode 号只在同一 `st_dev` 内唯一 |
| 8 | 释放数据：`nlink == 0` 且无打开的 fd |
| 9 | `st_blocks` 单位固定 512 字节；`st_size` 是逻辑大小 |
| 10 | 文件名在**目录数据块**里，不在 inode 里 |
| 11 | 硬链接 = 目录里多一条名字；跨 FS → `EXDEV(18)` |
| 12 | `open` 两次 = 两个 `struct file`（偏移独立） |
| 13 | `dup` = 同一个 `struct file`（偏移共享） |
| 14 | 日志保结构一致，`fsync` 保数据 |
| 15 | `write` 不产生 jbd2 事务，`fsync` 让计数 +1 |
| 16 | ext4 数据小端、jbd2 日志**大端** |
| 17 | `/proc/self/mountinfo` 是唯一权威挂载判据 |
| 18 | `mountinfo` 的 `root != /` ⇒ bind mount |
| 19 | `mountinfo` 有两栏选项：per-mount 与 super |
| 20 | `master:N` = 挂载传播（slave）标记 |
| 21 | 挂载遮盖：卸载后原内容恢复 |
| 22 | `mount`/`umount2` 要 `CAP_SYS_ADMIN`；看 `CapEff` |
| 23 | 四套位值：`MS_*` / `MNT_*` / `ST_*` / `MOUNT_ATTR_*` |
| 24 | tmpfs 上限 = `totalram_pages()/2`（无 `size=` 时） |
| 25 | `size=` ↔ `f_blocks × f_frsize`；`nr_inodes=` ↔ `f_files` |
| 26 | `fsid` 才是「同一个超级块」的判据 |
| 27 | `statvfs` 是 glibc 的包装，不是系统调用 |
| 28 | `f_flag = statfs.f_flags ^ 0x20`（`ST_VALID`） |
| 29 | `f_favail == f_ffree`（Linux 上算不出来） |
| 30 | `f_bavail` 是真值；ext4 保留 5% 给 root |

---

## 参考

- Kerrisk · TLPI Ch14（专有名词：single directory hierarchy / mount point / inode / superblock）
- man-pages 6.19：`man 2 statfs` · `man 3 statvfs` · `man 2 mount` · `man 2 umount2` · `man 2 mknod` · `man 2 link` · `man 2 stat` · `man 5 proc`（`mountinfo` 的字段定义） · `man 7 mount_namespaces` · `man 5 tmpfs` · `man 5 ext4` · `man 3 major`
- Linux v6.6 源码：`block/genhd.c` · `block/partitions/core.c` · `fs/statfs.c` · `fs/proc_namespace.c` · `fs/namespace.c` · `fs/open.c` · `fs/jbd2/journal.c` · `mm/shmem.c` · `fs/ext4/sysfs.c` · `include/linux/kdev_t.h` · `include/linux/blkdev.h` · `include/uapi/linux/mount.h` · `include/linux/mount.h`
- Linux v6.6 自带文档：`Documentation/filesystems/ext4/overview.rst` · `blockgroup.rst` · `inodes.rst` · `tmpfs.rst` · `sharedsubtree.rst` · `vfs.rst`
- glibc 2.39：`sysdeps/unix/sysv/linux/statvfs64.c` · `statvfs.c` · `internal_statvfs.c` · `bits/statvfs.h`
- 实测环境：Compiler Explorer 公开 API（gcc 13.3.0 / x86-64 / Ubuntu 24.04），冻结日志 `tlpi-ch14-final.txt`

---

## 代码示例

本章 `code/` 下有 **18 个文件**（12 个自编 demo + 5 个原书镜像 + 1 个替身头文件）：

### 自编 demo（12 个）

| 文件 | 对应节 | 演示什么 |
|------|--------|---------|
| `code/c14_1_device_files.c` | 14.1 | `dev_t` 编码、`st_dev` vs `st_rdev`、扫 `/dev`、`/proc/devices`、`mknod`、`/dev/null` vs `/dev/zero` |
| `code/c14_2_disks_partitions.c` | 14.2 | `/proc/partitions` 的 KiB 单位、`st_dev` 反查分区、按 major 分组、`/proc/devices` 块段、`/sys/dev/block` 为什么不存在 |
| `code/c14_3_fs_layout.c` | 14.3 | `statfs` 字段 = 超级块投影、`/proc/fs/ext4`、`/proc/fs/jbd2`、块组数推算 |
| `code/c14_4_inode.c` | 14.4 | inode 号定域、硬链接、`EXDEV`、unlink 后 fd 仍可读、`st_size` vs `st_blocks` |
| `code/c14_5_vfs.c` | 14.5 | VFS 四对象的用户态投影、`open`×2 / `dup` / 硬链接的三组偏移对照 |
| `code/c14_6_journaling.c` | 14.6 | jbd2 计数与 `fsync`、jbd2 大端、日志的边界 |
| `code/c14_7_mount_points.c` | 14.7 | `mountinfo` 逐字段、全部挂载点、同设备多次、`st_ino` 相同、挂载遮盖、`st_dev` 核对 |
| `code/c14_8_mount_umount.c` | 14.8 | `CapEff`、`mount(2)` 六种调用、`umount2` 四个 flag、常量表、`MS_*` vs `MNT_*` |
| `code/c14_9_advanced_mount.c` | 14.9 | optional fields、同设备多点、`root != /`、per-mount vs super、试 bind |
| `code/c14_10_tmpfs.c` | 14.10 | `/proc/filesystems`、`/tmp` 的 mountinfo、`size=` 对账、`fsid` 判重、默认上限、块与 inode 两条线 |
| `code/c14_11_statvfs.c` | 14.11 | 结构体尺寸/偏移、`statfs` vs `statvfs` 逐字段、`f_flag` 分解、`^0x20` 实证、`f_favail`、`f_fsid` 重组 |
| `code/ex14_1_file_churn.c` | 14.13 | 习题实现：`NF` 个 1 字节文件、随机序创建 + 递增序删除、inode/块记账 |

### 原书镜像件（5 个，逐字取自 man7 官方）

| 文件 | 出处 |
|------|------|
| `code/t_mount.c` | **Listing 14-1, page 268** |
| `code/t_umount.c` | 未印刷 |
| `code/t_statfs.c` | 未印刷 |
| `code/t_statvfs.c` | 未印刷 |
| `code/overlayfs_example.sh` | supplementary file for Chapter 14（**shell 脚本，不参与编译**） |

### 替身（1 个）

| 文件 | 说明 |
|------|------|
| `code/tlpi_hdr.h` | 代替原书的 `lib/tlpi_hdr.h`（含 `ename.c.inc`/`get_num.c` 的必要部分）。差异见 [`code/README.md`](code/README.md) |

编译与运行（在 `code/` 目录下）：

```bash
# 自编 demo（每个都是独立程序）
gcc -O0 -Wall -Wextra -o c14_1_device_files c14_1_device_files.c && ./c14_1_device_files
gcc -O0 -Wall -Wextra -o c14_2_disks_partitions c14_2_disks_partitions.c && ./c14_2_disks_partitions
gcc -O0 -Wall -Wextra -o c14_10_tmpfs c14_10_tmpfs.c && ./c14_10_tmpfs
gcc -O0 -Wall -Wextra -o ex14_1_file_churn ex14_1_file_churn.c && ./ex14_1_file_churn ./testdir 2000

# 原书镜像件
gcc -O0 -Wall -Wextra -o t_mount t_mount.c
gcc -O0 -Wall -Wextra -o t_umount t_umount.c
gcc -O0 -Wall -Wextra -o t_statfs t_statfs.c && ./t_statfs /app
gcc -O0 -Wall -Wextra -o t_statvfs t_statvfs.c && ./t_statvfs /app
```

### ⚠️ 会漂的数字（不要写进断言）

| 量 | 观察到的变化 |
|----|-------------|
| 设备号（`st_dev`、`/proc/partitions`） | 本轮 `259:1`/`259:2`；另一作业 `202:1`；更早一轮整盘是 `xvda`/`202:x` |
| `/proc/partitions` 条目数 | 29 ~ 31 |
| `/proc/devices` 块设备 major 数 | 上一轮被截断成 11；本轮 23 |
| `/proc/self/mountinfo` 条数 | 61 / 62 / 65 / 66（不同作业不同宿主） |
| `f_ffree` / `f_bfree` / `f_bavail` | 宿主上别的容器也在写 |
| `f_fsid` | 每次挂载随机生成（procfs 的 `0x50` 是例外） |
| jbd2 的 `N transactions` 首行 | 上一轮 348、本轮 590 —— **只能看增量** |
| `/cefs/**` 的具体名字、`master:N` 的 N | 编译器镜像滚动更新 |
| 所有耗时（`us/个`） | 不同宿主、不同负载 |

### 相对稳定（可以引用）

| 量 | 值 |
|----|----|
| `f_type` magic | ext4 `0xef53`、tmpfs `0x01021994`、proc `0x9fa0` |
| `MS_*` / `MNT_*` / `ST_*` / `MOUNT_ATTR_*` | 见 14.8 第 4 节 / 14.11 |
| `sizeof(struct statfs)` / `statvfs` | `120` / `112`（x86-64 / glibc 2.39） |
| `ST_VALID` | `0x0020` |
| `MINORBITS` / `BLKDEV_MAJOR_MAX` | `20` / `512` |
| `BLOCK_EXT_MAJOR` | `259` |
| ext4 块组大小 | `8 × block_size`（4 KiB → 128 MiB） |
| 增量方向 | `fsync` 让 jbd2 +1；`write` 不产生事务 |

---

## 与前后章

| | 章 | 关系 |
|--|----|------|
| ← 前置 | [Ch13 File I/O Buffering](../chapter-13-file-io-buffering/README.md) | 页缓存是「磁盘」之上那一层；本章的 `fsync` 就是它讲的持久化入口 |
| ← 前置 | [Ch12 System and Process Information](../chapter-12-system-process-info/README.md) | `/proc` 下伪文件的读法、命名空间隔离 |
| → 后置 | [Ch15 File Attributes](../chapter-15-file-attributes/README.md) | `stat` 家族的完整字段（`st_dev`/`st_ino` 的出处） |
| → 后置 | [Ch18 Directories and Links](../chapter-18-directories-links/README.md) | 目录数据块的实际格式、`link`/`unlink`/`rename` |
| → 后置 | [Ch19 Monitoring File Events](../chapter-19-monitoring-file-events/README.md) | inotify 看不到哪些改动（元数据 vs 数据） |
| → 后置 | [Ch49 Memory Mappings](../chapter-49-memory-mappings/README.md) | 同一份页缓存的另一种映射方式 |
