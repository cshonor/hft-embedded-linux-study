# Ch05 `code/` 目录说明

TLPI 第 5 章（File I/O: Further Details）的可编译代码。分三类：

| 类别 | 数量 | 命名 |
|------|------|------|
| 自编 demo | **17** | `c5_<节>_<名字>.c`（12 个）+ `ex5_<n>_<名字>.c`（5 个） |
| 原书镜像 | **6** | `bad_exclusive_open.c`(5-1)、`t_readv.c`(5-2)、`large_file.c`(5-3)、`atomic_append.c`(习题 5-3)、`multi_descriptors.c`(习题 5-6)、`t_truncate.c`，**逐字保真** |
| 框架替身 | **1** | `tlpi_hdr.h` |

**全部 35 个作业**（17 自编 + 6 个原书程序的多组调用姿势）都在 Compiler Explorer（gcc 13.3.0 / x86-64 / Ubuntu 24.04）上跑过：`build code = 0`、`didExecute = True`、`diagnostics = 0`。

> 为什么走 Compiler Explorer：本机环境里 `wsl.exe` 被安全策略禁用，且没有任何 C 编译器（`gcc`/`clang`/`tcc`/`cl`/`cc`/`zig` 全无）。CE 提供真实的 gcc 13.3 编译诊断与真实运行输出；笔记里凡引用输出都标注「CE 实测」，**不当成本机实测**。
>
> ⚠️ CE 是**一次性容器**，`/tmp` 每次都是新的。所以 `tmpfile()` 显示 `/tmp/#4`、`O_TMPFILE` 显示 `/tmp/#5`——这些 **inode 号会随环境变**，不要当常量写进断言。

---

## 文件表

### 自编 demo（17）

