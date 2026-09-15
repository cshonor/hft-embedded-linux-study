# Ch14 代码索引 — File Systems

本目录有 **18 个文件**，分三类：**12 个自编 demo** + **5 个原书镜像件** + **1 个替身头文件**。

所有自编 demo 都在 Compiler Explorer 上实跑过（gcc 13.3.0 / x86-64 / Ubuntu 24.04），输出与笔记逐字一致。

---

## 一、自编 demo（12 个）

| 文件 | 行数 | 对应节 | 演示什么 | 沙箱里能不能跑出结论 |
|------|------|--------|---------|-------------------|
| `c14_1_device_files.c` | 211 | 14.1 | `dev_t` 编码、`st_dev` vs `st_rdev`、扫 `/dev`、`/proc/devices`、`mknod`、`/dev/null` vs `/dev/zero` | ✅ 全部可跑（`mknod(S_IFCHR)` 会 `EPERM`，`S_IFIFO` 成功 —— 这本身是结论） |
| `c14_2_disks_partitions.c` | 251 | 14.2 | `/proc/partitions` 的 KiB 单位、`st_dev` 反查分区、按 major 分组、`/proc/devices` 块段、`/sys/dev/block` 为什么不存在 | ✅ 全部可跑 |
| `c14_3_fs_layout.c` | 197 | 14.3 | `statfs` = 超级块投影、`/proc/fs/ext4`、`/proc/fs/jbd2`、块组数推算 | ⚠️ ④ 段只能**推算**（无块设备，`dumpe2fs` 不可用） |
| `c14_4_inode.c` | 208 | 14.4 | inode 号定域、硬链接、`EXDEV`、unlink 后 fd 仍可读、`st_size` vs `st_blocks` | ✅ 全部可跑（`EXDEV` 也能拿到真实 errno） |
| `c14_5_vfs.c` | 192 | 14.5 | VFS 四对象的用户态投影、`open`×2 / `dup` / 硬链接的三组偏移对照 | ✅ 全部可跑 |
| `c14_6_journaling.c` | 199 | 14.6 | jbd2 计数与 `fsync`、jbd2 大端、日志的边界 | ✅ 可跑（计数是**整机累计**，只看增量） |
| `c14_7_mount_points.c` | 258 | 14.7 | `mountinfo` 逐字段、全部挂载点、同设备多次、`st_ino` 相同、挂载遮盖、`st_dev` 核对 | ✅ 全部可跑 |
| `c14_8_mount_umount.c` | 136 | 14.8 | `CapEff`、`mount(2)` 六种调用、`umount2` 四个 flag、常量表、`MS_*` vs `MNT_*` | ⚠️ 所有 `mount`/`umount` 调用**必然 `EPERM`**（无 `CAP_SYS_ADMIN`），但**失败顺序**有教学价值 |
| `c14_9_advanced_mount.c` | 214 | 14.9 | optional fields、同设备多点、`root != /`、per-mount vs super、试 bind | ⚠️ ⑤ 段必然 `EPERM`；前四段是「在已有挂载表里把这些特性**认出来**」 |
| `c14_10_tmpfs.c` | 404 | 14.10 | `/proc/filesystems`、`/tmp` 的 mountinfo、`size=` 对账、`fsid` 判重、默认上限、块与 inode 两条线 | ⚠️ ⑦ 段两件事**测不了**（无重启、无 swap），只标注官方文档依据 |
| `c14_11_statvfs.c` | 345 | 14.11 | 结构体尺寸/偏移、`statfs` vs `statvfs` 逐字段、`f_flag` 分解、`^0x20` 实证、`f_favail`、`f_fsid` 重组 | ⚠️ ⑥ 段的 `f_frsize ?:` 回退分支**不可达**（v6.6 一律填值） |
| `ex14_1_file_churn.c` | 319 | 14.13 | 习题实现：`NF` 个 1 字节文件、随机序创建 + 递增序删除、inode/块记账 | ✅ 可跑；建议目标目录先用**独立子目录**（中途 `ENOSPC` 会留残骸） |

---

## 二、原书镜像件（5 个，逐字取自 man7 官方）

