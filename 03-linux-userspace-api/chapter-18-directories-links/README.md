# TLPI 第 18 章 — Directories and Links 目录与链接

**优先级**：🔴（路径树操作、临时文件、TOCTOU、可靠写入——HFT 落盘与部署安全的根基）
**前置**：[Ch03 系统编程概念](../chapter-03-system-programming-concepts/README.md) · [Ch04 File I/O](../chapter-04-file-io-universal/README.md)（fd 与 inode 绑定）· [Ch14 FS/inode](../chapter-14-file-systems/README.md) · [Ch15 stat/lstat](../chapter-15-file-attributes/README.md)
**后置**：[Ch19 inotify](../chapter-19-monitoring-file-events/README.md)

> **实测环境**：macOS 26.6.2 (arm64) · clang 23.1.0（micromamba `cdev`）· APFS。
> Pi（192.168.31.109）本轮离线，Linux 专有行为按 **man-pages 6.19** / **Linux v6.6 源码**标注，未实测处一律诚实标注并入 **Pi5 复测清单**。
> 节号与官方目录 `toc-detailed.html` 逐条对齐（18.1–18.16，无子节）。

---

## 小节目录

- [18.1 Directories and (Hard) Links 目录与硬链接](notes/18.1-directories-and-hard-links.md)
- [18.2 Symbolic (Soft) Links 符号链接](notes/18.2-symbolic-soft-links.md)
- [18.3 Creating and Removing (Hard) Links: link()/unlink()](notes/18.3-creating-and-removing-hard-links-link-an.md)
- [18.4 Changing the Name of a File: rename()](notes/18.4-changing-the-name-of-a-file-rename.md)
- [18.5 Working with Symbolic Links: symlink()/readlink()](notes/18.5-working-with-symbolic-links-symlink-and-.md)
- [18.6 Creating and Removing Directories: mkdir()/rmdir()](notes/18.6-creating-and-removing-directories-mkdir-.md)
- [18.7 Removing a File or Directory: remove()](notes/18.7-removing-a-file-or-directory-remove.md)
- [18.8 Reading Directories: opendir()/readdir()](notes/18.8-reading-directories-opendir-and-readdir.md)
- [18.9 File Tree Walking: nftw()](notes/18.9-file-tree-walking-nftw.md)
- [18.10 The Current Working Directory of a Process](notes/18.10-the-current-working-directory-of-a-proce.md)
- [18.11 Operating Relative to a Directory File Descriptor（*at() 族）](notes/18.11-operating-relative-to-a-directory-file-d.md)
- [18.12 Changing the Root Directory: chroot()](notes/18.12-changing-the-root-directory-of-a-process.md)
- [18.13 Resolving a Pathname: realpath()](notes/18.13-resolving-a-pathname-realpath.md)
- [18.14 Parsing Pathname Strings: dirname()/basename()](notes/18.14-parsing-pathname-strings-dirname-and-bas.md)
- [18.15 Summary](notes/18.15-summary.md)
- [18.16 Exercises](notes/18.16-exercises.md)

---

## 章节目标

- **分清「名字」与「本体」两个层次**：目录项/路径串是名字，inode 是本体。`link`/`unlink`/`rename` 全是目录表操作——`unlink` 只摘名字（实测：名字全没了 fd 还能读回数据，`close` 才释放）、`rename` 原子换名（实测：inode 不变）
- **三种「覆盖/替换」语义**：`unlink+open`（有竞态窗口）、`rename`（原子替换，safe-write 的根）、`cc 覆盖运行中的程序`（= rename 换 inode，老 inode 被 fd 吊住——习题 18-1 完整实测）
- **目录是一等公民但有独立规则**：`r/w/x` 三位含义完全不同于文件（实测：`st_nlink = 2 + 子目录数`）；`rmdir` 只吃「只剩 `.`/`..`」的目录；指向目录的符号链接要用 `unlink` 删（`rmdir` 报 `ENOTDIR`）
- **遍历的正确姿势**：`readdir` 返回 `NULL` 要查 `errno`（否则 I/O 错误被当「读完」→ 备份工具丢数据）；`nftw` 递归删除必须 `FTW_DEPTH|FTW_PHYS`（实测：不加 `FTW_PHYS` 指向目录的链接会让**子树被遍历两次**）
- **消除 cwd 全局状态**：`*at()` 族把基准从进程属性变成一个 fd——锚点打开即固定，TOCTOU 与多线程竞争一起消掉；「记住老目录」用 `open(".") + fchdir`（fd 方案免疫改名/删除，习题 18-9 正解）
- **诚实**：本轮实测纠正了两处旧结论——① macOS 与 Linux 的 rename 目录规则**一致**（此前笔记怀疑有差异，实测证伪）；② 「macOS 不 enforce ETXTBSY」必须用**自编译**长跑程序验证（拷贝的系统二进制会被代码签名 SIGKILL，rc=137，此前测试无效）