| 文件 | 对应节 | 演示什么 | 需要什么 |
|------|--------|----------|---------|
| `c5_1_atomicity.c` | 5.1 | ① `O_APPEND` 两进程各写 20 万字节 → `st_size = 400000`（一个不丢）；② 同样负载改用 `lseek+write` → `391764`（**丢 8236**）；③ 无 `O_EXCL` 的「先查后建」→ 两个进程**都**打印「我创建了它」；④ `O_CREAT\|O_EXCL` → 唯一赢家 + 输家 `errno=17 (EEXIST)` | `fork` |
| `c5_2_fcntl.c` | 5.2 | `F_GETFL`/`F_SETFL`（含内核补的 `0x8000`）；`F_GETFD`/`F_SETFD`；`F_DUPFD(50)`/`F_DUPFD_CLOEXEC(60)`；`F_SETOWN(getpid())` → `F_GETOWN`；未知命令 `EINVAL(22)` vs `fd=999` 的 `EBADF(9)` | — |
| `c5_3_status_flags.c` | 5.3 | **两类标志分层**；`F_SETFL(O_APPEND)` 生效 / `F_SETFL(O_ASYNC)` **普通文件上静默失败**（管道上成功，`0x02000`）；`F_SETFL(O_CREAT\|O_TRUNC\|O_EXCL)` **返回 0 但一位没变**；访问模式改不了；`dup` 出来的 fd 共享状态标志 | — |
| `c5_4_fd_open_files.c` | 5.4 | 复刻原书习题 5-6 的全部过程（不依赖 `system()`）：`fd1=3`(open) / `fd2=4`(dup) / `fd3=5`(open)，六轮打印「文件内容 + 三个偏移」；`st_ino` 两者都是 `2`；只对 fd1 设 `O_APPEND` 时 fd3 的 `F_GETFL` 不受影响 | — |
| `c5_5_dup.c` | 5.5 | `dup` 取最小可用号（`close(0)` 后返回 **`0`**）；`dup2(fd,7)` 共享偏移；`dup2(fd,fd) -> 3` vs `dup2(999,999) -> EBADF`；`dup3(fd,fd,0) -> EINVAL(22)`；`dup` 不带 `CLOEXEC` / `dup3(...,O_CLOEXEC)` 带（`F_GETFD=0x1`）；三条错误路径 | — |
| `c5_6_pread_pwrite.c` | 5.6 | `pread(fd,4,0)` 后 `f_pos` 仍 `10` vs `lseek(0)+read(4)` 后变 `4`；`pwrite(fd,"xyz",3,5)` 后 `f_pos` 仍 `10`；`pread(offset=1000)`（文件 10 B）→ `0`；`pwrite(offset=1000)` → `st_size=1001`/物理 `4096`；管道上 `pread` → `ESPIPE(29)`；同一 fd 并发读三段互不干扰 | — |
| `c5_7_readv_writev.c` | 5.7 | 一次 `writev` 写三段（`108` B）；一次 `readv` 读回 + `memcmp`；`iovcnt` 小于数组长度；`sysconf(_SC_IOV_MAX) = 1024`；管道压到 4096 后 `writev` 请求 4096 → `EAGAIN` | — |
| `c5_8_truncate.c` | 5.8 | 26 B → `ftruncate(10)`；撑到 `8192`（物理**仍** `4096`，多出的是**洞**）；**对照**：真写满 `8192`（物理变 `8192`，**2 倍**）；`truncate(path,5)`；四条失败路径（`ENOENT`/`EINVAL`/`EISDIR`/负长度）；截短后 `lseek` 到 100 再读 → `0` | — |
| `c5_9_nonblocking.c` | 5.9 | 空管道 + `O_NONBLOCK` → `EAGAIN(11)`（与 `EWOULDBLOCK` **同值**）；清掉位后阻塞读被 `SIGALRM` 打断 → `EINTR(4)`；有数据立刻返回；写满 `65536` 后 `EAGAIN` | — |
| `c5_10_large_files.c` | 5.10 | `sizeof(off_t)=8`；`RLIMIT_FSIZE` soft/hard；`lseek(4 GiB)` **成功但文件不变大**；在 4 GiB 处 `write`：默认动作 = 被 `signal 25 (SIGXFSZ)` 杀；装 handler 后 `errno=27 (EFBIG)`；**`0x8000` 之谜**的两侧源码解释 | `fork` |
| `c5_11_dev_fd.c` | 5.11 | `stat("/dev/fd")` → `ENOENT`（沙箱没有）→ 全程改走 `/proc/self/fd`；`readlink` 看真名；**`open("/proc/self/fd/3")` 得到新表项**（读 4 B 后原 fd `f_pos` **仍是 0**）↔ 对照 `dup(3)` 的最小可用号；`unlink` 后 `readlink` 显示 `... (deleted)`、仍能重新打开并读回 `"0123"` | — |
| `c5_12_temp_files.c` | 5.12 | `mkstemp`：模板**就地改写**（`"/tmp/c5tmpXXXXXX"` → `"/tmp/c5tmpEE7wgl"`）、权限 `0600`、`unlink` 后 fd 仍可读；`mkostemp(O_CLOEXEC)` → `F_GETFD=0x1`；`tmpfile()` → `/tmp/#4 (deleted)`；`O_TMPFILE` → `/tmp/#5 (deleted)` + `linkat` 挂名后读回 `"anonymous!"` | `_GNU_SOURCE` |
| `ex5_1_large_file_offset_bits.c` | 5.14 练习 5-1 | 把 Listing 5-3 改写成标准接口 + `_FILE_OFFSET_BITS=64`：打印 `_FILE_OFFSET_BITS` / `sizeof(off_t)` / `RLIMIT_FSIZE`；`lseek(4 GiB)` 成功、`write` 被 rlimit 拦（`EFBIG` + `SIGXFSZ` handler 计数） | — |
| `ex5_2_append_seek.c` | 5.14 练习 5-2 | A 组带 `O_APPEND`：`lseek(0)` **确实**把 `f_pos` 变 0，但写完变 `16`、数据**仍在末尾**；B 组不带：同样操作从偏移 0 **覆盖**（文件不变长） | — |
| `ex5_4_my_dup.c` | 5.14 练习 5-4 | 用 `fcntl(fd, F_DUPFD, …)` + `close()` 实现 `my_dup`/`my_dup2`；与真 `dup`/`dup2` 对照（共享偏移、错号）；`old==new` 特例（`my_dup2(3,3) -> 3`、`my_dup2(999,999) -> EBADF`）；目标号被 `/dev/null` 占用时先关再复制 | — |
| `ex5_5_shared_offset_flags.c` | 5.14 练习 5-5 | 三段验证：偏移共享（读 3 B 后 fd2 跟着动、fd3 不动）；状态标志共享（只对 fd2 设 `O_APPEND`，fd1 一起变）；**`FD_CLOEXEC` 不共享**（层 ① vs 层 ②）；末尾小结三层各管什么 | — |
| `ex5_7_readv_by_read.c` | 5.14 练习 5-7 | 用 `read`/`write` + `malloc` 实现 `my_readv`/`my_writev`；与真 `writev` 逐字节对照（`memcmp -> 完全相同`）；**短读时不越界**（请求 15 只返回 3，第二段首字节仍是 `0x7f`）；末尾指出「1 次 malloc + 1 次系统调用」的代价 | — |