| 文件 | 行数 | 出处 | 备注 |
|------|------|------|------|
| `t_mount.c` | 164 | **Listing 14-1, page 268** | 把 `MS_*` 字母翻译成标志再调 `mount(2)`；输出在 **stderr** |
| `t_umount.c` | 32 | 未印刷 | `umount`/`umount2` 最小演示；输出在 **stderr** |
| `t_statfs.c` | 53 | 未印刷 | 逐字段打印 `struct statfs` |
| `t_statvfs.c` | 50 | 未印刷 | 逐字段打印 `struct statvfs` |
| `overlayfs_example.sh` | 36 | "supplementary file for Chapter 14" | **shell 脚本，不参与编译**；演示 overlayfs 的多层合并语义 |

官方分发地址形如 `https://man7.org/tlpi/code/online/dist/filesys/<name>`，本目录下的内容与之一致（未做任何改写）。

### 编译与运行

```bash
# 原书 C 程序（t_*.c 都要 -I. 才找得到替身的 tlpi_hdr.h）
gcc -O0 -Wall -Wextra -I. -o t_mount t_mount.c
gcc -O0 -Wall -Wextra -I. -o t_umount t_umount.c
gcc -O0 -Wall -Wextra -I. -o t_statfs t_statfs.c
gcc -O0 -Wall -Wextra -I. -o t_statvfs t_statvfs.c

./t_mount                      # 打用法（在 stderr）
./t_mount -t tmpfs none /mnt   # 无特权时：mount: Operation not permitted
./t_statfs /app
./t_statvfs /app

# overlayfs 示例（要 root + 内核支持 overlay）
sh overlayfs_example.sh /tmp/ovl-test
```

> ⚠️ **`t_mount` / `t_umount` 的输出全在 stderr**。用 `./t_mount 2>/dev/null` 会「什么都看不到」。本仓库笔记里引用它们时，代码块内容取自 **stderr**。

---

## 三、替身（1 个）

| 文件 | 说明 |
|------|------|
| `tlpi_hdr.h` | 代替原书的 `lib/tlpi_hdr.h`（原书结构是「头文件声明 + `error_functions.c` 实现」，这里为单文件可控把实现做成 `static inline` 放在头里） |

### 替身差异表（写在这里免得读者对不上输出）

| # | 差异 | 影响 |
|---|------|------|
| 1 | **`errExit` 的报文格式**：原书打 `ERROR [EPERM Operation not permitted] mount`（靠 `lib/ename.c.inc` 的 errno 符号表把数字翻成助记名）；替身没有这张表，退化成 `mount: Operation not permitted` | ⚠️ **本章一定会看到**：`t_mount`/`t_umount` 在容器里必然失败，所以实测输出是 `mount: Operation not permitted` 而不是 `ERROR [EPERM ...] mount`。笔记 §14.8 已标注 |
| 2 | 只提供 `errExit` / `fatal` / `usageErr` / `cmdLineErr` 四个；原书的 `errMsg` / `err_exit` / `errExitEN` / `terminate` **未提供** | 本章四个程序用不到 |
| 3 | `usageErr` 的提示文案（`"Usage: "`）与真实实现**逐字相同**；`cmdLineErr` 是 `"Command-line usage error: "` | `t_statfs`/`t_statvfs`/`t_umount` 在 `argc` 不对时都调 `usageErr`，必须一致 —— 已核对 |
| 4 | 原书 `lib/tlpi_hdr.h` 里的 `Boolean`/`TRUE`/`FALSE`/`max()`/`socklen_t` 兜底/`O_ASYNC` 兜底等可移植性补丁**不提供** | 本章四个程序都不用 |
| 5 | **不带 `get_num.h`、不带 `min`/`max` 宏** | 这是与 **Ch13 替身**的关键差别：Ch13 的四个原书程序用了 `min(m,n)` 与 `getLong()`，那一版替身必须带上；Ch14 的四个程序**一个都不用**（`t_mount.c` 的短选项解析全走 `getopt(3)`）。**本目录下没有 `get_num.h` 是有意的，不是漏了** |

> ⚠️ **与 Ch04 / Ch05 / Ch10 / Ch11 / Ch12 / Ch13 的同名替身不通用**，别跨章复制。

---

## 四、沙箱（Compiler Explorer 容器）的硬限制

本目录的 demo 全部在 CE 上跑过。容器里**做不到**的事，笔记里都做了诚实标注，不要把它们当成「代码写错了」：

