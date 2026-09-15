# TLPI 第 11 章 — System Limits and Options

**优先级**：🟡→🔴（可移植性 / 路径长度 / 管道原子写边界；Ch15 / Ch36 / Ch44 都要回头看这一章）
**前置**：[Ch3](../chapter-03-system-programming-concepts/README.md)（syscall 与 `errno` 范式——本章所有「`-1` 到底是哪个意思」全靠这个）· [Ch10](../chapter-10-time/README.md)（`_SC_CLK_TCK` ↔ `times()`）
**后置**：[Ch12 系统与进程信息](../chapter-12-system-process-info/README.md)（`/proc`、`uname`——限制信息的另一来源）· [Ch15 文件属性](../chapter-15-file-attributes/README.md)（`NAME_MAX` / `PATH_MAX` 的实际使用）· [Ch36 进程资源](../chapter-36-process-resources/README.md)（`RLIMIT_*`）· [Ch44 管道与 FIFO](../chapter-44-pipes-fifos/README.md)（`_PC_PIPE_BUF`）

---

## 小节目录

- [11.1 System Limits 系统限制的三类分法](notes/11.1-system-limits.md)
- [11.2 Retrieving System Limits at Run Time 运行时查询系统限制](notes/11.2-runtime-limits.md)
- [11.3 Retrieving File-Related Limits at Run Time 运行时查询文件相关限制](notes/11.3-file-related-limits.md)
- [11.4 Indeterminate Limits 不确定的限制](notes/11.4-indeterminate-limits.md)
- [11.5 System Options 系统选项](notes/11.5-system-options.md)
- [11.6 Summary 本章总结](notes/11.6-summary.md)
- [11.7 Exercises 练习](notes/11.7-exercises.md)

> 七节的划分与 TLPI 原书一致（11.1–11.7）。原书本章**没有子编号小节**（不像 Ch3 那样有 3.5.1/3.6.1），所以一篇对应一节。
>
> **注意 11.7 的性质**：原书第 11 章末有 **2 道**习题（11-1、11-2，都是「把本章的示例程序换个环境再跑一遍」）。本仓库为两题各写了一份可运行的实现（`ex11_1_*` / `ex11_2_*`）。
>
> ⚠️ **原书习题文字的公开原文未能核验**：man7 只分发源码、不放习题正文，所以 11.7 与两份 `ex11_*.c` 的头注释里**只写任务要求，不做逐字引用**。

---

## 章节目标

- **分清三个入口**：编译期宏 / 运行时全局查询 / 运行时路径查询。**问错了入口，答案本身就是错的**（问 `NAME_MAX` 用 `sysconf()` 根本查不到——没有 `_SC_NAME_MAX` 这个常量）
- **`-1` 有三义**：① 调用失败（`errno` 被设）② 限制不确定（`errno` **不变**）③ 选项不支持（`-1` 就是答案）。**同一个返回值，三种处置**
- **`limit` 与 `option` 是两类问题**：limit 的答案是「一个数」，option 的答案是「支持 / 不支持」。man-pages 把 RETURN VALUE 拆成两条写，正是因为它们共用了 `-1`
- **诚实**：本仓库把 man-pages 6.19、POSIX.1-2017、glibc 2.39、Linux v6.6 的相关段落全部抓下来逐字核对，发现**书上的若干说法需要修正**（`_SC_ARG_MAX` 的 2 MiB 只是「名义值」；`_PC_PATH_MAX` 压根不问文件系统）
- **溯源**：本章 **7 篇**笔记里的每一行实测输出都能在 CE 日志 `tlpi-ch11-final.txt` 里定位；每一处 glibc / 内核坐标都实读核准

### 一条主线：四级分派

`sysconf()` 不是「查表」，而是**一层层往下问**，最后落到一个「查不到就报错」的兜底：

| 级 | 谁 | 在哪 | 管什么 |
|----|----|------|--------|
| ① | `__sysconf`（x86 特化） | `sysdeps/unix/sysv/linux/x86/sysconf.c:31-37` | 只拦 8 个 `_SC_LEVEL*_CACHE_*`，转给 `__cache_sysconf`，其余全丢给下一级 |
| ② | `linux_sysconf` | `sysdeps/unix/sysv/linux/sysconf.c` + `linux/sysconf.c` | Linux 专属项：`_SC_NGROUPS_MAX` 读 `/proc/sys/kernel/ngroups_max`、`_SC_ARG_MAX` 算 `rlim_stack/4`、`_SC_SIGQUEUE_MAX` 取 `RLIMIT_SIGPENDING` … |
| ③ | `posix_sysconf` | `sysdeps/posix/sysconf.c` | POSIX 通项：`_SC_CLK_TCK` → `__getclktck()`、`_SC_OPEN_MAX` → `__getdtablesize()`、一堆 `#ifdef ... return ...` |
| ④ | `default:` | `sysdeps/posix/sysconf.c:64-66` | `__set_errno(EINVAL); return -1;` —— **这才是「真错误」的唯一出处** |

