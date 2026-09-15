# Ch11 `code/` 目录说明

TLPI 第 11 章（System Limits and Options）的可编译代码。分三类：

| 类别 | 数量 | 命名 |
|------|------|------|
| 自编 demo | **7** | `c11_<节>_<名字>.c`（5 个）+ `ex11_<n>_<名字>.c`（2 个） |
| 原书镜像 | **2** | `t_sysconf.c`(Listing 11-1)、`t_fpathconf.c`(Listing 11-2)，**逐字保真** |
| 框架替身 | **1** | `tlpi_hdr.h`（只含 `errExit` / `fatal` / `usageErr`） |

**全部 9 个作业**都在 Compiler Explorer（gcc 13.3.0 / x86-64 / Ubuntu 24.04）上跑过：`build code = 0`、`didExecute = True`、`diagnostics = 0`。

> 为什么走 Compiler Explorer：本机环境里 `wsl.exe` 被安全策略禁用，且没有任何 C 编译器（`gcc`/`clang`/`tcc`/`cl`/`cc`/`zig` 全无）。CE 提供真实的 gcc 13.3 编译诊断与真实运行输出；笔记里凡引用输出都标注「CE 实测」，**不当成本机实测**。
>
> ⚠️ CE 是**一次性容器**，而且是个**功能残缺的容器**：没有 `/bin/sh`、没有 `/bin/bash`、没有 `getconf`、没有 `/dev/fd`、没有 `/dev/shm`；`RLIMIT_NOFILE` 只有 **100**。所以本章**所有交叉验证都靠程序内部打两遍，不靠外部命令**（这也是本章不写 `getconf ARG_MAX` 之类命令的原因）。

---

## 文件表

### 自编 demo（7）

| 文件 | 对应节 | 演示什么 | 需要什么 |
|------|--------|----------|---------|
| `c11_1_three_kinds.c` | 11.1 | 三类限制现场对照：先查 `_SC_CLK_TCK` / `_SC_PAGESIZE` / `_SC_VERSION` / `_SC_OPEN_MAX`，把 `RLIMIT_NOFILE` 的 soft 减半后**只重查那四项** —— 前三项一位不变（类 1 恒定），`_SC_OPEN_MAX` 从 100 变 50（类 3，它只是一个当前值）；末尾附 10 行 `_POSIX_*` 下限宏 vs 实测值对照表 | — |
| `c11_2_sysconf_table.c` | 11.2 | `sysconf()` 四类 name 全表（常量类 / 限制类 28 项 / 选项类 / 扩展类），**每行都带「调用后 `errno`」**；`-1` 三义逐个抓现场：`sysconf(9999)` → `EINVAL(22)`、`_SC_TZNAME_MAX` → `-1` 且 `errno==0`（indeterminate）、`_SC_2_FORT_DEV` → `-1`（选项不支持）；末段演示「不重置 `errno` 就会把 indeterminate 误判成错误」 | — |
| `c11_3_pathconf_matrix.c` | 11.3 | 三维矩阵：**A** 同一组 `_PC_*` 问 `/`(tmpfs) 与 `/lib`(ext4) —— 只有 `_PC_LINK_MAX`(127 vs 65000) / `_PC_FILESIZEBITS` 会变，`_PC_PATH_MAX` / `_PC_PIPE_BUF` 恒 4096；**B** 同一组问题问四种 fd 类型（目录 / 普通文件 / 管道 / 字符设备），`_PC_ASYNC_IO` 分叉；**C** 错误码矩阵（`""` → `ENOENT`、`/proc/version/inside` → `ENOTDIR`、非法 name → `EINVAL`、非法 fd → `EBADF`） | — |
| `c11_4_indeterminate.c` | 11.4 | 扫 20+ 项 `_SC_*` 只打印真 indeterminate 的（实测 **4 项**：`_SC_TZNAME_MAX` / `_SC_SYMLOOP_MAX` / `_SC_MQ_OPEN_MAX` / `_SC_SEM_NSEMS_MAX`）；`pathconf("/")` 侧 **3 项**（`_PC_ASYNC_IO` / `_PC_SYNC_IO` / `_PC_PRIO_IO`）；再演示**降级阶梯**：`sysconf` → `pathconf` → `#ifdef NAME_MAX` → 保守常量（`name_max_ladder()` 三级全部打印，含「为什么必须 `#ifdef`」） | — |
| `c11_5_options.c` | 11.5 | `_POSIX_FOO` **四态**用宏打出来（未定义 / `-1` 不支持 / `0` 存在但支持度要问 / 正值支持），再与运行时 `sysconf(_SC_*)` 逐项对齐（重点看 `_POSIX_MONOTONIC_CLOCK == 0` 而 `sysconf(_SC_MONOTONIC_CLOCK)` 返回 `200809`）；末段 `confstr()` 三连：`_CS_PATH` → `"/bin:/usr/bin"`、`_CS_GNU_LIBC_VERSION` → `"glibc 2.39"`、`_CS_GNU_LIBPTHREAD_VERSION` → `"NPTL 2.39"` | — |
| `ex11_1_sysconf_wide.c` | 11.7 练习 11-1 | 先**原样**跑一遍 Listing 11-1 的 6 项（方便与书上输出逐行对照），再给宽表：**值 / glibc 2.39 的取值路径 / POSIX 只保证的下限** 三列。第三列才是本题答案——它说明「换一个 UNIX 实现，哪些项允许不一样、允许差到多少」 | — |
| `ex11_2_fs_sweep.c` | 11.7 练习 11-2 | 遍历 `/proc/mounts`，对**每个**挂载点问同一组 `_PC_*`，按 `(NAME_MAX, LINK_MAX, FILESIZEBITS)` **去重**后打印。实测扫到 **60 个**挂载点、归并成 **3 种答案**：tmpfs **31** / ext4 **10** / squashfs **19**；每行附 `statfs().f_type` 与 `statvfs().f_namemax` | `/proc/mounts` 可读 |

