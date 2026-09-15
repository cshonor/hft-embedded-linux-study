# TLPI 第 05 章 — File I/O: Further Details

**优先级**：🔴 必读（Ch4 给了模型，本章讲**这套模型下面的机关**：标志在哪一层、偏移归谁共享、怎么原子地做定位读写）
**前置**：[Ch4 通用 I/O 模型](../chapter-04-file-io-universal/README.md)（`open`/`read`/`write`/`close`/`lseek`/`ioctl` 六个调用与三层结构）
**后置**：[Ch6 进程](../chapter-06-processes/README.md)（`fork` 如何共享打开文件表项）→ [Ch13 文件 I/O 缓冲](../chapter-13-file-io-buffering/README.md)

---

## 小节目录

- [5.1 原子性与竞争条件](notes/5.1-atomicity-race-conditions.md)
- [5.2 `fcntl()`](notes/5.2-fcntl.md)
- [5.3 打开文件状态标志](notes/5.3-open-file-status-flags.md)
- [5.4 文件描述符与打开文件的关系](notes/5.4-fd-and-open-files.md)
- [5.5 复制文件描述符](notes/5.5-duplicating-fds.md)
- [5.6 `pread()` / `pwrite()`](notes/5.6-pread-pwrite.md)
- [5.7 分散/聚集 I/O：`readv()` / `writev()`](notes/5.7-readv-writev.md)
- [5.8 `truncate()` / `ftruncate()`](notes/5.8-truncate-ftruncate.md)
- [5.9 非阻塞 I/O](notes/5.9-nonblocking-io.md)
- [5.10 大文件 I/O（LFS）](notes/5.10-large-files.md)
- [5.11 `/dev/fd`](notes/5.11-dev-fd.md)
- [5.12 创建临时文件](notes/5.12-temporary-files.md)
- [5.13 Summary 本章总结](notes/5.13-summary.md)
- [5.14 Exercises 练习](notes/5.14-exercises.md)

> 十四节的划分与 TLPI 原书一致（5.1–5.14）。原书本章**没有子编号小节**（如 5.6 里的子标题收在同一篇里用 `###`），所以一篇对应一节。
>
> **注意 5.14 的性质**：原书第 5 章末有 **7 道**习题（5-1 … 5-7，全部要求写代码）。本仓库对 5-1/5-2/5-4/5-5/5-7 各写了一份可运行实现，5-3 与 5-6 直接用原书随书分发的解答文件（`atomic_append.c`、`multi_descriptors.c`）做对照。

---

## 章节目标

- **分层**：同样是「设一个标志」，在 **fd 表项**上设和在 **`struct file`** 上设，后果完全不同。本章的所有「怪现象」都能归到「你动的是哪一层」
- **原子性**：`O_CREAT|O_EXCL` 消灭创建竞态、`O_APPEND` 消灭追加竞态、`pread`/`pwrite` 消灭定位竞态——**三处竞态，三个内核级解法，用户态都复刻不了**
- **静默失败**：`F_SETFL` 改不动的位**不报错**；普通文件上设 `O_ASYNC` **不生效**。本章专门为「静默失败」建了几张证据表
- **诚实**：`/dev/fd` 与 `dup` **不等价**（与流传说法不同）；`O_TMPFILE` 的 `O_EXCL` **不是**「已存在就失败」；`F_GETFL` 里的 `0x8000` 是内核补的
- **溯源**：本章 **13 篇**笔记逐条核对了 Linux **v6.6** 与 glibc **2.39** 的真实源码行号；正文引用的每一行实测输出都能在本仓库的 CE 日志里定位

### 一条主线

| 层 | 是什么 | 谁拥有 | 关键字段 | 本章哪一节在动它 |
|----|--------|--------|---------|----------------|
| ① 进程 fd 表 | `int` 号 → 表项指针 | 进程（`files_struct`） | 表项指针、**`FD_CLOEXEC`** | [5.3](notes/5.3-open-file-status-flags.md) 的 `F_GETFD`、[5.5](notes/5.5-duplicating-fds.md) 的 `dup3(O_CLOEXEC)` |
| ② 打开文件表项 | `struct file` | **可被多个 fd / 多进程共享** | `f_pos` / `f_flags` / `f_count` / `f_op` | [5.4](notes/5.4-fd-and-open-files.md)、[5.5](notes/5.5-duplicating-fds.md)、[5.6](notes/5.6-pread-pwrite.md)、[5.9](notes/5.9-nonblocking-io.md) |
| ③ inode | 文件本体 | 全局（inode 缓存） | `i_size` / `i_mode` / `i_ino` / 块映射 | [5.8](notes/5.8-truncate-ftruncate.md) 的 `truncate`、[5.12](notes/5.12-temporary-files.md) 的 `d_tmpfile` |

**读法**：动 ① → 只影响一个 fd；动 ② → 影响所有共享它的 fd（`dup`/`fork`）；动 ③ → 影响所有打开者。

### 与 Ch4 / Ch6 边界（防混淆）

| 章 | 主题 | 内容 |
|----|------|------|
| **Ch4** | 通用 I/O 模型 | **只用** `open`/`read`/`write`/`close`/`lseek`/`ioctl` 六个调用，把「一切对象」走一遍 |
| **Ch5** | Further Details | 同样的模型，换**更强的手段**：`fcntl`（改标志/异步通知）、`dup` 族（复制 fd）、`pread`/`pwrite`（原子定位读写）、`readv`/`writev`（批量）、`truncate`、`O_NONBLOCK`、LFS、`/dev/fd`、临时文件 |
| **Ch6** | 进程 | `fork` 复制层 ①、`exec` 清 `FD_CLOEXEC` 的 fd；`_exit` 与 flush 的关系 |