同一套四级结构在 `pathconf()` 上重复一遍（`x86` 无特化，从 `linux` 起算），只是第 ① 级换成了「只有 4 个 `_PC_*` 真的去问文件系统」：

| 真的问文件系统（走 `statfs`） | 直接返回编译期常量 |
|------------------------------|-------------------|
| `_PC_LINK_MAX`、`_PC_FILESIZEBITS`、`_PC_2_SYMLINKS`、`_PC_CHOWN_RESTRICTED` | `_PC_PATH_MAX` → `PATH_MAX`(4096)、`_PC_PIPE_BUF` → `PIPE_BUF`(4096)、`_PC_MAX_CANON` / `_PC_MAX_INPUT` / `_PC_VDISABLE` → 常量 |

一句话：**「文件相关的限制」里只有一半真的跟文件有关。**

### 与 Ch10 / Ch12 边界（防混淆）

| 章 | 主题 | 内容 |
|----|------|------|
| **Ch10** | Time | **读 / 写 / 量**时间。`sysconf(_SC_CLK_TCK)` 只是它顺手用过的一个常量 |
| **Ch11** | System Limits and Options | **问系统「能到什么程度」**：数（limit）与布尔（option），三个入口（宏 / `sysconf` / `pathconf`），以及 `-1` 三种含义 |
| **Ch12** | System and Process Information | `/proc`、`uname`、`sysinfo` —— **另一种拿同样信息的方式**，往往比 `sysconf` 更直接（例如 `/proc/meminfo` vs `_SC_PHYS_PAGES`） |

Ch11 与 Ch12 是**同一批数据的两个入口**：Ch11 是 POSIX 可移植接口，Ch12 是 Linux 专有的「直接看内核」。移植优先 Ch11，刨根问底用 Ch12。

---

## 原书示例清单（man7 官方按章文件列表）

| Listing | 文件 | 页码 | 作用 |
|---------|------|------|------|
| **11-1** | `syslim/t_sysconf.c` | **p.216** | 六项 `sysconf()` 的标准写法：`errno=0` → 调 → 按「`-1` 且 `errno==0`」判 indeterminate |
| **11-2** | `syslim/t_fpathconf.c` | **p.218** | 三项 `fpathconf()` 对 `STDIN_FILENO` 查 `_PC_NAME_MAX` / `_PC_PATH_MAX` / `_PC_PIPE_BUF` |