### 原书镜像（2，逐字保真）

| 文件 | 原书位置 | 说明 | 需要什么 |
|------|---------|------|---------|
| `t_sysconf.c` | **Listing 11-1, p.216** | 六项 `sysconf()` 的标准写法。核心是那个 `sysconfPrint()`：`errno = 0` → `sysconf(name)` → 三分支（`!= -1` 成功 / `errno == 0` indeterminate / 否则 `errExit`）。**这 12 行就是全章的骨架** | `tlpi_hdr.h` |
| `t_fpathconf.c` | **Listing 11-2, p.218** | 同一套写法换 `fpathconf(STDIN_FILENO, …)`，问 `_PC_NAME_MAX` / `_PC_PATH_MAX` / `_PC_PIPE_BUF`。**注意它写死 fd 0**，所以 stdin 必须指向一个真实文件（否则 `fpathconf` 拿到的是管道的答案） | `tlpi_hdr.h` + stdin 可读 |

> 页码出处：man7 单文件页原文（`This is syslim/t_sysconf.c (Listing 11-1, page 216), an example from the book, The Linux Programming Interface.` 等）。
> 镜像来源：`https://man7.org/tlpi/code/online/dist/syslim/<file>`。
> 文件清单出处：[man7 — List of source code files, by chapter](https://man7.org/tlpi/code/online/all_files_by_chapter.html) 的 **Chapter 11** 一节——共 **2 个**文件，都在 `syslim/` 下。

⚠️ **本章只有 2 个原书示例**，是全模块最少的章之一。真正的信息量在「拿这两个程序去问不同的机器 / 不同的文件系统，答案怎么变」——这正是两道习题的设计意图，所以本目录给两题各写了一份实现。

⚠️ **习题原文未逐字引用（诚实标注）**：`ex11_1_*` / `ex11_2_*` 的头注释里只写【任务】，不引原书题干——因为 man7 只分发源码、不放习题正文，公开的第三方镜像（`boykaa/lpi-exercises` 等）已 404，无法核验逐字原文。

### 框架替身（1）

| 文件 | 说明 |
|------|------|
| `tlpi_hdr.h` | 原书 `lib/tlpi_hdr.h` 的**最小可用子集**，按原书语义实现 3 个助手：`errExit`（`fmt: strerror(errno)` + `exit(1)`）/ `fatal`（不带 errno）/ `usageErr`（打 `Usage: ` 前缀）。原书结构是「`tlpi_hdr.h` 声明 + `error_functions.c` 实现」，为单文件可控，这里把实现做成 `static inline` 放在头里 |

> ⚠️ 这份 `tlpi_hdr.h` 与 Ch04 / Ch05 / Ch10 的同名替身**不能互换**：
> - Ch04/Ch05 那份多带 `getLong` / `getInt`（`copy.c` / `atomic_append.c` 要用）；
> - 这里 Ch11 的两个原书程序**只用到 `errExit`**，所以只保留三个助手。
>
> 别跨章复制粘贴——这是本仓库故意保持的「每章最小化」策略。

---

## 编译

一次编完全部自编 demo（在 `code/` 目录下）：

```bash
for f in c11_*.c ex11_*.c; do
    gcc -O0 -Wall -Wextra -o "${f%.c}" "$f" || echo "FAIL $f"
done
```

7 个自编 demo 都在源码开头自带 `#define _GNU_SOURCE`，**不需要额外旗标**。

2 个原书程序都需要 `tlpi_hdr.h`（**同目录即可**）：

```bash
for f in t_sysconf t_fpathconf; do
    gcc -O0 -Wall -Wextra -Wno-unused-parameter -o "$f" "$f.c" || echo "FAIL $f"
done
```

> `-Wno-unused-parameter` 是因为两个原书程序的 `main(int argc, char *argv[])` **根本不用这两个参数**。原书 Makefile 只开 `-Wall`，我们额外加的 `-Wextra` 会把它们报成 `warning: unused parameter`（CE 上实测 7 条诊断）。**原书代码一字不改**，改旗标。

## 运行示例

```bash
./c11_1_three_kinds        # 三类限制 + rlimit 现场改动（会调 setrlimit）
./c11_2_sysconf_table      # 全表 + -1 三义
./c11_3_pathconf_matrix    # 挂载点矩阵 + 四种 fd 类型 + 五条错误路径
./c11_4_indeterminate      # indeterminate 清单 + 降级阶梯
./c11_5_options            # _POSIX_FOO 四态 + confstr 三连
./ex11_1_sysconf_wide      # 加宽版 Listing 11-1
./ex11_2_fs_sweep          # 扫全部挂载点（需 /proc/mounts 可读）

# 原书程序
./t_sysconf                      # Listing 11-1
./t_fpathconf < /proc/mounts     # Listing 11-2（fd 0 需指向一个真实文件）
```

## 沙箱环境注意（这些「失败」是环境限制，不是代码 bug）

| 现象 | 原因 |
|------|------|
| 没有 `getconf` | CE 容器里没装。本章的交叉验证因此全部改成「程序内打两遍」，不依赖外部命令 |
| 没有 `/bin/sh`、`/bin/bash` | 所以本章没有一处用 `system()` / `popen()`。`ex11_2_fs_sweep.c` 直接读 `/proc/mounts` 自己解析，不走 `mount` 命令 |
| `pathconf("/lib", _PC_LINK_MAX)` 返回 65000 但 `errno == 2` | **返回值是对的，`errno` 是脏的**。glibc 的 `distinguish_extX()` 要先 `readlink /sys/dev/block/MAJ:MIN` 判断是 ext2/3 还是 ext4，失败再退到 `/proc/mounts`——失败那一次把 `ENOENT` 留在了 `errno`（`sysdeps/unix/sysv/linux/pathconf.c:63-128`）。教训：**判 `errno` 前必须先 `errno = 0`** |
| `_SC_NPROCESSORS_CONF` 返回 2 但 `errno == 2` | 同上同源 |
| `RLIMIT_NOFILE` soft = hard = **100** | 容器默认值。本机桌面上通常远大于此（1024 / 1048576）。所以 `_SC_OPEN_MAX = 100`、fd 只能开到第 47 个（0/1/2 已占，第 48 个 `EMFILE`）——**这些数字是容器特征，不是 Linux 特征** |
| `_SC_ARG_MAX = 2097152` 但 `execve` 传 128 KiB 就 `E2BIG` | 2 MiB 是 `MAX(131072, RLIMIT_STACK/4)` 算出来的**名义值**（`linux/sysconf.c:56-67`）；真正卡住的是 `MAX_ARG_STRLEN = PAGE_SIZE*32 = 131072`（`include/uapi/linux/binfmts.h:16`）。**这是本章最重要的反直觉点**，不是 bug |
| `_SC_CHILD_MAX` / `_SC_SIGQUEUE_MAX` 两次跑出不同数（54956 / 56783） | 它们来自 `RLIMIT_NPROC` / `RLIMIT_SIGPENDING` 的 `rlim_cur`，随容器当时状态变。**别把某一次的数字当常量** |
| `ex11_2_fs_sweep` 的挂载点计数每次可能不同 | `/cefs` 下挂了几十个带哈希的 autofs/squashfs 子挂载点，路径名每次都不一样。程序已按三元组**去重**，所以稳定输出三行，但首行的「扫到 N 个挂载点」会漂移 |
| `_SC_2_FORT_DEV = -1` | glibc 在 Linux 上确实不支持 FORTRAN 开发工具集选项（`sysdeps/posix/sysconf.c:486-491`）。**这是预期答案**，不是错误 |
| `/dev/fd`、`/dev/shm` 不存在 | Ch05 也用到的同一个容器限制。本章不涉及 |