一句话：**Ch4 里那些「用户态做不对」的事，Ch5 给出正路写法；而「为什么做不对」，要靠下面这张图。**

```
进程 fd 表（每进程一份）        打开文件表项（struct file）        inode 表（全局共享）
┌───────────────┐              ┌──────────────────────┐          ┌──────────────────┐
│ 0 ──┐         │              │ f_pos      偏移游标   │          │ i_ino    inode 号 │
│ 1 ──┼──┐      │   dup/fork   │ f_flags    状态标志   │          │ i_mode   类型权限 │
│ 3 ──┼──┼──┐   │ ───────────▶ │ f_count    引用计数   │ ───────▶ │ i_size   逻辑长度 │
│     │  │  │   │   共享它      │ f_op       操作表     │          │ 数据块映射/洞     │
│ FD_CLOEXEC ◀──┼──「哪一个 fd」 │                      │          │                  │
└───────────────┘              └──────────────────────┘          └──────────────────┘
   ①                          ②                                  ③
```

---

## 原书示例清单（man7 官方按章文件列表）

| 文件 | 原书位置 | 作用 |
|------|---------|------|
| `fileio/bad_exclusive_open.c` | **Listing 5-1, p.90** | 反面教材：`O_EXCL` 失败就退回普通 `open`，于是「谁创建的」无从判断 |
| `fileio/t_readv.c` | **Listing 5-2, p.101** | `readv` 读「定长头 + 变长体」：`totRequired` vs 实际读到字节数的对照 |
| `fileio/large_file.c` | **Listing 5-3, p.105** | 已废弃的 LFS API：`_LARGEFILE64_SOURCE` + `open64`/`off64_t`/`lseek64` |
| `fileio/atomic_append.c` | **Exercise 5-3 解答, p.110** | `O_APPEND` vs `lseek+write` 的竞态对照程序（`file num-bytes [x]`） |
| `fileio/multi_descriptors.c` | **Exercise 5-6 解答, p.111** | 三个描述符共享/不共享打开文件表项的现场直播（每写一次 `cat` 一次） |
| `fileio/t_truncate.c` | 无 listing 编号 | `truncate` 的交互工具：`t_truncate file length` |