> 出处：[man7 — List of source code files, by chapter](https://man7.org/tlpi/code/online/all_files_by_chapter.html) 的 **Chapter 11** 一节（共 **2 个**文件，都在 `syslim/` 下）。
> 本仓库在 `code/` 下**逐字镜像**了这 2 个文件，并提供 `tlpi_hdr.h` 的**最小替身**让它们能单文件编译（原书的 `tlpi_hdr.h` 依赖整个 `lib/` 目录，且以 `@` 结尾标记「已完成」）。
>
> ⚠️ **本章只有 2 个原书文件**，是全模块示例最少的章之一——但**实测数据最多**：真正的知识量在「拿这两个程序去问不同的系统/文件系统，答案怎么变」。

两个容易踩的坑：

| 容易踩的坑 | 正解 |
|-----------|------|
| 把 `errno = 0` 省掉 | **不能省**。`-1` 到底是「错误」还是「不确定」，**唯一判据就是 `errno` 是否被改动**；不预先清零，你读到的是上一次调用留下的陈年 `errno` |
| 以为 `-1` 一定是「失败」 | `-1` 也可能是**正确答案**：选项不支持时就是 `-1`，而限制不确定时也是 `-1`（但 `errno == 0`） |

---

## 易错清单

1. **`_1` 的三义必须靠 `errno` 拆开** —— ① 错误（`errno != 0`）② indeterminate（`errno == 0`）③ 选项不支持（`-1` 是答案，`errno` 也不变）。**先把 `errno = 0`，再判断**。
2. **`sysconf()` 对「不认识的常量」报 `EINVAL`** —— 实测 `sysconf(9999)` → `errno=22`、返回 `-1`。唯一出处是 `sysdeps/posix/sysconf.c:64-66`。
3. **`_SC_TZNAME_MAX` 在本机恒为 `-1` 且 `errno` 不变** —— 它是**恒 indeterminate**，不是坏了（`sysdeps/posix/sysconf.c:102-103` 直接 `return -1`，不 `set_errno`）。同理 `_SC_SYMLOOP_MAX`、`_SC_MQ_OPEN_MAX`、`_SC_SEM_NSEMS_MAX` 也是（共 **4 项**）。
4. **`_SC_OPEN_MAX` 不是常量，它等于 `getrlimit(RLIMIT_NOFILE).rlim_cur`** —— glibc 走 `__getdtablesize()`（`sysdeps/posix/getdtsz.c:32`）。实测：soft 从 100 降到 50 后，`_SC_OPEN_MAX` **立刻**变成 50。man-pages 那句「值在进程生命期内不变」对它是**不成立**的。
5. **`_SC_CLK_TCK` 恒为 100，与 `CONFIG_HZ` 无关** —— 它是内核经 `auxv` 的 `AT_CLKTCK` 传进来的 `USER_HZ`；glibc 侧 `__getclktck()` 是 `GLRO(dl_clktck) ?: SYSTEM_CLK_TCK`，兜底值写死 100（`sysdeps/unix/sysv/linux/getclktck.c:22-31`）。
6. **`_SC_ARG_MAX` 的 2 MiB 是「名义值」** —— 实测本机 `_SC_ARG_MAX = 2097152`（= `RLIMIT_STACK(8 MiB)/4`，glibc `linux/sysconf.c:56-67`），但同一台机器上 `execve` 传 **65536 字节 OK / 131072 字节 `E2BIG`(7)**。真正卡住的是 `MAX_ARG_STRLEN = PAGE_SIZE*32 = 131072`（`include/uapi/linux/binfmts.h:16`）。
7. **`ARG_MAX` 本身「难以使用」是官方承认的** —— man-pages BUGS 段原话：「It is difficult to use `ARG_MAX` because it is not specified how much of the argument space for `exec(3)` is consumed by the user's environment variables.」另外还有一条：「Some returned values may be huge; they are not suitable for allocating memory.」
8. **`pathconf` 的返回值可能带「脏 `errno`」** —— 实测 `pathconf("/lib", _PC_LINK_MAX)` 返回正确的 `65000`，但 `errno` 被留成 `2`（`ENOENT`）。原因是 glibc 区分 ext2/3/4 时先 `readlink /sys/dev/block/MAJ:MIN`，失败退到 `/proc/mounts`，失败那次把 `ENOENT` 留在了 `errno`（`sysdeps/unix/sysv/linux/pathconf.c:63-128` 的 `distinguish_extX`）。**返回值对 ≠ `errno` 干净**。
9. **`_PC_PATH_MAX` 不查文件系统** —— 它直接 `return PATH_MAX`（`sysdeps/posix/pathconf.c:89-93`）。所以本仓库扫 60 个挂载点，`_PC_PATH_MAX` 一律 `4096`，**不管底层是 tmpfs 还是 nfs**。
10. **`_PC_PIPE_BUF` 也不查文件系统** —— 直接 `return PIPE_BUF`（`sysdeps/posix/pathconf.c:96-100`），实测恒 `4096`。
11. **`_PC_LINK_MAX` 才会随文件系统变，而且值很分散** —— 实测：tmpfs `127`、ext4 `65000`、squashfs `127`。glibc 的常量表（`linux_fsinfo.h:255-268`）里 ext2/3 = `32000`、ext4 = `65000`、XFS = `2147483647`、F2FS = `32000`、未知 = `127`。**用 `127` 当「硬上限」会浪费 500 倍空间**。
12. **`pathconf("")` 是 `ENOENT`，不是 `EINVAL`** —— 实测 `errno=2`。空路径在 `sysdeps/posix/pathconf.c:32-36` 就被挡掉了（`if (path[0] == '\0')`）。
13. **`pathconf` 的错误码要分类读** —— 实测：`9999` → `EINVAL(22)`；`""` → `ENOENT(2)`；不存在的目录 → `ENOENT(2)`；**`/proc/version/inside`（中间是普通文件）→ `ENOTDIR(20)`**。
14. **`fpathconf` 的第一参数是 fd，所以失败是 `EBADF` 不是 ENOENT** —— 实测 `fpathconf(9999, _PC_NAME_MAX)` → `EBADF(9)`；`fpathconf(STDIN_FILENO, 9999)` → `EINVAL(22)`。
15. **`_PC_ASYNC_IO` 在 glibc 里其实是返回 `-1`（indeterminate）** —— `sysdeps/posix/pathconf.c:128-141` 有 `S_ISREG`/`S_ISBLK` 判断，但对普通文件返回 `1`、其它返回 `-1` 且不设 `errno`。实测 `pathconf("/")` 上 `_PC_ASYNC_IO` / `_PC_SYNC_IO` / `_PC_PRIO_IO` 三项都是 indeterminate。
16. **`_PC_VDISABLE` 不检查「这是不是终端」** —— 它直接返回 `_POSIX_VDISABLE`（`sysdeps/posix/pathconf.c:115-119`）。而 `_PC_MAX_CANON` / `_PC_MAX_INPUT` 在**规范**上要求 fd/path 必须是终端（man-pages 原文 "where fd or path must refer to a terminal"）—— 两者行为不一致。
17. **`_POSIX_FOO` 有四态，不是三态** —— 未定义 → 运行时问；`-1` → **不支持**；`0` → **函数和头文件存在，但支持程度要运行时问**；其它值（如 `200809L`）→ 支持，数字表示 POSIX 修订年月。glibc 还有个特例：修订版尚未发布时用 `1` 表示支持。
18. **`_POSIX_MONOTONIC_CLOCK` / `_POSIX_CPUTIME` / `_POSIX_THREAD_CPUTIME` 在 glibc 里都是 `0`** —— 不是「不支持」，是「存在但支持度要问」。所以 `sysconf(_SC_MONOTONIC_CLOCK)` 走的是 `linux/sysconf.c:51-54` 的 `return _POSIX_VERSION`（返回 `200809`），**不等于宏的值 0**。
19. **`confstr()` 和 `sysconf()` 不是同一回事** —— `confstr()` 返回**字符串**（`_CS_PATH`、`_CS_GNU_LIBC_VERSION`…），要靠返回值判长度：返回 `0` 表示「没有这个值」，返回 `>n` 表示缓冲区太小、需要多大就给你多大。实测 `_CS_PATH` 需 14 B → `"/bin:/usr/bin"`；`_CS_GNU_LIBC_VERSION` 11 B → `"glibc 2.39"`。
20. **`getconf` 命令是验证手段，但精简系统里常常没有** —— CE 沙箱里既无 `getconf` 也无 `/bin/sh`，所以本仓库的交叉验证一律靠**自编程序里打两遍**，不靠外部命令。
21. **`fd` 用满是 `EMFILE`，且硬上限抬不动** —— 实测 soft=50 时只能开到 **47** 个（0/1/2 已占），第 48 个 `errno=24 EMFILE`；`setrlimit` 把 soft 抬到 `hard+1` → `EINVAL(22)`。所以「`_SC_OPEN_MAX` 报 100 就敢开 100 个 fd」是错的。
22. **`_SC_NPROCESSORS_CONF` 会留下脏 `errno`** —— 实测它返回 `2` 但 `errno` 被留成 `2`（`ENOENT`），跟第 8 条同源。**判断「是不是错误」不能只看 `errno != 0`，必须先 `errno = 0` 再调**。

---

## 章节链路

```text
Ch2  进程 / 内核分工（谁提供这些数字）
  → Ch3  syscall 约定 + errno 范式（本章 -1 三义的判据全靠它）
  → Ch10 时间（_SC_CLK_TCK ↔ times() 的换算系数）
  → Ch11 系统限制与选项 —— 三个入口：
           编译期   <limits.h> / <unistd.h> 的 *_MAX 宏（标准给的是「下限保证」）
           运行时   sysconf() 全局数 / confstr() 字符串
           路径相关 pathconf() / fpathconf()（只有 4 项真问文件系统）
           └─ 三者共用一个 -1，靠 errno 区分「错误 / 不确定 / 不支持」
  → Ch12 /proc、uname、sysinfo（同一批数据的第二个入口）
  → Ch15 link / rename / mkdir 要用 NAME_MAX、PATH_MAX
  → Ch36 RLIMIT_*（_SC_OPEN_MAX 背后就是 RLIMIT_NOFILE）
  → Ch44 管道与 FIFO（_PC_PIPE_BUF 决定原子写边界）
  → Ch52/53/54 POSIX IPC（_SC_MQ_OPEN_MAX / _SC_SEM_NSEMS_MAX 的 indeterminate）
  → Ch62 终端（_PC_MAX_CANON / _PC_MAX_INPUT / _PC_VDISABLE）
```

---

## 双线提示

| 路线 | |
|------|--|
| **HFT** | 路径缓冲区**永远不要**按 `PATH_MAX` 写死——它只是「相对路径的最大长度」，拼接后的绝对路径可以远超；`_PC_PIPE_BUF` 实测恒 `4096`，也就是说**管道上超过 4 KiB 的一次 `write` 不保证原子**，多线程写同一条管道必须先自己分片；`_SC_OPEN_MAX` 要**在改过 `RLIMIT_NOFILE` 之后重查**（它不是常量），行情网关进程开几万个连接前先 `setrlimit` 再 `sysconf` 复核；`_SC_ARG_MAX` 报 2 MiB 而 `execve` 实际 128 KiB 就 `E2BIG`，**热重启子进程时把大配置塞进 argv 会炸**，改用 stdin / 临时文件；`_POSIX_MONOTONIC_CLOCK == 0` 别误读成「不支持 MONOTONIC 时钟」——要 `sysconf(_SC_MONOTONIC_CLOCK)` 看 `_POSIX_VERSION` |
| **嵌入式** | 交叉编译时 `_SC_*` / `_PC_*` 的**运行时值才是真的**，宿主机上查到的数字一个都不能带到板子上；只读根文件系统 + tmpfs `/tmp` 的组合会让 `pathconf` 在同一台设备上**按路径给出不同答案**（这正是 11-2 习题的考点）；BusyBox 精简系统里**没有 `getconf`**，程序里要么自带 `sysconf` 探测、要么读 `/proc`；老 uClibc / musl 对 `_SC_*` 的覆盖面比 glibc 小得多，遇到 `EINVAL` 要能降级到编译期宏；`_POSIX_VDISABLE` 在串口控制台上的行为跟 PC 终端不一样，写 TTY 初始化代码前先 `pathconf` 确认 |
| **两条线共同的坑** | 「未定义就当成 0」——`<limits.h>` 里 `ARG_MAX` / `OPEN_MAX` / `LINK_MAX` / `NR_OPEN` 都被 glibc **主动 `#undef`**（注释写在 `bits/local_lim.h`：这些值运行时可变，给个宏会误导）。用之前一律 `#ifdef`，并且**不要把「未定义」当「不支持」** |

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 三个入口：**编译期宏**（下限保证）/ **`sysconf`（全局）** / **`pathconf`·`fpathconf`（路径相关）**；字符串走 **`confstr`** |
| 2 | `-1` 三义：**错误**（`errno != 0`）/ **indeterminate**（`errno == 0`）/ **选项不支持**（`-1` 就是答案）。**先 `errno = 0`** |
| 3 | `sysconf` 不认识的常量 → `EINVAL`(22)，出处 `sysdeps/posix/sysconf.c:64-66` 的 `default:` |
| 4 | glibc 四级分派：**x86 特化 → `linux_sysconf` → `posix_sysconf` → `default: EINVAL`** |
| 5 | `_SC_OPEN_MAX` = `getrlimit(RLIMIT_NOFILE).rlim_cur`，**改 rlimit 它立刻变** |
| 6 | `_SC_CLK_TCK` 恒 100 = `USER_HZ`，与 `CONFIG_HZ` 无关 |
| 7 | `_SC_ARG_MAX` 是**名义值**（`MAX(131072, rlim_stack/4)`，封顶 6 MiB）；真实墙是 `MAX_ARG_STRLEN = 128 KiB` |
| 8 | man-pages 明说 `ARG_MAX` **难以使用**（环境变量占多少没规定），且「返回值可能大得不能拿来分配内存」 |
| 9 | `pathconf` 里**只有 4 项**真问文件系统：`_PC_LINK_MAX` / `_PC_FILESIZEBITS` / `_PC_2_SYMLINKS` / `_PC_CHOWN_RESTRICTED` |
| 10 | `_PC_PATH_MAX` → `return PATH_MAX`(4096)；`_PC_PIPE_BUF` → `return PIPE_BUF`(4096)。**都不查文件系统** |
| 11 | `_PC_NAME_MAX` 走 `statvfs64` 的 `f_namemax`（实测 tmpfs 255 / ext4 255 / squashfs 256）；`ENOSYS` 才退到 `NAME_MAX` |
| 12 | `_PC_LINK_MAX` 实测 tmpfs **127** / ext4 **65000**；glibc 表里 XFS 是 `2147483647` |
| 13 | 返回值对**不代表** `errno` 干净：`pathconf("/lib", _PC_LINK_MAX)` 返回 65000 而 `errno=2` |
| 14 | `pathconf("")` → `ENOENT`(2)；中间段是普通文件 → `ENOTDIR`(20)；未知常量 → `EINVAL`(22) |
| 15 | `fpathconf` 失败是 `EBADF`(9)（fd 非法）/ `EINVAL`（常量非法） |
| 16 | `_POSIX_FOO` **四态**：未定义=运行时问 / `-1`=不支持 / `0`=存在但支持度要问 / 其它值=支持（数字是修订年月） |
| 17 | glibc 里 `_POSIX_MONOTONIC_CLOCK` = `_POSIX_CPUTIME` = `_POSIX_THREAD_CPUTIME` = **`0`**（不是 -1） |
| 18 | `confstr` 靠返回值判长度：`0` = 没有该值；`> n` = 缓冲区不够，返回所需大小 |
| 19 | `<limits.h>` 的 `ARG_MAX` / `OPEN_MAX` / `LINK_MAX` / `NR_OPEN` 被 glibc **主动 `#undef`** |
| 20 | 把 soft 抬到 `hard+1` → `EINVAL`(22)；fd 用满 → `EMFILE`(24) |
| 21 | indeterminate 清单（CE 实测）：sysconf 侧 **4 项**（`_SC_TZNAME_MAX` / `_SC_SYMLOOP_MAX` / `_SC_MQ_OPEN_MAX` / `_SC_SEM_NSEMS_MAX`）；`pathconf("/")` 侧 **3 项**（`_PC_ASYNC_IO` / `_PC_SYNC_IO` / `_PC_PRIO_IO`） |
| 22 | 原书本章只有 **2 个**示例文件，都在 `syslim/`：`t_sysconf.c`（**p.216**）/ `t_fpathconf.c`（**p.218**） |

---

## 参考

- Kerrisk, *The Linux Programming Interface*, **Chapter 11 — System Limits and Options**
- [man7 官方源码清单（按章）](https://man7.org/tlpi/code/online/all_files_by_chapter.html) · [OUTLINE](../OUTLINE.md) · [Ch10 时间](../chapter-10-time/README.md) · [Ch12 系统与进程信息](../chapter-12-system-process-info/README.md)
- man-pages **6.19**：`sysconf(3)`、`pathconf(3)`、`fpathconf(3)`、`confstr(3)`、`getconf(1)`、`posixoptions(7)`、`limits(7)`
- POSIX.1-2017：`<limits.h>`、`<unistd.h>`、`<sysconf>`（其中 `<sysconf>` 明确「The symbol `CLK_TCK` is obsolescent and removed.」）
- 内核源码（**v6.6**）：`include/uapi/linux/limits.h`（`NR_OPEN`/`ARG_MAX`/`LINK_MAX`/`NAME_MAX`/`PATH_MAX`/`PIPE_BUF`/`MAX_CANON`/`MAX_INPUT`/`RTSIG_MAX`）、`include/uapi/asm-generic/resource.h`（`RLIMIT_NOFILE`）、`include/uapi/linux/resource.h`（`_STK_LIM`）、`include/uapi/linux/binfmts.h`（`MAX_ARG_STRLEN`/`MAX_ARG_STRINGS`）、`fs/exec.c`（`valid_arg_len`/`bprm_stack_limits`/`copy_string_kernel`）、`fs/pipe.c`（`pipe_max_size`/`pipe_write`）
- glibc **2.39**：`sysdeps/unix/sysv/linux/x86/sysconf.c`、`sysdeps/unix/sysv/linux/sysconf.c`、`sysdeps/unix/sysv/linux/sysconf-sigstksz.h`、`sysdeps/posix/sysconf.c`、`sysdeps/posix/getdtsz.c`、`sysdeps/unix/sysv/linux/getdtsz.c`、`sysdeps/unix/sysv/linux/getclktck.c`、`sysdeps/unix/sysv/linux/pathconf.c`、`sysdeps/unix/sysv/linux/fpathconf.c`、`sysdeps/unix/sysv/linux/linux_fsinfo.h`、`sysdeps/posix/pathconf.c`、`posix/confstr.c`、`sysdeps/unix/sysv/linux/bits/posix_opt.h`、`posix/bits/posix1_lim.h`

---

## 代码示例

本章 `code/` 下有：

- **7 个自编 demo**（`c11_*.c` 5 个 + `ex11_*.c` 2 个）
- **2 个原书文件**逐字镜像（`t_sysconf.c` = Listing 11-1、`t_fpathconf.c` = Listing 11-2）
- **1 个框架替身** `tlpi_hdr.h`（原书 `lib/tlpi_hdr.h` 的最小可用子集，只含 `errExit` / `fatal` / `usageErr`）

全部在 Compiler Explorer（gcc 13.3.0 / x86-64 / Ubuntu 24.04）上真实编译 + 运行过，共 **9 个作业**，全部 `build code = 0` / `didExecute = True` / `diagnostics = 0`。输出原样抄在对应笔记的实测块里。完整索引见 [`code/README.md`](code/README.md)。

| 文件 | 对应节 | 演示什么 | 需要什么 |
|------|--------|----------|---------|
| `c11_1_three_kinds.c` | 11.1 | 三类限制现场对照：恒定 / 路径相关 / 可增；`setrlimit` 把 soft 从 100 降到 50 → `_SC_OPEN_MAX` **立刻**变 50；10 行 `_POSIX_*` 下限宏 vs 实测值对照表 | — |
| `c11_2_sysconf_table.c` | 11.2 | `sysconf()` 全表（常量类 + 限制类 28 项 + 选项类）+ `-1` 三义现场：`_SC_TZNAME_MAX` indeterminate、`sysconf(9999)` → `EINVAL` | — |
| `c11_3_pathconf_matrix.c` | 11.3 | 三种维度矩阵（挂载点 × `_PC_*` × 目录 / 普通文件 / fd）；`/`(tmpfs)=`LINK_MAX 127` vs `/lib`(ext4)=`65000`；`_PC_PATH_MAX` 恒 4096 的真相；四条错误路径 | — |
| `c11_4_indeterminate.c` | 11.4 | indeterminate 清单（sysconf 4 项 + pathconf 3 项）+ 降级阶梯（`statvfs` → `#ifdef NAME_MAX` → 兜底） | — |
| `c11_5_options.c` | 11.5 | `_POSIX_FOO` 四态对照 + `confstr()` 三连：`_CS_PATH` → `/bin:/usr/bin`、`_CS_GNU_LIBC_VERSION` → `glibc 2.39`、`_CS_GNU_LIBPTHREAD_VERSION` → `NPTL 2.39` | — |
| `ex11_1_sysconf_wide.c` | 11.7 练习 11-1 | 把原书 Listing 11-1 加宽：`sysconf` 侧与 `pathconf` 侧一次打全，看「换一台机器哪些数字会变」 | — |
| `ex11_2_fs_sweep.c` | 11.7 练习 11-2 | 扫全部挂载点：实测 **60 个**挂载点 → tmpfs **31** / ext4 **10** / squashfs **19**，三种文件系统三种 `NAME_MAX` / `LINK_MAX` | — |
| `t_sysconf.c` | 11.2 | **原书 Listing 11-1（p.216）**：六项 `sysconf` 标准写法（`errno=0` → 调 → 三分支） | `tlpi_hdr.h` |
| `t_fpathconf.c` | 11.3 | **原书 Listing 11-2（p.218）**：三项 `fpathconf(STDIN_FILENO, ...)` | `tlpi_hdr.h` + stdin 需可读 |

一次编完全部自编 demo（在 `code/` 目录下）：

```bash
for f in c11_*.c ex11_*.c; do
    gcc -O0 -Wall -Wextra -o "${f%.c}" "$f" || echo "FAIL $f"
done
```

2 个原书程序都需要 `tlpi_hdr.h` 替身（且原书 `main(argc, argv)` 不用这两个参数，原书 Makefile 只开 `-Wall`，我们额外加 `-Wextra` 时要带 `-Wno-unused-parameter`）：

```bash
for f in t_sysconf t_fpathconf; do
    gcc -O0 -Wall -Wextra -Wno-unused-parameter -o "$f" tlpi_hdr.h "$f.c" || echo "FAIL $f"
done
```

运行示例：

```bash
./c11_1_three_kinds        # 三类限制 + rlimit 现场改动
./c11_2_sysconf_table      # 全表 + -1 三义
./c11_3_pathconf_matrix    # 挂载点矩阵 + 四条错误路径
./c11_4_indeterminate      # indeterminate 清单 + 降级阶梯
./c11_5_options            # 四态 + confstr
./ex11_1_sysconf_wide      # 加宽版 Listing 11-1
./ex11_2_fs_sweep          # 扫全部挂载点（需 /proc/mounts 可读）
./t_sysconf                # 原书 Listing 11-1
./t_fpathconf < /proc/mounts   # 原书 Listing 11-2（fd 0 需指向一个真实文件）
```

**几条实测结论**（都是本仓库跑出来的，不是书上抄的）：

- **`_SC_OPEN_MAX` 会跟着 rlimit 变**：`RLIMIT_NOFILE` soft=hard=100 时 `_SC_OPEN_MAX = 100`；把 soft 降到 50 之后**立刻**变 50 —— 「值在进程生命期内不变」对它不成立
- **`_SC_ARG_MAX = 2097152`（名义）vs `execve` 128 KiB 就 `E2BIG`**：65536 字节 OK，131072 字节 `errno=7`。差 16 倍
- **`_SC_CLK_TCK = 100`、`_SC_PAGESIZE = 4096`、`_SC_VERSION = 200809`**，改 rlimit 前后一字不变
- **下限宏 vs 实测：`_POSIX_OPEN_MAX 20 → 100`、`_POSIX_CHILD_MAX 25 → 54956`、`_POSIX_GROUPS_MAX 8 → 65536`、`_POSIX_STREAM_MAX 8 → 16`、`_POSIX_LOGIN_NAME_MAX 9 → 256`、`_POSIX_TTY_NAME_MAX 9 → 32`、`_POSIX_NAME_MAX 14 → 255`、`_POSIX_PATH_MAX 256 → 4096`、`_POSIX_PIPE_BUF 512 → 4096`** —— 标准给的是「保证」，实际常常宽 2000 倍
- **indeterminate 实测清单**：sysconf 侧 **4 项** `_SC_TZNAME_MAX` / `_SC_SYMLOOP_MAX` / `_SC_MQ_OPEN_MAX` / `_SC_SEM_NSEMS_MAX`；`pathconf("/")` 侧 **3 项** `_PC_ASYNC_IO` / `_PC_SYNC_IO` / `_PC_PRIO_IO`
- **`_PC_LINK_MAX` 按文件系统分三档**：tmpfs **127** / ext4 **65000** / squashfs **127**；`_PC_NAME_MAX` 是 tmpfs **255** / ext4 **255** / squashfs **256**
- **`_PC_PATH_MAX` 恒 4096、`_PC_PIPE_BUF` 恒 4096**，跨 60 个挂载点无一例外 —— 因为它们是编译期常量，不查文件系统
- **错误码实测**：`sysconf(9999)` → `EINVAL(22)`；`pathconf("/", 9999)` → `EINVAL(22)`；`pathconf("")` → `ENOENT(2)`；`pathconf("/proc/version/inside")` → `ENOTDIR(20)`；`fpathconf(9999, …)` → `EBADF(9)`
- **`confstr` 三连**：`_CS_PATH` 需 14 B → `"/bin:/usr/bin"`；`_CS_GNU_LIBC_VERSION` 11 B → `"glibc 2.39"`；`_CS_GNU_LIBPTHREAD_VERSION` 10 B → `"NPTL 2.39"`
- **`_SC_2_FORT_DEV = -1`**（不支持），而 `_SC_2_VERSION = 200809`、`_SC_XOPEN_VERSION = 700`
- **fd 用满**：soft=50 时只能开 **47** 个（0/1/2 已占），第 48 个 `EMFILE(24)`；把 soft 抬到 `hard+1` → `EINVAL(22)`
- **脏 `errno` 两例**：`pathconf("/lib", _PC_LINK_MAX)` 返回正确的 65000 但 `errno=2`；`_SC_NPROCESSORS_CONF` 返回 2 但 `errno=2`

> ⚠️ **会漂移的数字**：`_SC_CHILD_MAX` / `_SC_SIGQUEUE_MAX`（两次采样 **54956** / **56783**）、`_SC_AVPHYS_PAGES`、`_SC_MINSIGSTKSZ` / `_SC_SIGSTKSZ`、`ex11_2` 的挂载点计数（**60 / 31 / 10 / 19**）都跟当时机器状态有关，**重跑会是另一个数**。沙箱环境本身也特殊：无 `/bin/sh`、无 `getconf`、`RLIMIT_NOFILE` 只有 100。
