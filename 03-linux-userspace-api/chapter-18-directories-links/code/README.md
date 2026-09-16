# Ch18 代码 — Directories and Links

> 实测环境：macOS 26.6.2 (arm64) · clang 23.1.0（micromamba `cdev`）。全部自编 demo `clang -Wall -Wextra` **零警告**通过。
> Linux 专有行为（`ETXTBSY`、`seekdir` 定位、`renameat2` 等）按 man-pages 6.19 / Linux v6.6 标注，Pi5 复测清单见[章 README](../README.md)。

## 编译

```bash
CC=clang    # 或任意 cc；本机为 /Users/a0000/micromamba/envs/cdev/bin/clang

# 自编 demo（无额外依赖）
$CC -Wall -Wextra -o c18_1_link_unlink      c18_1_link_unlink.c
$CC -Wall -Wextra -o c18_2_rename           c18_2_rename.c
$CC -Wall -Wextra -o c18_3_symlink_readlink c18_3_symlink_readlink.c
$CC -Wall -Wextra -o c18_4_cwd              c18_4_cwd.c
$CC -Wall -Wextra -o c18_6_mkdir_rmdir      c18_6_mkdir_rmdir.c
$CC -Wall -Wextra -o c18_7_remove           c18_7_remove.c
$CC -Wall -Wextra -o c18_8_readdir          c18_8_readdir.c
$CC -Wall -Wextra -o ex18_1_txtbsy          ex18_1_txtbsy.c
$CC -Wall -Wextra -o ex18_3_realpath        ex18_3_realpath.c

# 原书镜像（需要 tlpi_hdr.h 替身；t_unlink 还要链接 get_num.c）
D=-D_DARWIN_C_SOURCE
$CC $D -I. -Wall -Wextra -o t_unlink            t_unlink.c get_num.c
$CC $D -I. -Wall -Wextra -o list_files          list_files.c
$CC $D -I. -Wall -Wextra -o nftw_dir_tree       nftw_dir_tree.c
$CC $D -I. -Wall -Wextra -o view_symlink        view_symlink.c
$CC $D -I. -Wall -Wextra -o t_dirbasename       t_dirbasename.c
$CC $D -I. -Wall -Wextra -o bad_symlink         bad_symlink.c
$CC $D -I. -Wall -Wextra -Wno-deprecated-declarations -o list_files_readdir_r list_files_readdir_r.c
$CC $D -I. -Wall -Wextra -o file_type_stats     file_type_stats.c
```

> ⚠️ `bad_symlink` 与 `file_type_stats` 的 `-Wunused-parameter` 警告来自**原书源码本身**（回调签名带未用参数），不是移植问题。
> ⚠️ `ex18_1_txtbsy` 运行时会 `cp` 本程序自身到 `/tmp` 并 exec——**不要**把长跑程序换成拷贝的系统二进制（见 18-1 的签名坑）。

## 文件清单

### 自编 demo（对应官方节号）

| 文件 | 节 | 钉住的点（全部实测） |
|------|----|---------------------|
| `c18_1_link_unlink.c` | 18.1/18.3 | link 后两名字同 inode；unlink 后 fd 仍可读写、close 才释放；`unlink(/tmp)`=`EPERM`、`link(不存在)`=`ENOENT` |
| `c18_2_rename.c` | 18.4 | 原子替换且 inode 不变；目录四规则（EISDIR/ENOTDIR/ENOTEMPTY/可覆盖空目录）+ 自子树/`..` 两条 `EINVAL` |
| `c18_3_symlink_readlink.c` | 18.2/18.5 | 悬空链接合法（lstat OK / open ENOENT）；readlink 裸串；ELOOP 上限 40；链接自身 uid/gid/mode 的平台差异 |
| `c18_4_cwd.c` | 18.10/18.11/18.12 | fchdir+fd 方案「记住老目录」；`open(".")` 作 openat 锚点；非特权 chroot=`EPERM` |
| `c18_6_mkdir_rmdir.c` | 18.6 | mode 受 umask 削减（0777→0755）；`st_nlink=2+子目录数`；rmdir 三类失败面；`unlinkat(AT_REMOVEDIR)` |
| `c18_7_remove.c` | 18.7 | remove=lstat 分派（unlink/rmdir）；删链接不动目标；`unlink(dir)` 的 errno 平台差异 |
| `c18_8_readdir.c` | 18.8 | readdir 的 errno 协议；telldir cookie 不是偏移；**APFS 上 seekdir 恒等于 rewinddir**；fdopendir 所有权；d_type vs lstat |
| `ex18_3_realpath.c` | 18.13 | 习题 18-3：逐组件 lstat+readlink+栈式 `..`；与系统库对拍（`/etc/../etc/hosts`→`/private/etc/hosts`） |