| 限制 | 观察到的事实 | 受影响的 demo |
|------|-------------|--------------|
| **无 `CAP_SYS_ADMIN`** | `/proc/self/status` 的 `CapPrm`/`CapEff` 全 0，`NoNewPrivs: 1` | `c14_8`（全部 `mount`/`umount2` → `EPERM`）、`c14_9` ⑤ 段 |
| **无 `CAP_MKNOD`** | `mknod(S_IFCHR)` → `EPERM`；但 `mknod(S_IFIFO)` **成功** | `c14_1` ⑤ 段 |
| **无块设备** | `/dev` 下只有 4 个字符设备 + 5 个 `nvidia*` 普通文件，块设备 **0 个** | `c14_3` ④ 段（只能算组数）、`c14_2`（只能看 `/proc` 表） |
| **`/sys` 未挂 sysfs** | `/sys/dev`、`/sys/block` → `ENOENT(2)`；`/` 与 `/sys` 的 `fsid` **相同** | `c14_2` ⑥ 段、`c14_10` ④ 段 |
| **无 swap** | `/proc/meminfo` 的 `SwapTotal` 为 0 | `c14_10` ⑦ 段（换出测不了） |
| **不能重启** | — | `c14_10` ⑦ 段（「掉电即失」测不了） |
| **`/etc/mtab` 不存在**；`/proc/mounts` 是**符号链接** → `self/mounts` | 只能用 `/proc/self/mountinfo` | `c14_7` |
| **`RLIMIT_FSIZE` 与 `/tmp` 的 `nr_inodes=100`** | 在 `/tmp` 上建超过 99 个文件 → `ENOSPC` | `ex14_1_file_churn.c`（本机用 `/tmp` 跑 `NF=200` 会中途停） |
| **CE 每个作业是独立容器，宿主可能不同** | 设备号（`nvme` vs `xvda`）、`mountinfo` 条数（61~66）、`f_ffree` 全都会变 | 全部 demo：**只在同一次运行内比较** |
| **`output.s` 是 CE 给产物起的名字** | `usageErr` 用 `argv[0]`，所以用法行显示 `./output.s` | `t_mount`（`c14_8` §5.1 已说明） |

---

## 五、文件校验（sha256）

```text
6cfa29ff960bb6de858d37355bd85ceda4b59594793e4b9430597a1cab0d3c03  c14_10_tmpfs.c
5482e7581920b9e22e81cb2ec7c0b6a86a19d6fa21906fae12169630ea34433a  c14_11_statvfs.c
2e5e28520aa0815184e38e6c1301bd9911d049453f3cd10776e91cffb0b3cc64  c14_1_device_files.c
3bd2598b28bde9fdc79a705d83b963675a0dcd21578e7fb47eff13ab9c0b2dee  c14_2_disks_partitions.c
8e9bf2c189e9f662a77b379c3f5d0a6540f509b545b669ec44d91abf1601341e  c14_3_fs_layout.c
6226722feec273b29102340eb47d19478a9780f68b761b06c5f8b53a19e33d5a  c14_4_inode.c
74ed2191446ddf2d22bfb8b463223cf7ea30fe31f0d5ccdb6856b7e2826c550a  c14_5_vfs.c
5cc5db0685eb8bce5e712fb9ab0758a1c9180c91ef9e19fb67e0dae1b315dbb7  c14_6_journaling.c
e3ac9dfe72e88742ebb7a188d367cb84878e9322e0b18c936254983bd28795d3  c14_7_mount_points.c
685a496a6316bd7aa98cfb67823c0d1f021e4e6376b94afab659da57313353cf  c14_8_mount_umount.c
e1255442c9a11157bef6d025dc4dceed33c513fe69f4968bb03fd3c14708438c  c14_9_advanced_mount.c
28cd3d160ff9a8092f936d004b8fa4fba5601232c6023e83d02eb08489195816  ex14_1_file_churn.c
9e4cea7306307203f7f8fa0ed7d7d97c13fbca81a422c2d53f0923473fddde98  overlayfs_example.sh
6563ef6e25818a2daa9df6f535084c4e2af0d6646c5a21bba8a0bb62741b5ab0  t_mount.c
6f174ebb4883d24170bc0d21b2072dd0f653d447e0fcb3993eae65e006554c7c  t_statfs.c
c7ad5d193a4000f2f8a1d37138f6c2d5adf808f904ce168cf494855e3da64152  t_statvfs.c
7700d5085b5d03b0c6edaebc41502d33d91138e08d99651664fd2d42ac008c72  t_umount.c
4df91e00d47f14bf6ec44f6806f6bba9a232383d481224b657feff6bca2220a0  tlpi_hdr.h
```

