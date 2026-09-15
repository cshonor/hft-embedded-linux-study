# TLPI 第 04 章 — File I/O: The Universal I/O Model

**优先级**：🔴 必读（UNIX/Linux 一切 I/O 的地基，后面 60 章都在复用这套模型）
**前置**：[Ch2 基本概念](../chapter-02-basic-concepts/README.md)（fd / 进程与内核的分工）· [Ch3 系统编程概念](../chapter-03-system-programming-concepts/README.md)（syscall 与 `errno` 范式）
**后置**：[Ch5 文件 I/O 深入](../chapter-05-file-io-further/README.md)（`fcntl` / `dup` / `pread` / `readv`）→ [Ch13 文件 I/O 缓冲](../chapter-13-file-io-buffering/README.md)

---

## 小节目录

- [4.1 Overview 概览](notes/4.1-overview.md)
- [4.2 通用 I/O 模型（核心思想）](notes/4.2-universality.md)
- [4.3 `open()`](notes/4.3-open.md)
- [4.4 `read()`](notes/4.4-read.md)
- [4.5 `write()`](notes/4.5-write.md)
- [4.6 `close()`](notes/4.6-close.md)
- [4.7 `lseek()`](notes/4.7-lseek.md)
- [4.8 `ioctl()`](notes/4.8-ioctl.md)
- [4.9 Summary 本章总结](notes/4.9-summary.md)
- [4.10 Exercises 练习](notes/4.10-exercises.md)

> 十节的划分与 TLPI 原书一致（4.1–4.10）。原书本章**没有子编号小节**，所以一篇对应一节。
>
> **注意 4.10 的性质**：原书第 4 章末只有**两道**习题（**4-1** 实现 `tee`、**4-2** 保留空洞的 `cp`），本节的 4-3 是自编的（亲手制造部分写），另附两道无需写代码的巩固题。

---

## 章节目标

- **模型**：UNIX 的「一切皆文件」不是修辞，而是**同一个 `struct file_operations` 接口**被普通文件、设备、管道、socket 各自实现
- **动词**：`open` / `read` / `write` / `close` 四个动词 + `lseek` 定位 + `ioctl` 兜底——**全部 I/O 都由这六个组成**
- **层次**：用户态拿到的是整数 `fd`，中间是**进程私有的槽位表**，底下才是内核的 `struct file`（偏移、标志、引用计数）与 inode
- **诚实**：短读、部分写、`write` 成功≠落盘、`close` 不幂等——这些「反直觉」才是本章真正要带走的东西
- **溯源**：本章 **13 处**「书上的说法 / 网上的说法」被 v6.6 源码 + CE 实测修正（详见 [4.9 §反直觉清单](notes/4.9-summary.md)）

### 一条主线

| 层 | 是什么 | 谁拥有 | 关键字段 |
|----|--------|--------|---------|
| ① 进程 fd 表 | `int` 号 → 槽位 | 进程（`files_struct`） | 槽位号 = fd |
| ② 打开文件描述 | `struct file` | **可以多进程/多 fd 共享** | `f_mode` / `f_pos` / `f_flags` / `f_count` |
| ③ inode | 文件本体 | 全局（inode 缓存） | `i_size` / `i_mode` / 块映射 |

**读法**：同一行（`dup` / `fork`）→ 共享 ②（**共享偏移**）；不同行（各自 `open`）→ 各自的 ②（**各自偏移**），但可能指向同一个 ③。

### 与 Ch5 边界（防混淆）

| 章 | 主题 | 内容 |
|----|------|------|
| **Ch4** | 通用 I/O 模型 | **只用** `open`/`read`/`write`/`close`/`lseek`/`ioctl` 六个调用，把「一切对象」走一遍 |
| **Ch5** | Further Details | 同样的模型，换**更强的手段**：`fcntl`（改标志/锁）、`dup`（复制 fd）、`pread`/`pwrite`（原子定位读写）、`readv`/`writev`（批量）、`O_NONBLOCK` |

Ch4 里那些「用户态做不对」的事（比如原子追加），Ch5 会给出正路写法。

---

## 原书示例清单（man7 官方按章文件列表）