> 出处：[man7 — List of source code files, by chapter](https://man7.org/tlpi/code/online/all_files_by_chapter.html) 的 **Chapter 5** 一节（共 **6 个**文件）。
> 本仓库在 `code/` 下**逐字镜像**了这 6 个文件，并提供 `tlpi_hdr.h` 的**最小替身**让它们能单文件编译（原书的 `tlpi_hdr.h` 依赖整个 `lib/` 目录）。
>
> ⚠️ 别漏 `t_truncate.c`——它是原书本章唯一的 `truncate` 交互工具，且没有 listing 编号，容易被索引漏掉。

---

## 易错清单

1. **「没有 `O_EXCL` 的创建」不是创建** —— 实测两个进程**都**打印「我创建了它」，而文件只有一个。`O_EXCL` 失败的返回码是 `EEXIST`（`17`）。
2. **`O_APPEND` 的原子性用户态复刻不了** —— `lseek(SEEK_END)` + `write()` 是两步。实测：两个进程各写 20 万字节，`O_APPEND` 组 `st_size = 400000`（一个不丢），`lseek+write` 组 `391764`（**丢 8236**）。
3. **丢多少不固定** —— 取决于调度交错密度，每次跑都不同。**别把某一次的数字当常量写进断言。**
4. **`O_APPEND` 下次 `write` 会**无视**你刚做的 `lseek`** —— 内核在每次 `write` 里重设偏移（`fs/read_write.c:1666-1667`）。实测：`lseek(0)` 后 `f_pos` 确实变 0，但写完变成 16、内容仍在末尾。
5. **`fcntl` 未知命令是 `EINVAL`，不是 `ENOTTY`** —— `ENOTTY`（`25`）是 `ioctl` 的（Ch4）。实测 `fcntl(fd, 0x7fffffff)` → `errno=22`。
6. **`F_SETFL` 改不动的标志：不报错，只是静默丢掉** —— `SETFL_MASK` 只有 5 个位（`O_APPEND`/`O_NONBLOCK`/`O_NDELAY`/`O_DIRECT`/`O_NOATIME`，`fs/fcntl.c:35`）。实测 `F_SETFL(O_CREAT|O_TRUNC|O_EXCL)` **返回 0**，而 `F_GETFL` 一位没变。
7. **`F_SETFL(O_ASYNC)` 在普通文件上静默失败** —— `FASYNC` 不在掩码里，要靠 `f_op->fasync` 钩子设（`fs/fcntl.c:71` 注释原话）。同一段代码换成**管道**就生效（实测 `0x02000`）。
8. **`O_CREAT` / `O_TRUNC` / `O_EXCL` 根本不是「状态标志」** —— `do_dentry_open()` 里就被清掉了（`fs/open.c:945`：`f->f_flags &= ~(O_CREAT | O_EXCL | O_NOCTTY | O_TRUNC);`）。它们是「`open` 时的一次性开关」。
9. **访问模式（`O_RDONLY`/`O_WRONLY`/`O_RDWR`）改不了** —— `F_SETFL(O_WRONLY)` 后 `F_GETFL` 读回来还是 `O_RDONLY`。实测写进去 → `EBADF`。
10. **判断访问模式不能直接 `& O_RDONLY`** —— `O_RDONLY` 是 `0`。必须 `flags & O_ACCMODE` 再比较。
11. **状态标志跟着「打开文件表项」走** —— 只对 dup 出来的 fd 设 `O_APPEND`，原 fd 的 `F_GETFL` **一起**变成 `0x08401`。
12. **但 `FD_CLOEXEC` 不共享** —— 它在**层 ①**。实测只对 fd2 设，fd1 的 `F_GETFD` 仍是 `0x0`。
13. **`dup2(old, new)` 当 `old == new` 时一个字节都不动** —— 不关、不复制，但**必须校验 `old` 有效**（`fs/file.c:1273-1282`）。实测 `dup2(3,3) -> 3`、`dup2(999,999) -> EBADF`。
14. **`dup3(3, 3, 0)` 直接 `EINVAL`** —— POSIX 与 Linux 在这里故意不一致（`fs/file.c:1241-1242`）。
15. **`dup` 取「最小可用号」，所以会抢到 0 号** —— `close(0)` 之后 `dup(fd)` 返回 **`0`**。0/1/2 不是保留号。
16. **`dup` 不带 `CLOEXEC`，`dup3` 才带** —— 实测 `dup(fd) -> 4 F_GETFD=0x0`、`dup3(fd,30,O_CLOEXEC) -> 30 F_GETFD=0x1`。
17. **`F_DUPFD` 与 `dup2` 语义不同** —— `F_DUPFD` 是「**分配**一个 ≥ arg 的新号，从不关闭任何东西」；`dup2` 是「**替换** newfd」。混用是 fd 串号 bug 的常见根源。
18. **`pread`/`pwrite` 不碰共享游标** —— 机制是它用 `fdget()` 而 `read`/`write` 用 `fdget_pos()`（`fs/read_write.c:661` vs `:604`）。实测 `pread` 后 `f_pos` 仍是 `10`。
19. **`pread` 在管道上是 `ESPIPE`** —— 管道没有「偏移」概念。实测 `errno=29`。
20. **`pwrite` 超尾写** —— `pwrite(offset=1000)` 返回 `1`，`st_size` 变 `1001`，但**物理仍只占 4096 B**——偏移没跨出块边界，所以没真省空间。**「洞」要跨块才产生**。
21. **`readv`/`writev` 的段数上限是 `IOV_MAX = 1024`** —— `sysconf(_SC_IOV_MAX)` 实测 `1024`（= `UIO_MAXIOV`，`include/uapi/linux/uio.h:28`）。内核内部 `UIO_FASTIOV = 8` 段以内走栈上数组。
22. **`readv`/`writev` 一样会短读/部分写** —— 实测容量 4 KiB 的管道上 `writev` 请求 4096 → `-1 errno=11 (EAGAIN)`。段数多不代表「一次到位」。
23. **`ftruncate` 失败在只读 fd 上是 `EINVAL`（22），不是 `EACCES`** —— 权限在 `open` 时已查过，这里只查 `S_ISREG` / `FMODE_WRITE`（`fs/open.c:179`）。
24. **`truncate` 按路径走才会报 `EACCES`** —— 它要过 `inode_permission(MAY_WRITE)`（`fs/open.c:90`）。实测 `t_truncate /proc/version 10` → `Permission denied`。
25. **`truncate` 变长产生的是「洞」** —— 实测撑到 `8192` 时物理仍是 `4096`；而真写 4096 个 0 之后物理变 `8192`——**同样 8192 逻辑长度，物理差 2 倍**。
26. **`ftruncate(fd, 负数)` 是 `EINVAL`；`truncate("/dir", n)` 是 `EISDIR`** —— 实测 `errno=22` / `errno=21`。
27. **`EAGAIN` 与 `EINTR` 是两回事** —— `EAGAIN`（`11`）= 非阻塞下现在没数据/没空间（**状态**）；`EINTR`（`4`）= 慢系统调用被信号打断（**事件**）。实测两者都拿到了。
28. **`EAGAIN == EWOULDBLOCK`（同值 11）**，但**不要**用 `EAGAIN` 去判「被信号打断」。
29. **`SA_RESTART` 会让慢系统调用自动重启** —— 设了就收不到 `EINTR`。调试时「为什么我的 `EINTR` 分支从不进」多半是它。
30. **FIFO 的 `open` 语义随访问模式变** —— `O_RDONLY|O_NONBLOCK` 立即成功；`O_WRONLY|O_NONBLOCK` 无读端时 `ENXIO`；`O_RDWR` 从不阻塞。
31. **64 位平台上 `off_t` 天然是 8 字节** —— `sizeof(off_t) = 8`。真正拦住大文件的是 `RLIMIT_FSIZE`（沙箱 16 MiB，超了先 `SIGXFSZ` 杀进程，装 handler 后 `EFBIG`）和文件系统的 `s_maxbytes`。
32. **`F_GETFL` 里的 `0x8000` 是内核补的 `O_LARGEFILE`，而 glibc 的宏是 `0`** —— 两边不一致不是 bug：x86-64 上 glibc 把它定成 0（无事可做），内核在 `fs/open.c:1443-1444` 自己补上。
33. **`/dev/fd` 在精简系统里不存在** —— 它只是 `MAKEDEV` 建的符号链接（CE 沙箱里 `stat` → `ENOENT`）。直接走 `/proc/self/fd` 更可靠。
34. **`open("/dev/fd/N")` **不**等于 `dup(N)`** —— 官方 `proc_pid_fd(5)` 只说它是「a symbolic link to the actual file」；内核 `proc_fd_link()` 只复制 `f_path`（`fs/proc/fd.c:181-182`）。实测：读 4 字节后**原 fd 的 `f_pos` 仍是 0**（若是 `dup` 应变成 4）。
35. **`unlink` 之后仍能通过 `/proc/self/fd/N` 重新打开** —— 它引用的是 **inode**。实测返回新 fd 并读回原内容，`readlink` 显示 `... (deleted)`。
36. **`(deleted)` 后缀有歧义** —— 内核注释自己承认（`fs/d_path.c:255-256`：`Note that this is ambiguous.`），因为文件名本身可以叫 `xxx (deleted)`。**别拿它当判据。**
37. **`/proc/self/fdinfo/N` 里的 `O_CLOEXEC` 是内核「合成」的** —— `f_flags |= O_CLOEXEC`（`fs/proc/fd.c:45-46`），因为它不在 `f_flags` 里。所以 `F_GETFL` 看不到、`fdinfo` 看得到。
38. **`mkstemp` 的模板会被就地改写** —— 必须 `char tmpl[] = "/tmp/xXXXXXX"`，传字符串字面量会段错误。实测 `"/tmp/c5tmpXXXXXX"` → `"/tmp/c5tmpEE7wgl"`。
39. **`mkstemp` 强制 `0600`，且仍要过 `umask`** —— `try_file()` 里写死 `S_IRUSR|S_IWUSR`（`sysdeps/posix/tempname.c:180`）。glibc ≤ 2.6 曾是 `0666`（安全隐患）。
40. **`mktemp` / `tmpnam` / `tempnam` 都是 TOCTOU** —— `mktemp` 用 `__GT_NOCREATE`，只 `lstat` **检查**不创建（`sysdeps/posix/tempname.c:189-197`）。glibc 直接给链接期警告（`misc/mktemp.c:36-37`）。
41. **`tmpfile()` 显示 `/tmp/#4 (deleted)`** —— 说明它走的是 `O_TMPFILE` 路径而不是「`mkstemp`+`unlink`」。`#4` 是 `d_tmpfile()` 用 `sprintf(d_name, "#%llu", i_ino)` 编的（`fs/dcache.c:3259-3260`），**是 inode 号，会随环境变**。
42. **`O_TMPFILE` 的文件从来没有过名字** —— `d_tmpfile()` 直接 `inode_dec_link_count()`（`fs/dcache.c:3253`），不是「先创建再 unlink」。
43. **`O_TMPFILE | O_EXCL` 的 `O_EXCL` 不是「已存在就失败」** —— 它的含义被重定义成「不许 `linkat` 挂名」（控制 `I_LINKABLE`，`fs/namei.c:3700-3704` / `:4598`）。官方 `open(2)` 明确说「the meaning of O_EXCL in this case is different」。
44. **`AT_EMPTY_PATH` 需要 `CAP_DAC_READ_SEARCH`** —— 否则 `-ENOENT`（`fs/namei.c:4648-4649`）。想让它自己不需要特权，就改用 `/proc/self/fd/N` + `AT_SYMLINK_FOLLOW`。
45. **`do_linkat()` 只认 `AT_SYMLINK_FOLLOW` 和 `AT_EMPTY_PATH`** —— 多传一个 flag 直接 `-EINVAL`（`fs/namei.c:4639-4642`）。
46. **`O_TMPFILE` 需要文件系统支持** —— 不支持时 `-EOPNOTSUPP`（`fs/namei.c:3683-3684`）。官方清单：ext2/3/4、UDF、Minix、tmpfs；XFS 3.15、Btrfs 3.16、F2FS 3.16、ubifs 4.9。

---

## 章节链路

```text
Ch2  进程 / 内核分工 → fd 是什么
  → Ch3  syscall 约定 + errno 范式 + 错误处理助手
  → Ch4  通用 I/O 模型：六个调用走遍所有对象（三层结构第一次出现）
  → Ch5  把三层结构讲透，并给出「原子、并发、静默失败」的正解
           fcntl       → 改标志（只有 5 个位可改）/ 异步通知归属
           dup 族      → 复制 fd（共享层 ②）/ F_DUPFD vs dup2 语义差
           pread/pwrite→ 原子定位读写（并发安全的正路）
           readv/writev→ 一次 syscall 搬多段内存
           truncate    → 改 inode 的 i_size（洞的产生与回收）
           O_NONBLOCK  → 把「等待」换成 EAGAIN；与 EINTR 区分
           LFS         → off_t 宽度 vs RLIMIT_FSIZE / s_maxbytes（两层）
           /dev/fd     → fd 的路径化身（≠ dup）
           临时文件     → 名字与 inode 的分离（mkstemp / O_TMPFILE）
  → Ch6  fork 复制层 ①、共享层 ②；exec 清 FD_CLOEXEC 的 fd
  → Ch13 stdio 缓冲 vs 内核缓冲（为什么 fork 前要 fflush）
  → Ch15 link/unlink/stat；Ch44 管道与 FIFO；Ch55 文件锁；Ch63 epoll/aio/io_uring
```

---

## 双线提示

| 路线 | |
|------|--|
| **HFT** | 多进程行情日志**必须** `O_APPEND`（实测 `lseek+write` 一次就丢 8236 字节）；多线程读同一份快照文件用 `pread`，绝不用 `lseek+read`；批量收发用 `readv`/`writev`，但记住 `IOV_MAX = 1024` 且**仍会部分写**；事件循环靠 `O_NONBLOCK` + `epoll`，`EAGAIN`（重试）与 `EINTR`（重试）都要处理、且要留意 `SA_RESTART`；原子写配置 = `mkstemp` 同目录 + `fsync` + `rename`；长跑进程 `open` 一律带 `O_CLOEXEC`，否则 `fork+exec` 会把行情 fd 泄露给子工具；大文件归档要同时过 `off_t` 宽度、`RLIMIT_FSIZE`、`s_maxbytes` 三道门 |
| **嵌入式** | `/tmp` 常是 `tmpfs`——重启即消失、但容量等于内存，写大文件要盯 `ENOSPC`；只读根文件系统上 `tmpfile()` 会 `EROFS`，必须先把 `tmpfs` 挂到 `/tmp`；设备上 `ulimit -f` 常被设死，写日志/录制会被 `SIGXFSZ` **静默杀掉**，要显式处理；`O_TMPFILE` 依赖文件系统（jffs2/ubifs 新版本才支持），不支持时 glibc 会自动退到「`mkstemp`+`unlink`」；没有 `strace` 时，`readlink /proc/PID/fd/*` 是判断「握着哪些设备节点、哪些是匿名 `O_TMPFILE`（显示 `#N`）」的最廉价手段；`O_DIRECT` 在嵌入式上更危险（缓冲区必须块对齐），优先考虑 `mmap`；`/dev/fd` 在 BusyBox 精简系统里常常不存在，脚本里直接写 `/proc/self/fd` |

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 三层结构：**fd 表（进程私有）→ 打开文件表项 `struct file`（可共享）→ inode（全局）** |
| 2 | `f_pos` / `f_flags` 在层 ② → `dup`/`fork` **共享**；`FD_CLOEXEC` 在层 ① → **不共享** |
| 3 | 动一个标志前先问：**它在哪一层**？这一句话能解释本章所有「怪现象」 |
| 4 | `O_CREAT\|O_EXCL` = 原子创建；`O_APPEND` = 原子追加；`pread`/`pwrite` = 原子定位读写。**三处竞态，用户态都复刻不了** |
| 5 | `F_SETFL` 只能改 5 个位：`O_APPEND` / `O_NONBLOCK` / `O_NDELAY` / `O_DIRECT` / `O_NOATIME`（`SETFL_MASK`） |
| 6 | `F_SETFL` 改不动的位**静默丢弃、返回 0**；`O_ASYNC` 要靠 `f_op->fasync` 才有用 |
| 7 | `fcntl` 未知命令 → `EINVAL`（`22`）；`ioctl` 未知命令 → `ENOTTY`（`25`） |
| 8 | `dup` = 最小可用号；`dup2` = 指定号（`old==new` 时原样返回）；`dup3` = `dup2` + flags（`old==new` 是 `EINVAL`） |
| 9 | `dup(fd)` ≡ `fcntl(fd, F_DUPFD, 0)`；但 `F_DUPFD` 是「分配」、`dup2` 是「替换」 |
| 10 | `pread`/`pwrite` 用 `fdget()`（不拿 `f_pos_lock`）→ 不碰共享游标，是并发安全的定位读写 |
| 11 | `readv`/`writev` 段数上限 `IOV_MAX = 1024`；内核 `UIO_FASTIOV = 8` 段以内走栈上数组 |
| 12 | `ftruncate` 只读 fd → `EINVAL`；`truncate` 按路径 → `EACCES`；负长度 → `EINVAL`；对目录 → `EISDIR` |
| 13 | `truncate` 变长 = **洞**：`st_size` 涨、`st_blocks` 不涨（实测 8192 vs 4096，差 2 倍） |
| 14 | `O_NONBLOCK` → `EAGAIN`/`EWOULDBLOCK`（同值 `11`）；被信号打断 → `EINTR`（`4`）；`SA_RESTART` 会让后者消失 |
| 15 | 大文件三道门：`off_t` **宽度** / `RLIMIT_FSIZE` **资源** / `s_maxbytes` **文件系统** |
| 16 | `F_GETFL` 会比入参多出 `O_LARGEFILE`（`0x8000`），是内核在 `fs/open.c:1443-1444` 补的；glibc 的宏在 x86-64 上是 `0` |
| 17 | `/dev/fd` 只是指向 `/proc/self/fd` 的符号链接，且**可能不存在** |
| 18 | `open("/proc/self/fd/N")` **≠ `dup(N)`**：新表项、独立偏移（内核只复制 `f_path`） |
| 19 | `unlink` 后仍可从 `/proc/self/fd/N` 打开（引用 inode，不是名字）；` (deleted)` 后缀**有歧义** |
| 20 | `fdinfo` 里的 `O_CLOEXEC` 是内核合成出来的（它不在 `f_flags` 里） |
| 21 | `mkstemp` 原子性 = `open(O_CREAT\|O_EXCL\|O_RDWR, 0600)`；模板**就地改写**，权限**强制 0600** |
| 22 | `mktemp`/`tmpnam` = 只取名不创建 → TOCTOU；glibc 链接期就警告 |
| 23 | `tmpfile()` = 先试 `O_TMPFILE`（`P_tmpdir` → `/tmp`），失败才退到「`mkstemp`+`unlink`」 |
| 24 | `O_TMPFILE` 的文件**从来没有名字**（`d_tmpfile` 直接 `nlink--`），名字 `#<ino>` 是内核编的 |
| 25 | `O_TMPFILE\|O_EXCL` 的 `O_EXCL` = 「不许挂名」（`I_LINKABLE`），**不是**「已存在就失败」 |
| 26 | 给匿名文件挂名两条路：`linkat(fd, "", ..., AT_EMPTY_PATH)`（需 `CAP_DAC_READ_SEARCH`）或 `/proc/self/fd/N` + `AT_SYMLINK_FOLLOW`（不需特权） |
| 27 | 原书本章 **6 个**示例文件（2 个 listing + 2 个习题解答 + `t_readv` 以外的 `t_truncate`），全在本仓库 `code/` 里逐字镜像 |
| 28 | 本章 **14 节**：5.1 原子性 → 5.2 `fcntl` → 5.3 状态标志 → 5.4 三层 → 5.5 复制 → 5.6 定位读写 → 5.7 散布聚集 → 5.8 截断 → 5.9 非阻塞 → 5.10 大文件 → 5.11 `/dev/fd` → 5.12 临时文件 → 5.13 小结 → 5.14 习题 |

---

## 参考

- Kerrisk, *The Linux Programming Interface*, **Chapter 5 — File I/O: Further Details**
- [man7 官方源码清单（按章）](https://man7.org/tlpi/code/online/all_files_by_chapter.html) · [OUTLINE](../OUTLINE.md) · [Ch4 通用 I/O 模型](../chapter-04-file-io-universal/README.md) · [Ch6 进程](../chapter-06-processes/README.md)
- 内核源码（**v6.6**）：`fs/fcntl.c`（`SETFL_MASK`/`setfl`/`do_fcntl`/`f_setown`）、`fs/file.c`（`alloc_fd`/`do_dup2`/`ksys_dup3`/`f_dupfd`）、`fs/open.c`（`build_open_flags`/`do_dentry_open`/`do_truncate`/`vfs_truncate`/`do_sys_ftruncate`）、`fs/read_write.c`（`ksys_read`/`ksys_pread64`/`vfs_readv`/`generic_write_checks_count`）、`fs/pipe.c`（`pipe_read`/`pipe_write`/`fifo_open`/`pipe_fcntl`/`pipe_fasync`）、`fs/proc/fd.c`（`proc_fd_link`/`seq_show`/`tid_fd_update_inode`）、`fs/namei.c`（`vfs_tmpfile`/`vfs_link`/`do_linkat`/`getname_uflags`）、`fs/dcache.c`（`d_tmpfile`）、`fs/d_path.c`（`d_path`）、`mm/shmem.c`（`shmem_tmpfile`）
- 头文件：`include/linux/fs.h`（`struct file` / `struct inode` / `MAX_NON_LFS`）、`include/linux/fdtable.h`、`include/uapi/linux/uio.h`（`struct iovec` / `UIO_MAXIOV`）、`include/uapi/asm-generic/fcntl.h`（`__O_TMPFILE` / `O_TMPFILE`）
- glibc **2.39**：`sysdeps/unix/sysv/linux/open.c`、`sysdeps/unix/sysv/linux/x86/bits/fcntl.h`、`sysdeps/unix/sysv/linux/bits/fcntl-linux.h`、`misc/mkstemp.c`、`misc/mkostemp.c`、`misc/mkdtemp.c`、`misc/mktemp.c`、`sysdeps/posix/tempname.c`、`stdio-common/tmpfile.c`、`sysdeps/unix/sysv/linux/gentempfd.c`
- man-pages **6.19**（2026-02-08）：`open(2)`、`mkstemp(3)`、`tmpfile(3)`、`proc_pid_fd(5)`、`dup(2)`、`fcntl(2)`、`readv(2)`、`truncate(2)`

---

## 代码示例

本章 `code/` 下有：

- **17 个自编 demo**（`c5_*.c` 12 个 + `ex5_*.c` 5 个）
- **6 个原书文件**逐字镜像（`bad_exclusive_open.c` = Listing 5-1、`t_readv.c` = Listing 5-2、`large_file.c` = Listing 5-3、`atomic_append.c` = 习题 5-3 解答、`multi_descriptors.c` = 习题 5-6 解答、`t_truncate.c`）
- **1 个框架替身** `tlpi_hdr.h`（原书 `lib/tlpi_hdr.h` 的最小可用子集：`errExit` / `fatal` / `usageErr` / `cmdLineErr` / `getInt` / `getLong`）

全部在 Compiler Explorer（gcc 13.3.0 / x86-64 / Ubuntu 24.04）上真实编译 + 运行过，共 **35 个作业**，全部 `build code = 0` / `didExecute = True` / `diagnostics = 0`。输出原样抄在对应笔记的实测块里。完整索引见 [`code/README.md`](code/README.md)。

| 文件 | 对应节 | 演示什么 | 需要什么 |
|------|--------|----------|---------|
| `c5_1_atomicity.c` | 5.1 | `O_APPEND` vs `lseek+write` 的并发对照（一个不丢 vs 丢 8236）；无 `O_EXCL` 时两个进程**都**说「我创建了它」；`O_EXCL` 唯一赢家 + `EEXIST` | `fork` |
| `c5_2_fcntl.c` | 5.2 | 8 组 `fcntl` 命令逐组实测：`F_GETFL`/`F_SETFL`、`F_GETFD`/`F_SETFD`、`F_DUPFD`/`F_DUPFD_CLOEXEC`、`F_GETOWN`/`F_SETOWN`；未知命令 `EINVAL` vs `EBADF` | — |
| `c5_3_status_flags.c` | 5.3 | 两类标志分层；`O_ASYNC` 静默失败（普通文件 vs 管道）；`O_CREAT\|O_TRUNC\|O_EXCL` 返回 0 却一位没变；访问模式改不了；`dup` 共享状态标志 | — |
| `c5_4_fd_open_files.c` | 5.4 | 三层结构现场：`fd1=3`/`fd2=4`(dup)/`fd3=5`(open)；六轮「内容 + 三个偏移」；`st_ino` 相同；只对 fd1 设 `O_APPEND` 时 fd3 不受影响 | — |
| `c5_5_dup.c` | 5.5 | `dup` 抢 0 号；`dup2` 共享偏移；`dup2(fd,fd)` 与 `dup2(999,999)`；`dup3(fd,fd,0)` → `EINVAL`；`CLOEXEC` 差异；错误路径 | — |
| `c5_6_pread_pwrite.c` | 5.6 | `pread` 不动 `f_pos` vs `lseek+read` 会动；`pwrite` 超尾写（`st_size=1001`/物理 `4096`）；管道上 `ESPIPE`；同 fd 上并发读三段互不干扰 | — |
| `c5_7_readv_writev.c` | 5.7 | 一次 `writev` 写三段（108 B）；一次 `readv` 读回并 `memcmp`；`iovcnt` 可小于数组长度；`IOV_MAX = 1024`；管道上 `writev` 部分写 | — |
| `c5_8_truncate.c` | 5.8 | `ftruncate` 变短/变长；**8192 逻辑 / 4096 物理** vs **写满 8192**（差 2 倍）；`truncate(path, n)`；四条失败路径（`ENOENT`/`EINVAL`/`EISDIR`/负长度）；截短后游标留在原处 | — |
| `c5_9_nonblocking.c` | 5.9 | 空管道非阻塞读 → `EAGAIN`（= `EWOULDBLOCK`）；清掉 `O_NONBLOCK` 后被 `SIGALRM` 打断 → `EINTR`；有数据立刻返回；写满 65536 后 `EAGAIN` | — |
| `c5_10_large_files.c` | 5.10 | `sizeof(off_t)`；`RLIMIT_FSIZE`；`lseek(4 GiB)` 成功但**文件不变大**；`SIGXFSZ` 杀进程 vs 装 handler 后 `EFBIG`；`0x8000` 之谜的两侧源码对照 | `fork` |
| `c5_11_dev_fd.c` | 5.11 | `/dev/fd` 不存在（`ENOENT`）→ 走 `/proc/self/fd`；`readlink` 看真名；**`open("/proc/self/fd/N")` 不共享偏移**（原 fd `f_pos` 仍是 0）；`unlink` 后仍能重新打开 | — |
| `c5_12_temp_files.c` | 5.12 | `mkstemp` 模板改写 + `0600` + `unlink` 后仍可读；`mkostemp(O_CLOEXEC)` → `F_GETFD=0x1`；`tmpfile()` → `/tmp/#4 (deleted)`；`O_TMPFILE` + `linkat` 挂名后读回 `"anonymous!"` | `_GNU_SOURCE` |
| `ex5_1_large_file_offset_bits.c` | 5.14 练习 5-1 | 标准接口 + `_FILE_OFFSET_BITS=64` 改写 Listing 5-3；`sizeof(off_t)=8`；`lseek(4 GiB)` 成功、`write` 被 `RLIMIT_FSIZE` 拦（`EFBIG` + `SIGXFSZ` handler） | — |
| `ex5_2_append_seek.c` | 5.14 练习 5-2 | 带 `O_APPEND` 时 `lseek(0)` 后写 → 数据**仍在末尾**；对照组（不带 `O_APPEND`）→ 从 0 覆盖 | — |
| `ex5_4_my_dup.c` | 5.14 练习 5-4 | 用 `fcntl`+`close` 实现 `my_dup`/`my_dup2`；与真 `dup`/`dup2` 行为对照；`old==new` 特例 + `EBADF` 校验；目标号被占用时先关再复制 | — |
| `ex5_5_shared_offset_flags.c` | 5.14 练习 5-5 | 偏移共享 / 状态标志共享 / **`FD_CLOEXEC` 不共享**；三层结构各管什么的小结 | — |
| `ex5_7_readv_by_read.c` | 5.14 练习 5-7 | 用 `read`/`write`+`malloc` 实现 `my_readv`/`my_writev`；与真 `writev` 逐字节对照；**短读时不越界** | — |
| `bad_exclusive_open.c` | 5.1 | **原书 Listing 5-1**：`O_EXCL` 失败后退回普通 `open` 的反面教材 | `tlpi_hdr.h` |
| `t_readv.c` | 5.7 | **原书 Listing 5-2**：`totRequired`（= `sizeof(struct stat) + 100` = `248`）vs 实际读到字节数 | `tlpi_hdr.h` |
| `large_file.c` | 5.10 | **原书 Listing 5-3**：已废弃的 LFS API（`_LARGEFILE64_SOURCE` + `open64`/`lseek64`） | `tlpi_hdr.h` |
| `atomic_append.c` | 5.14 练习 5-3 | **原书习题 5-3 解答**：`file num-bytes [x]`，`x` 时改用 `lseek+write` | `tlpi_hdr.h` |
| `multi_descriptors.c` | 5.14 练习 5-6 | **原书习题 5-6 解答**：每写一次 `cat` 一次（沙箱无 `/bin/sh`，故无输出） | `tlpi_hdr.h` |
| `t_truncate.c` | 5.8 | **原书分发文件**：`t_truncate file length` | `tlpi_hdr.h` |

一次编完全部自编 demo（在 `code/` 目录下）：

```bash
for f in c5_*.c ex5_*.c; do
    gcc -O0 -Wall -Wextra -o "${f%.c}" "$f" || echo "FAIL $f"
done
```

6 个原书程序都需要 `tlpi_hdr.h` 替身：

```bash
for f in bad_exclusive_open t_readv large_file atomic_append multi_descriptors t_truncate; do
    gcc -O0 -Wall -Wextra -o "$f" "$f.c" || echo "FAIL $f"
done
```

带参数的用法：

```bash
./c5_12_temp_files                                  # 临时文件五段实测
./c5_10_large_files                                 # 大文件与 SIGXFSZ
./ex5_1_large_file_offset_bits /tmp/c5_lfs.bin 4294967296
./bad_exclusive_open /dev/null                      # 原书 Listing 5-1：已存在
./bad_exclusive_open /tmp/c5_excl_new               # 原书 Listing 5-1：创建成功
./t_readv /proc/self/maps                           # 原书 Listing 5-2：248 / 248
./t_readv /proc/version                             # 原书 Listing 5-2：248 / 219（短读）
./large_file /tmp/c5_large.bin 4294967296           # 原书 Listing 5-3（沙箱里被 SIGXFSZ 杀）
./t_truncate /proc/version 10                       # 原书 t_truncate：Permission denied
# 并发那一组必须同时起两个进程：
./atomic_append /tmp/f1.bin 200000 & ./atomic_append /tmp/f1.bin 200000
./atomic_append /tmp/f2.bin 200000 x & ./atomic_append /tmp/f2.bin 200000 x
```

**几条实测结论**（都是本仓库跑出来的，不是书上抄的）：

- **`O_APPEND` 一个不丢**：两进程各写 20 万字节 → `st_size = 400000`；`lseek+write` 同样负载 → `391764`（**丢 8236**）
- **`F_SETFL` 静默丢弃**：`O_CREAT|O_TRUNC|O_EXCL` 返回 `0`，`F_GETFL` 从 `0x08001` 到 `0x08001`（变化 `0x00000`）
- **`O_ASYNC` 要钩子**：普通文件上 `F_SETFL(O_ASYNC)` 后该位仍为 `0`；同一段代码换管道 → `F_GETFL = 0x02000`，该位为 `1`
- **`dup` 会抢 0 号**：`close(0)` 之后 `dup(fd)` 返回 `0`
- **`dup3(fd, fd, 0)` → `EINVAL`（22）**；而 `dup2(fd, fd) -> 3`（原样返回）
- **`pread` 不动游标**：读 4 字节后 `f_pos` 仍是 `10`；`lseek+read` 之后变成 `4`
- **`pwrite` 超尾写不省空间**：`pwrite(offset=1000)` 后 `st_size=1001`、`st_blocks*512=4096`（未跨块）
- **洞的物理差**：`ftruncate` 撑到 `8192` → 物理 `4096`；真写满 `8192` → 物理 `8192`（**2 倍**）
- **`ftruncate` 只读 fd 是 `EINVAL`（22）**，而 `truncate /proc/version` 是 `EACCES`（`Permission denied`）
- **`EAGAIN` vs `EINTR`**：空管道非阻塞读 → `11`；清掉 `O_NONBLOCK` 后 `SIGALRM` 打断 → `4`
- **`SIGXFSZ`**：4 GiB 处 `write` 默认动作杀掉进程（`signal 25`）；装 handler 后 `errno=27 (EFBIG)`
- **`O_LARGEFILE` 错位**：glibc 的宏 = `0x0`，内核侧 `F_GETFL & 0x8000` = `1`，`F_GETFL = 0x08002`
- **`/proc/self/fd/N` ≠ `dup`**：经由它读 4 字节后，**原 fd 的 `f_pos` 仍是 0**
- **`unlink` 后仍可重开**：`open("/proc/self/fd/3") -> 6`，读回 `"0123"`；`readlink` 显示 `... (deleted)`
- **`mkstemp` 的指纹**：模板 `"/tmp/c5tmpXXXXXX"` → `"/tmp/c5tmpEE7wgl"`，权限 `0600`
- **`tmpfile()` 的真身**：`/proc/self/fd/3` 指向 `/tmp/#4 (deleted)`——是 `O_TMPFILE` 无名 inode，`#4` = inode 号
- **`O_TMPFILE` 挂名**：`linkat` 到 `/tmp/c5_linked.txt` 后能读回 `"anonymous!"`
- **原书 `t_readv` 的两个数字**：`/proc/self/maps` → `248 / 248`；`/proc/version` → `Read fewer bytes than requested` + `248 / 219`