---

## 本轮实测钉住的硬结论

| # | 结论 | 节 | 代码 |
|---|------|----|------|
| 1 | `unlink` 后 fd 仍可读写；名字全没了还能读回数据；`close` 才释放 | 18.3 | `c18_1_link_unlink.c` |
| 2 | 300 MB 文件 unlink 后名字消失、`close` 后 `df` 才见空间释放（184074220→184374208） | 18.3 | `t_unlink.c`（Listing 18-1） |
| 3 | `rename` 原子替换且 inode 不变；目录四规则 + 自子树/`..` 两条 `EINVAL` **与 Linux 一致** | 18.4 | `c18_2_rename.c` |
| 4 | 悬空链接合法（创建返回 0，使用时才 `ENOENT`）；ELOOP 上限 40 层 | 18.2 | `c18_3_symlink_readlink.c` |
| 5 | `rmdir(指向目录的符号链接)` = `ENOTDIR`；`rmdir(".")`/`("..")` = `EINVAL` | 18.6 | `c18_6_mkdir_rmdir.c` |
| 6 | `remove` 是 libc 分派器（lstat → unlink/rmdir）；删链接时目标目录安然无恙 | 18.7 | `c18_7_remove.c` |
| 7 | **macOS/APFS 的 `seekdir` 恒等于 `rewinddir`**（实测 5 个 cookie 全回第一项）；`telldir` 值不是字节偏移 | 18.8 | `c18_8_readdir.c` |
| 8 | 目录 `st_nlink = 2 + 子目录数`；APFS 目录 `st_size` 非块对齐（1952/960，与 Linux 相反） | 18.1/18.6 | `c18_6_mkdir_rmdir.c` |
| 9 | **不加 `FTW_PHYS` 指向目录的链接让子树被遍历两次**（inode 相同、条目重复） | 18.9 | `nftw_dir_tree.c`（Listing 18-3） |
| 10 | **macOS 不 enforce ETXTBSY**（探活确认子进程在跑）；cc 覆盖 = rename 换 inode | 18.16 | `ex18_1_txtbsy.c` |
| 11 | **拷贝系统二进制到 /tmp 被代码签名 SIGKILL**（rc=137）；自编 ad-hoc 二进制可任意搬 | 18.16 | `ex18_1_txtbsy.c` |
| 12 | `realpath` 对拍：`/etc/../etc/hosts` → `/private/etc/hosts`（`..` 是物理父目录，不能字符串消去） | 18.13 | `ex18_3_realpath.c` |
| 13 | `t_dirbasename` 全边界表逐行与 TLPI 一致（`/usr/`→`usr`、`..`→`.`、空串→`.`） | 18.14 | `t_dirbasename.c`（Listing 18-5） |
| 14 | 非特权 `chroot` = `EPERM`；chroot 后行为标注「源码核验、未实测」入 Pi5 清单 | 18.12 | `c18_4_cwd.c` |
| 15 | `fchdir` + 目录 fd 方案免疫改名/删除；`openat` 锚点与 cwd 解耦 | 18.10/18.11 | `c18_4_cwd.c` |

---

## 三个惯用法（全章的工程兑现）