### 原书镜像（6，逐字保真）

| 文件 | 原书位置 | 说明 | 需要什么 |
|------|---------|------|---------|
| `bad_exclusive_open.c` | **Listing 5-1, p.90** | 反面教材：`open(O_CREAT\|O_EXCL)` 失败后退回**普通 `open`**，于是「谁真正创建的」无从判断。实测传 `/dev/null` → `File "/dev/null" already exists`；传新路径 → `doesn't exist yet` + `Created file ... exclusively` | `tlpi_hdr.h` |
| `t_readv.c` | **Listing 5-2, p.101** | `readv` 读「`struct stat` + 100 字节」：`totRequired = sizeof(struct stat) + 100 = 248`；打印 `total bytes requested: N; bytes read: N`，短读时先打印 `Read fewer bytes than requested` | `tlpi_hdr.h` |
| `large_file.c` | **Listing 5-3, p.105** | 已废弃的 LFS API：`#define _LARGEFILE64_SOURCE` + `open64()`/`off64_t`/`lseek64()`。在 CE 沙箱里被 `SIGXFSZ` 杀掉（因为 `RLIMIT_FSIZE = 16 MiB`） | `tlpi_hdr.h` |
| `atomic_append.c` | **习题 5-3 解答, p.110** | `file num-bytes [x]`；**一次只写 1 字节**；`x` 时改用 `lseek(fd,0,SEEK_END)` + `write`。两行核心：`bool useLseek = argc > 3;` / `int flags = useLseek ? 0 : O_APPEND;` | `tlpi_hdr.h` / `getInt` |
| `multi_descriptors.c` | **习题 5-6 解答, p.111** | 三个描述符（`fd2 = dup(fd1)`、`fd3 = open(file)`）交替读写的现场直播，每次写后 `system("cat a; echo")`。**沙箱里无输出**——它要 `fork+exec` 一个 `/bin/sh`，而沙箱没有（`system()` 返回 `32512`） | `tlpi_hdr.h` |
| `t_truncate.c` | 无 listing 编号（原书分发文件） | `t_truncate file length`，`truncate` 的命令行工具。实测 `t_truncate /proc/version 10` → `truncate: Permission denied`（`EACCES`） | `tlpi_hdr.h` / `getLong` |