| Listing | 文件 | 页码 | 作用 |
|---------|------|------|------|
| **4-1** | `fileio/copy.c` | p.71 | 通用拷贝：`open` → `while (read > 0) write` → `close`；`usageErr` / `errExit` / `fatal` 三个助手的标准用法 |
| **4-3** | `fileio/seek_io.c` | p.84 | 交互式 `lseek` 演示器：`seek_io file {r<len>\|R<len>\|w<string>\|s<offset>}...`，一条命令串起读/写/定位 |

> 出处：[man7 — List of source code files, by chapter](https://man7.org/tlpi/code/online/all_files_by_chapter.html) 的 **Chapter 4** 一节。
> 注意：该索引里 **Chapter 4 只有这两个文件**（Listing 4-1 与 Listing 4-3）。本仓库的 `code/` 下**逐字镜像**了它们，并提供 `tlpi_hdr.h` 的**最小替身**让它们能单文件编译（原书的 `tlpi_hdr.h` 依赖整个 `lib/` 目录）。
>
> ⚠️ 别漏 `seek_io.c`——它不像 `copy.c` 那么有名，但它是原书唯一的 `lseek` 交互工具，且和 [4.7](notes/4.7-lseek.md) 直接对应。

---

## 易错清单

1. **`close` 之后那个 fd 号立刻能复用** —— 不是「用完就废」。实测 `close(3)` 后下一次 `open` 拿到的就是 `3`。
2. **`close` 不幂等** —— 重复 `close(3)` 返回 `EBADF`。所以「`close` 失败就重试」是**危险**写法（重试可能关掉别的线程刚拿到的同一个号）。
3. **`close(0)` 之后新 `open` 会拿到 0 号** —— 0/1/2 只是「约定」不是「保留」。守护进程重绑标准输入输出正是靠这个。
4. **`write(fd, buf, 0)` 返回 `0` 是合法的** —— `count = 0` 时内核连数据都不碰，`errno` 保持干净。
5. **`read(fd, buf, count)` 里 `count` 超过 buffer 实际大小不会报错** —— 内核的 `access_ok` 只验证**地址范围属于用户空间**，不知道你分配了多少。实测 `read(fd, buf[64], 1GiB)` **成功返回 36**，多出来的字节写进了相邻内存。**静默内存破坏**。
6. **`count` 有硬上限 `MAX_RW_COUNT`** —— `INT_MAX & PAGE_MASK`（`include/linux/fs.h:2399`），超过会被截断而不是报错。
7. **`O_CLOEXEC` 不在 `F_GETFL` 里** —— 它是**fd 标志**，要用 `F_GETFD` 查 `FD_CLOEXEC` 位。实测 `F_GETFL` 拿到 `0x8000` 里没有 `O_CLOEXEC`。
8. **`F_GETFL` 返回的不等于你 `open` 时传的** —— 内核会在 `do_sys_openat2` 里补位：`if (force_o_largefile()) flags |= O_LARGEFILE;`（`fs/open.c:1443-1444`）。这就是那个多出来的 `0x8000` 的来源。
9. **`O_LARGEFILE` 在 x86-64 的 glibc 头文件里根本没有** —— 因为 `__libc_open` 整块包在 `#ifndef __OFF_T_MATCHES_OFF64_T` 里（glibc 2.39 `sysdeps/unix/sysv/linux/open.c:33`），64 位平台上**完全不参与编译**。别去 glibc 里找答案，答案在内核侧。
10. **`umask` 是「减权限」不是「设权限」** —— 最终权限 = `mode & ~umask`。实测 `umask 0022` + 请求 `0666` → 实际 `0644`；`umask 0027` → `0640`。
11. **`write` 成功 ≠ 数据安全** —— 只保证进了**页缓存**，掉电即丢。要落盘得 `fsync`/`fdatasync`（实测都返回 `0`）。
12. **短读不是错误** —— `read` 返回小于 `count` 的正数是**正常结果**（EOF、管道剩余量、信号、终端行缓冲都会造成）。必须循环。
13. **部分写不是错误** —— 实测：容量 4 KiB 的管道上裸 `write(8192)` **返回 4096**，`errno` 干净。原书 Listing 4-1 里那句 `fatal("...partial write occurred")` 只在「只写普通文件」时安全。
14. **`SIGPIPE` 默认杀进程** —— 往读端已关的管道写，子进程直接死于 `signal 13`。忽略它才会拿到 `-1 errno=32 (EPIPE)`。
15. **`/dev/null` 写 30 字节返回 30** —— 「写成功」和「数据被保存」是两件事，这是最纯粹的证明。
16. **`TIOCGWINSZ` 不符合 `_IOR` 编码规范** —— 它是硬编码的 `0x5413`（`ioctls.h:38`），按位拆出来 `dir = 0`、`size = 0`。网上流传的 `_IOR('T', 104, struct winsize)` 算出来是 `0x80085468`，**对不上**。
17. **`_IOR` / `_IOW` 的方向是「用户态视角」** —— `ioctl.h:82-83` 原话：`_IOW means userland is writing and kernel is reading`。和「内核读/写」正好相反。
18. **`ENOTTY` 有两个来源** —— ① 对象**没有** `unlocked_ioctl`（`fs/ioctl.c:46-49`）；② 对象有、但驱动返回内部哨兵 `-ENOIOCTLCMD`（`:52-53`）。**用户态永远看不到 `ENOIOCTLCMD`**。
19. **`isatty()` 内部就是 ioctl** —— glibc 走 `__isatty` → `__tcgetattr` → `INLINE_SYSCALL (ioctl, 3, fd, TCGETS, &k_termios)`。
20. **`FIONBIO` 与 `fcntl(F_SETFL, O_NONBLOCK)` 改的是同一个位** —— 都是 `filp->f_flags` 的 `O_NONBLOCK`（`fs/ioctl.c:342` `ioctl_fionbio`）。不是两套独立状态。
21. **同一个 `FIONREAD` 有两套实现** —— 普通文件在 `do_vfs_ioctl` 里直接算 `i_size - f_pos`（`fs/ioctl.c:829-834`）；管道则转发给 `pipe_ioctl`（`fs/pipe.c:608`）加锁遍历环形缓冲区。
22. **`lseek` 是 0-based 的** —— `lseek(fd, 4, SEEK_SET)` 之后的第一个字节是第 **5** 个字符。原书 `seek_io` 实测：`wABCDEFGHIJ` 后 `s4 r3` 得到 `EFG`。
23. **`lseek(fd, 0, SEEK_CUR)` 是「查偏移而不移动」** —— `default_llseek` 里有专门的快捷分支（`fs/read_write.c:78-81`），直接返回 `file->f_pos`。
24. **`lseek` 不改数据、不触发磁盘 I/O** —— 只改内存里的 `f_pos`。同一位置重复读结果不变。
25. **`SEEK_HOLE` 的边界按文件系统块对齐，但会被 `i_size` 截断** —— 实测同一份代码两种表现：数据段顶到文件末尾时报 `i_size`（`2097157`，看着像字节级精确）；后面还有洞时报块边界（`2101248`）。
26. **`SEEK_HOLE` / `SEEK_DATA` 的可移植性取决于底层文件系统** —— 内核通用兜底 `default_llseek` 假设「**文件末尾是唯一的洞**」（`fs/read_write.c:95-106` 注释原话）。在不支持的 FS 上，稀疏拷贝优化会**静默失效**。
27. **朴素 `read/write` 拷贝会把空洞灌实** —— 读空洞得到的是**真实的 0 字节**，写过去就真分配块了。实测一个 3 MiB 逻辑 / 12 KiB 物理的稀疏文件，物理占用差 **256 倍**。
28. **`fork` 前必须 `fflush(stdout)`** —— stdout 被重定向时是全缓冲的，子进程会继承缓冲再刷一遍，输出**重复两遍**。（本章 `c4_5_write.c` 真踩过这个坑。）
29. **流对象不可 seek** —— 管道/socket/终端没有 `FMODE_LSEEK`（`fs.h:115`，值 `0x4`），`vfs_llseek` 第一行就 `-ESPIPE`（实测 `errno=29`）。
30. **`dup` / `fork` 共享偏移，各自 `open` 才是独立的** —— 因为 `f_pos` 在 `struct file` 里（`include/linux/fs.h:1007`），不在 fd 表里。
31. **`read` / `write` 失败不会留下「半个偏移」** —— `ksys_read` 先把偏移拷到局部 `pos`，只有 `ret >= 0` 才回写进 `f_pos`（`fs/read_write.c:602-618`）。
32. **`O_APPEND` 的原子性无法用用户态代码复刻** —— `lseek(SEEK_END) + write()` 是**两步**，中间有 TOCTOU 竞争窗口；`O_APPEND` 把「定位到末尾」下沉到内核的 `write` 里完成。

---

## 章节链路

```text
Ch2  进程 / 内核分工 → fd 是什么
  → Ch3  syscall 约定 + errno 范式 + 错误处理助手
  → Ch4  通用 I/O 模型：六个调用走遍所有对象
           open  → 拿 fd（三件套 flags/mode/umask）
           read  → 短读合法；count 不校验 buffer
           write → 部分写合法；成功 ≠ 落盘
           close → 号立即回收；不幂等
           lseek → 只改 f_pos；空洞与 SEEK_HOLE
           ioctl → 字节流之外的控制通道
  → Ch5  fcntl / dup / pread / pwrite / readv / writev / O_NONBLOCK
  → Ch13 stdio 缓冲 vs 内核缓冲（为什么 fork 前要 fflush）
  → Ch14 文件系统：块、洞、extent
  → Ch49 mmap（把文件映射进地址空间，绕开 read/write）
  → Ch63 替代 I/O 模型：epoll / aio / io_uring
```

---

## 双线提示

| 路线 | |
|------|--|
| **HFT** | 短读/部分写必须循环，否则**静默丢数据**；`write` 成功只到页缓存，成交回报要 `fsync`；热路径减少 syscall → `writev`（Ch5）/`mmap`（Ch49）；多进程行情日志统一 `O_APPEND`，绝不用 `lseek+write`；长跑进程 `open` 一律带 `O_CLOEXEC` 防 fd 泄漏给子进程；`read(fd, buf, n)` 的 `n` 必须是**真实的 buffer 大小**——内核不替你检查 |
| **嵌入式** | `/dev` 下设备节点用同一套 `open/read/write/close`；专属控制（点灯、读寄存器、配波特率）走 `ioctl`，命令字由驱动定义、魔法数要全局唯一；串口/管道是流对象，**不可 seek**；`umask` 决定设备节点权限（`mknod` 之后要给对）；裸 flash/无 `fsync` 语义的介质上，「写成功」与「落盘」的差距更大 |

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 通用模型 = `open` / `read` / `write` / `close` + `lseek` + `ioctl`，六个调用走遍一切对象 |
| 2 | 三层结构：**fd 表（进程私有）→ `struct file`（可共享）→ inode（全局）** |
| 3 | `f_pos` 在 `struct file` 里 → `dup`/`fork` **共享偏移**；各自 `open` 才独立 |
| 4 | 0/1/2 是约定不是保留；`close` 后号**立即复用**；`close` **不幂等** |
| 5 | 短读 / 部分写都是**合法结果**，不是错误 → 必须循环 |
| 6 | `count` 不受 buffer 实际大小约束（只有 `MAX_RW_COUNT` 上限）→ 越界静默破坏内存 |
| 7 | `write` 成功 ≠ 落盘；要 `fsync` / `fdatasync` |
| 8 | `umask` 是**减**权限：`mode & ~umask` |
| 9 | `F_GETFL` 会被内核补 `O_LARGEFILE`；`O_CLOEXEC` 要用 `F_GETFD` 查 |
| 10 | `ioctl` 的 `request` = `dir(2) \| size(14) \| type(8) \| nr(8)`；方向是**用户态视角** |
| 11 | `TIOCGWINSZ = 0x5413` 是老式硬编码字，**不合** `_IOR` 规范 |
| 12 | `ENOTTY` = 「对象不认识这条命令」；来源有二（无方法 / `-ENOIOCTLCMD`） |
| 13 | `FIONBIO` ≡ `fcntl(F_SETFL, O_NONBLOCK)`；`FIONREAD` 普通文件算 `i_size - f_pos`、管道遍历环形缓冲 |
| 14 | `lseek` 是 0-based；只改游标，不改数据、不碰磁盘 |
| 15 | `SEEK_END` 的「末尾」= `i_size`；`SEEK_CUR + 0` 只查询不移动 |
| 16 | 越尾写造**文件空洞**：逻辑大小 ≫ 物理占用（实测 256 倍） |
| 17 | 洞检测（`SEEK_HOLE`/`SEEK_DATA`）按**块**对齐、且依赖具体文件系统 |
| 18 | 流对象不可 seek（`FMODE_LSEEK` 未设）→ `ESPIPE` |
| 19 | `O_APPEND` 把「定位+写」做成原子，用户态复刻不了 |
| 20 | 原书本章只有 **2 个**示例文件：`copy.c`（4-1）、`seek_io.c`（4-3） |

---

## 参考

- Kerrisk, *The Linux Programming Interface*, **Chapter 4 — File I/O: The Universal I/O Model**
- [man7 官方源码清单（按章）](https://man7.org/tlpi/code/online/all_files_by_chapter.html) · [OUTLINE](../OUTLINE.md) · [Ch5 文件 I/O 深入](../chapter-05-file-io-further/README.md)
- 内核源码（v6.6）：`fs/open.c`（`do_sys_openat2` / `SYSCALL_DEFINE3(open)` / `SYSCALL_DEFINE1(close)`）、`fs/read_write.c`（`vfs_read` / `vfs_write` / `ksys_read` / `ksys_write` / `vfs_llseek` / `default_llseek` / `file_ppos`）、`fs/ioctl.c`（`SYSCALL_DEFINE3(ioctl)` / `do_vfs_ioctl` / `vfs_ioctl` / `file_ioctl` / `ioctl_fionbio`）、`fs/pipe.c`（`pipe_ioctl`）
- 头文件：`include/linux/fs.h`（`struct file` / `struct file_operations` / `FMODE_*` / `MAX_RW_COUNT`）、`include/linux/fdtable.h`（`files_struct`）、`include/uapi/asm-generic/ioctl.h`、`include/uapi/asm-generic/ioctls.h`
- glibc 2.39：`sysdeps/unix/sysv/linux/open.c`、`sysdeps/posix/isatty.c`、`sysdeps/unix/sysv/linux/tcgetattr.c`

---

## 代码示例

本章 `code/` 下有：

- **11 个自编 demo**（`c4_*.c` 8 个 + `ex4_*.c` 3 个）
- **2 个原书文件**逐字镜像（`copy.c` = Listing 4-1、`seek_io.c` = Listing 4-3）
- **1 个框架替身** `tlpi_hdr.h`（原书 `lib/tlpi_hdr.h` 的最小可用子集：`errExit` / `fatal` / `usageErr` / `cmdLineErr` / `getLong`）

全部在 Compiler Explorer（gcc 13.3.0 / x86-64 / Ubuntu 24.04）上真实编译 + 运行过，共 **15 个作业**，全部 `build code = 0` / `didExecute = True` / `diagnostics = 0`。输出原样抄在对应笔记的实测块里。完整索引见 [`code/README.md`](code/README.md)。

| 文件 | 对应节 | 演示什么 | 需要什么 |
|------|--------|----------|---------|
| `c4_1_fd_basics.c` | 4.1 | fd 分配与号回收 / `/proc/self/fd` / 关 0 号拿 0 号 | `-` |
| `c4_2_universal.c` | 4.2 | 同一套 API 打「普通文件 / `/dev/null` / `/proc/version` / pipe」 | `-` |
| `c4_3_open.c` | 4.3 | 四组 `umask` 实测 / `O_EXCL` 原子创建 / `O_TRUNC` / `FD_CLOEXEC` vs `O_*` | `-` |
| `c4_4_read.c` | 4.4 | 真短读 / `EBADF` 两成因 / `EFAULT` / `count` 远超 buffer 竟然成功 | `-` |
| `c4_5_write.c` | 4.5 | `count=0` 合法 / `SIGPIPE` vs `EPIPE` / `fsync` 落盘 | `-`（会 `fork`） |
| `c4_6_close.c` | 4.6 | 号立即回收 / 重复 `close` `EBADF` / `dup` 与 `fork` 共享偏移 | `-`（会 `fork`） |
| `c4_7_lseek.c` | 4.7 | 三种基准 / 造洞 / `SEEK_HOLE` 块对齐 / 管道 `ESPIPE` | `_GNU_SOURCE` |
| `c4_8_ioctl.c` | 4.8 | `request` 位域拆解 + `TIOCGWINSZ` 不合规证据链 / `FIONREAD` / `FIONBIO` | `-` |
| `copy.c` | 4.1–4.5 | **原书 Listing 4-1**（三种调用姿势都跑过） | `tlpi_hdr.h` |
| `seek_io.c` | 4.7 | **原书 Listing 4-3**：交互式读/写/定位 | `tlpi_hdr.h` |
| `ex4_1_tee.c` | 4.10 | 原书习题 4-1：`-a` + 一份输入两处输出 | `-` |
| `ex4_2_cpholes.c` | 4.10 | 原书习题 4-2：**保留源文件的洞**的 `cp`（自造源文件含 3 处洞，其中一处是尾部洞） | `_GNU_SOURCE` |
| `ex4_3_partial_write.c` | 4.10 | 自编：制造一次真实的部分写 | `-` |

一次编完全部自编 demo（在 `code/` 目录下）：

```bash
for f in c4_*.c ex4_*.c; do
    gcc -O0 -Wall -Wextra -o "${f%.c}" "$f" || echo "FAIL $f"
done
```

两个原书程序都需要 `tlpi_hdr.h` 替身：

```bash
gcc -O0 -Wall -Wextra -o copy    copy.c
gcc -O0 -Wall -Wextra -o seek_io seek_io.c
```

带参数的用法：

```bash
./copy /proc/version /tmp/out.txt
./seek_io /tmp/seek.txt wABCDEFGHIJ s4 r3 s0 R10 s20 wXYZ s20 r3
printf 'hello tee\nsecond line\n' | ./ex4_1_tee /tmp/tee_out.txt
```

**几条实测结论**（都是本仓库跑出来的，不是书上抄的）：

- **fd 号立即复用**：`open a=3`、`open b=4`；`close(3)` 后 `open c=3`；`close(0)` 后 `open d=0`
- **`umask` 四组对账**：`0000→0666`、`0022→0644`、`0077→0600`、`0027→0640`
- **`F_GETFL` 多出 `0x8000`**：内核在 `fs/open.c:1443-1444` 补的 `O_LARGEFILE`
- **`read(fd, buf[64], 1GiB)` 返回 36（成功）**：内核只验地址范围，不管 buffer 实际大小
- **短读实测**：文件只剩 6 字节时 `read(8)` 返回 **6**；再读返回 `0`；管道里 3 字节时 `read(64)` 返回 **3**
- **`write(...,0)` 返回 0**，`errno` 干净——**合法**不是错误
- **`SIGPIPE` 实测**：子进程死于 `signal 13`；忽略后 `write` 返回 `-1 errno=32 (EPIPE)`
- **部分写实测**：`F_SETPIPE_SZ(4096)` 的管道上裸 `write(8192)` → **返回 4096**
- **`TIOCGWINSZ = 0x5413`**（`dir=0`、`size=0`）；`_IO('T',0x13)` 与之**相等**；`_IOR('T',104,…)` = `0x80085468` **对不上**
- **`isatty()` 全是 0**（沙箱无 tty）；`FIONREAD` 写 9 读 4 → `9`→`5`；`FIONBIO` 读空管道 → `EAGAIN`
- **稀疏文件**：3 段数据 + 3 处洞，源/目标都 `st_size = 3145728` / `st_blocks*512 = 12288`（**256 倍**差距）
- **`SEEK_HOLE` 块对齐 + `i_size` 截断**：真实洞检测来自文件系统，不是内核通用兜底
- **原书 `seek_io` 输出**：`wABCDEFGHIJ` → `r3` 得 `EFG`、`R10` 得 `41 42 … 4a`、`s20 wXYZ` → `r3` 得 `XYZ`