### 习题实现

| 文件 | 习题 | 说明 |
|------|------|------|
| `ex18_1_txtbsy.c` | 18-1 | ①写打开运行中的程序（macOS 不 enforce ETXTBSY，Linux 为 ETXTBSY）；②cc 覆盖 = 建新 inode + rename，老 inode 被 fd 吊住。含「拷贝系统二进制被签名 SIGKILL」的自证环节 |
| `ex18_3_realpath.c` | 18-3 | realpath 教学版（无逐级权限检查），与系统库逐字节对拍 |

### 原书镜像（TLPI dist `dirs_links/`，逐字）

| 文件 | 出处 | 实测 |
|------|------|------|
| `t_unlink.c` | **Listing 18-1** | `./t_unlink bigfile 300000`：两次 `df` 差 ≈300 MB，`close` 后才释放 |
| `list_files.c` | **Listing 18-2** | `./list_files /etc` 正常 |
| `nftw_dir_tree.c` | **Listing 18-3** | `-p`/`-d` 对照：不加 `FTW_PHYS` 时指向目录的链接让子树遍历两次 |
| `view_symlink.c` | **Listing 18-4** | `readlink`（裸串）vs `realpath`（解析到底）并排 |
| `t_dirbasename.c` | **Listing 18-5** | TLPI 语义表 10 个边界用例逐行一致 |
| `bad_symlink.c` | 习题 18-2 官方解 | `chmod: No such file or directory`（相对链接以链接所在目录为基准） |
| `list_files_readdir_r.c` | 习题 18-4 官方解 | 可编译运行（`readdir_r` 已废弃，教学价值） |
| `file_type_stats.c` | 习题 18-7 官方解 | `/etc`：262 项，Regular 225 / Dir 33 / LNK 4（`FTW_PHYS`，无重复计数） |

### 支撑文件

| 文件 | 说明 |
|------|------|
| `tlpi_hdr.h` | 官方头文件的 macOS 替身（含 `errExit`/`usageErr`/`Boolean`，并 `#include "get_num.h"`） |
| `get_num.{h,c}` | 官方 lib 的 `getInt`/`getLong`（`t_unlink` 解析命令行用） |

## 典型运行示例

```bash
$ ./t_unlink bigfile 300000
Filesystem   1024-blocks     Used Available Capacity ...
/dev/disk3s5   239362496 29504280 184074220    14% ...
/dev/disk3s5   239362496 29204292 184374208    14% ...   ← close 后释放 ≈300 MB
********** Closed file descriptor                    ← stdio 全缓冲，排到最后

$ ./t_dirbasename /usr/lib/libc.so /usr/ usr / . .. "" "a/b/" "///" "/a//b"
/usr/lib/libc.so ==> /usr/lib + libc.so
/usr/             ==> / + usr
usr               ==> . + usr
/                 ==> / + /
.                 ==> . + .
..                ==> . + ..
                  ==> . + .
a/b/              ==> a + b
///               ==> / + /
/a//b             ==> /a + b

$ ./ex18_3_realpath /etc/../etc/hosts
输入: /etc/../etc/hosts
自实现: /private/etc/hosts
系统库: /private/etc/hosts
→ 一致 ✔
```