> 笔记里的内联代码与本目录源码**逐行一致**（由 `sync_note_code.py` 强制同步、`cmp_note_code.py` 校验），所以笔记里引用的 `gcc` 行号（如 `<source>:33`）读者能对上。

---

## 六、怎么用这套代码

```bash
# 1. 全部编一遍（自编 demo）
for f in c14_*.c ex14_1_*.c; do
    gcc -O0 -Wall -Wextra -o "${f%.c}" "$f" || echo "FAILED: $f"
done

# 2. 原书镜像件（要 -I.）
for f in t_*.c; do
    gcc -O0 -Wall -Wextra -I. -o "${f%.c}" "$f" || echo "FAILED: $f"
done

# 3. 按 14.1 → 14.11 的顺序跑；先跑 c14_1，它能确定「这个环境有什么没有」
./c14_1_device_files

# 4. 做习题（14.13）：用一个独立子目录，别污染 /app
mkdir -p /tmp/churn && ./ex14_1_file_churn /tmp/churn 2000
./ex14_1_file_churn /tmp/churn 2000 -s     # 对照组：递增序创建
```

**建议的顺序**：`c14_1` → `c14_2` → `c14_3` → `c14_4` → `c14_5` → `c14_6` → `c14_7` → `c14_10` → `c14_11` → `c14_8` → `c14_9` → `ex14_1`。

理由：前六节建立「设备 → 分区 → 文件系统 → inode → VFS → 日志」的纵向链条；`c14_7` 与 `c14_10` 建立「挂载关系与超级块身份」的横向判据；`c14_11` 收口 API；`c14_8`/`c14_9` 是**注定失败**的实验（放后面，因为要知道前面那些判据才能读出信息量）；习题最后做。

---

## 七、与笔记的对应

| 节 | 笔记 | demo |
|----|------|------|
| 14.1 | [`notes/14.1-device-special-files.md`](../notes/14.1-device-special-files.md) | `c14_1_device_files.c` |
| 14.2 | [`notes/14.2-disks-and-partitions.md`](../notes/14.2-disks-and-partitions.md) | `c14_2_disks_partitions.c` |
| 14.3 | [`notes/14.3-file-systems.md`](../notes/14.3-file-systems.md) | `c14_3_fs_layout.c` |
| 14.4 | [`notes/14.4-i-nodes.md`](../notes/14.4-i-nodes.md) | `c14_4_inode.c` |
| 14.5 | [`notes/14.5-virtual-file-system-vfs.md`](../notes/14.5-virtual-file-system-vfs.md) | `c14_5_vfs.c` |
| 14.6 | [`notes/14.6-journaling-file-systems.md`](../notes/14.6-journaling-file-systems.md) | `c14_6_journaling.c` |
| 14.7 | [`notes/14.7-single-directory-hierarchy-mount-points.md`](../notes/14.7-single-directory-hierarchy-mount-points.md) | `c14_7_mount_points.c` |
| 14.8 | [`notes/14.8-mounting-and-unmounting.md`](../notes/14.8-mounting-and-unmounting.md) | `c14_8_mount_umount.c` + `t_mount.c` + `t_umount.c` |
| 14.9 | [`notes/14.9-advanced-mount-features.md`](../notes/14.9-advanced-mount-features.md) | `c14_9_advanced_mount.c` |
| 14.10 | [`notes/14.10-tmpfs.md`](../notes/14.10-tmpfs.md) | `c14_10_tmpfs.c` |
| 14.11 | [`notes/14.11-statvfs.md`](../notes/14.11-statvfs.md) | `c14_11_statvfs.c` + `t_statfs.c` + `t_statvfs.c` |
| 14.13 | [`notes/14.13-exercise.md`](../notes/14.13-exercise.md) | `ex14_1_file_churn.c` |