> 页码出处：man7 单文件页原文（`This is fileio/bad_exclusive_open.c (Listing 5-1, page 90), an example from the book, The Linux Programming Interface.` 等）。
> 镜像来源：`https://man7.org/tlpi/code/online/dist/fileio/<file>`（返回**原始源文件**，含 GPL 头与原作者注释）。
> 文件清单出处：[man7 — List of source code files, by chapter](https://man7.org/tlpi/code/online/all_files_by_chapter.html) 的 **Chapter 5** 一节——共 6 个文件。

⚠️ **`multi_descriptors.c` 与习题 5-6 的一处差异（诚实标注）**：习题题干里写 `write(fd2, " world", 6)`（6 字节，含前导空格），部分网上转载写成 `write(fd2, "world", 6)`（丢了空格）。**本仓库镜像的是 man7 官方分发版**，即 `" world"`。差别在最终内容上：带空格是 `"Gidday world"`，不带空格则第 12 字节是 `'\0'`、显示为 `"Giddayworld"`。

### 框架替身（1）

| 文件 | 说明 |
|------|------|
| `tlpi_hdr.h` | 原书 `lib/tlpi_hdr.h` 的**最小可用子集**，按原书语义实现了 6 个助手：`errExit` / `fatal` / `usageErr` / `cmdLineErr`（`error_functions.c`）+ `getInt` / `getLong`（`get_num.c`，含 `GN_*` 旗标）。原书版依赖整个 `lib/` 目录，无法单文件编译 |

> ⚠️ 这份 `tlpi_hdr.h` 与 Ch04 的同名替身**不能互换**：本章的 `atomic_append.c` 需要 `getInt`（Ch04 那版只有 `getLong`）。
> ⚠️ 本章的 `t_readv.c` 需要 `struct stat` 大小来算 `totRequired`，**它是 248 的前提是 `sizeof(struct stat) = 144`**（x86-64 上）。换平台这个数字会变。

---

## 编译

一次编完 17 个自编 demo（在 `code/` 目录下）：

```bash
for f in c5_*.c ex5_*.c; do
    gcc -O0 -Wall -Wextra -o "${f%.c}" "$f" || echo "FAIL $f"
done
```

6 个原书程序都需要 `tlpi_hdr.h`（**同目录即可**，在 `code/` 里直接编）：

```bash
for f in bad_exclusive_open t_readv large_file atomic_append multi_descriptors t_truncate; do
    gcc -O0 -Wall -Wextra -o "$f" "$f.c" || echo "FAIL $f"
done
```

`large_file.c` 在 CE 上是用这条命令编的（显式给 `_LARGEFILE64_SOURCE`）：

```bash
gcc -O0 -Wall -Wextra -D_LARGEFILE64_SOURCE= -o large_file large_file.c
```

> `multi_descriptors.c` 用了未使用的 `argv`，加 `-Wall -Wextra` 会告警，CE 上补了 `-Wno-unused-parameter`（原书代码，**不改**）。

## 运行示例

```bash
./c5_1_atomicity                   # 并发对照（会 fork 两个子进程）
./c5_2_fcntl
./c5_3_status_flags                # O_ASYNC 的静默失败在这里
./c5_4_fd_open_files               # 习题 5-6 的无 system() 版
./c5_5_dup
./c5_6_pread_pwrite
./c5_7_readv_writev
./c5_8_truncate                    # 洞的物理占用对照
./c5_9_nonblocking
./c5_10_large_files                # SIGXFSZ 在这里
./c5_11_dev_fd
./c5_12_temp_files                 # mkstemp / tmpfile / O_TMPFILE

./ex5_1_large_file_offset_bits /tmp/c5_lfs.bin 4294967296
./ex5_2_append_seek
./ex5_4_my_dup
./ex5_5_shared_offset_flags
./ex5_7_readv_by_read

# 原书程序
./bad_exclusive_open /dev/null                  # Listing 5-1：已存在
./bad_exclusive_open /tmp/c5_excl_new           # Listing 5-1：创建成功
./t_readv /proc/self/maps                       # Listing 5-2：248 / 248
./t_readv /proc/version                         # Listing 5-2：248 / 219（短读）
./large_file /tmp/c5_large.bin 4294967296       # Listing 5-3
./t_truncate /proc/version 10                   # Permission denied
./multi_descriptors                             # 沙箱里无输出（无 /bin/sh）

# 习题 5-3 必须并发跑两个实例
./atomic_append /tmp/f1.bin 200000 & ./atomic_append /tmp/f1.bin 200000
./atomic_append /tmp/f2.bin 200000 x & ./atomic_append /tmp/f2.bin 200000 x
```

## 沙箱环境注意（这些「失败」是环境限制，不是代码 bug）

| 现象 | 原因 |
|------|------|
| `stat("/dev/fd")` → `ENOENT` | 它只是 `MAKEDEV` 建的符号链接，CE 沙箱没建。`c5_11_dev_fd.c` 检测到就改走 `/proc/self/fd` |
| `tmpfile()` 显示 `/tmp/#4 (deleted)`、`O_TMPFILE` 显示 `/tmp/#5` | 正常现象（`#<ino>` 是 `d_tmpfile()` 编的展示名），但**数字随环境变** |
| `write` 在 4 GiB 处返回 `EFBIG` | `RLIMIT_FSIZE` 被限成 `16777216`（16 MiB）。**这条恰好演示了「拦你的是 rlimit 不是 `off_t` 宽度」** |
| `large_file.c` 被 `SIGXFSZ (25)` 杀 | 同上——它没装 handler，所以直接吃默认动作 |
| `t_truncate /proc/version 10` → `Permission denied` | `/proc/*` 只读。这是**预期的失败路径**（`EACCES`），不是 bug |
| `multi_descriptors` 零输出 | 它靠 `system("cat a; echo")`，而沙箱**没有 `/bin/sh`**（`system()` 返回 `32512`）。想复现请看 `c5_4_fd_open_files.c` |
| 管道容量要显式调小才会 `EAGAIN` | 沙箱默认 `F_GETPIPE_SZ = 65536`（官方 Linux 默认值）。`c5_7`/`c5_9` 里用 `F_SETPIPE_SZ(4096)` 压小才方便观察 |
| `tmpfile()` / `O_TMPFILE` 成功 | 沙箱 `/tmp` 是 **tmpfs**，支持 `O_TMPFILE`。换成不支持的文件系统，glibc 会自动退到「`mkstemp`+`unlink`」（`stdio-common/tmpfile.c:47-61`） |
| 并发数字每次不同 | `c5_1_atomicity.c` 的「丢 8236」是**某一次**的调度交错结果，不是常量。原书同款程序的量级差异也因机器而异 |