```c
/* ① 零竞态临时文件：名字从未暴露，崩溃自动回收（§18.3）*/
int fd = open(path, O_RDWR|O_CREAT|O_EXCL, 0600);
unlink(path);
/* ... 用 fd ... */
close(fd);                       /* 数据此刻才释放 */

/* ② safe-write：读者永远看到完整旧版或完整新版（§18.4）*/
write(tmp); fsync(tmp); rename(tmp, target);

/* ③ 锚点模式：把名字换成引用，消除 cwd 竞争与 TOCTOU（§18.11）*/
int anchor = open(root, O_RDONLY|O_DIRECTORY);
int fd = openat(anchor, rel, O_RDONLY|O_NOFOLLOW);
```

---

## Pi5 复测清单（本轮 macOS 实测无法覆盖的 Linux 侧行为）

| 项 | 节 | 预期 |
|----|----|------|
| `open(正在执行的程序, O_WRONLY)` → `ETXTBSY(16)` | 18.16 | Linux 强制；macOS 实测不 enforce |
| `unlink(dir)` → `EISDIR`（macOS 为 `EPERM`） | 18.7 | 错误码不同，「都拒绝」一致 |
| `seekdir(cookie)` 能正确定位（ext4） | 18.8 | Linux 正常；macOS 实测失效 |
| `rmdir("..")` 的具体 errno | 18.6 | man-pages 只写明 `.` → `EINVAL` |
| 目录 `st_size` 为块整数倍（ext4/tmpfs） | 18.1 | 与 APFS 实测值对照 |
| `linkat(AT_EMPTY_PATH)` 对 fd 建链接 | 18.3 | Linux 专有扩展 |
| `renameat2(RENAME_EXCHANGE)` 原子互换 | 18.4 | Linux 3.15+ 专有 |
| `chroot` 生效后的解析起点变化 + 越狱路径 | 18.12 | 需 sudo；macOS 只能实测到 `EPERM` |
| 符号链接 `lstat` 的 `mode`/属主（ext4 忽略权限位） | 18.2 | 与 macOS（有 mode=755）对照 |
| `getdents64` 的 `d_off` 为字节偏移 | 18.8 | 与 macOS 计数式 cookie 对照 |

---

## 代码清单

自编 demo（macOS 实测通过，`clang -Wall -Wextra` 零警告）+ 原书镜像（TLPI dist `dirs_links/` 逐字）+ 支撑文件，**全部明细与编译命令见 [`code/README.md`](code/README.md)**。

| 类别 | 文件 |
|------|------|
| 自编 demo（7） | `c18_1_link_unlink` `c18_2_rename` `c18_3_symlink_readlink` `c18_4_cwd` `c18_6_mkdir_rmdir` `c18_7_remove` `c18_8_readdir` |
| 习题实现（2） | `ex18_1_txtbsy`（18-1，含签名坑自证）`ex18_3_realpath`（18-3，对拍系统库） |
| 原书镜像（8） | `t_unlink`(18-1) `list_files`(18-2) `nftw_dir_tree`(18-3) `view_symlink`(18-4) `t_dirbasename`(18-5) `bad_symlink`(18-2 解) `list_files_readdir_r`(18-4 解) `file_type_stats`(18-7 解) |
| 支撑 | `tlpi_hdr.h` `get_num.{h,c}` |

---

## 参考

- Kerrisk · TLPI Ch18（18.1–18.16）
- man-pages 6.19：`link(2)` `unlink(2)` `rename(2)` `symlink(2)` `readlink(2)` `mkdir(2)` `rmdir(2)` `remove(3)` `readdir(3)` `fdopendir(3)` `seekdir(3)` `nftw(3)` `getcwd(3)` `chdir(2)` `openat(2)` `chroot(2)` `realpath(3)` `dirname(3)` `path_resolution(7)`
- Linux v6.6：`fs/namei.c`（路径解析/`ELOOP`/`vfs_*`）、`fs/open.c`（`chdir`/`chroot`）、`fs/readdir.c`（`getdents64`）
- 官方源码分发：`man7.org/tlpi/code/download/tlpi-260523-dist.tar.gz`（`dirs_links/` 共 8 个 .c + Makefile，本仓库全部逐字镜像）
- 习题编号核验：`github.com/segarciat/TLPI` `ch18-Directories-and-Links/exercises/`（18-1 ～ 18-9）
