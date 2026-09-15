# TLPI 第 13 章 — File I/O Buffering

**优先级**：🔴（日志持久化 / 高性能 I/O / DB 存储引擎 / 任何「写成功但数据没了」的事故）
**前置**：[Ch04 Universal I/O](../chapter-04-file-io-universal/README.md)（`read`/`write` 的语义） · [Ch05 Further I/O](../chapter-05-file-io-further/README.md)（`lseek` / `O_SYNC` 标志位） · [Ch12 `/proc`](../chapter-12-system-process-info/README.md)（`/proc/vmstat` 的脏页计数器、`/proc/sys/vm/dirty_*`）
**后置**：[Ch14 File Systems](../chapter-14-file-systems/README.md)（页缓存的落点：inode / 日志 / 回写） · [Ch49 mmap](../chapter-49-memory-mappings/README.md)（同一份页缓存的另一种映射方式） · [Ch63 Alternative I/O](../chapter-63-alternative-i-o-models/notes/63.4-the-epoll-api.md)（`O_DIRECT` 之外的异步路径；⚠️ 该章目前**还没有 README**，所以这里链到它现存的一篇笔记）

---

## 小节目录

- [13.1 Kernel Buffering of File I/O: The Buffer Cache 内核缓冲](notes/13.1-buffer-page-cache.md)
- [13.2 Buffering in the stdio Library stdio 库的缓冲](notes/13.2-stdio-file.md)
- [13.3 Controlling Kernel Buffering of File I/O 控制内核缓冲](notes/13.3-controlling-kernel-buffering.md)
- [13.4 Summary of I/O Buffering 缓冲总结](notes/13.4-summary-buffering.md)
- [13.5 Advising the Kernel About I/O Patterns 给内核 I/O 模式提示](notes/13.5-advising-kernel.md)
- [13.6 Bypassing the Buffer Cache: Direct I/O 绕过页缓存](notes/13.6-direct-io.md)
- [13.7 Mixing Library Functions and System Calls for File I/O 混用库函数与系统调用](notes/13.7-mixing-stdio-syscalls.md)
- [13.8 Summary 本章总结](notes/13.8-summary.md)
- [13.9 Exercises 练习](notes/13.9-exercises.md)

> 九节的划分与 TLPI 原书 TOC 一致（13.1–13.9）。本章**没有任何子编号小节**（不像 Ch12 有 12.1.1），所以**一节一篇**，节内的子标题收在同一篇里用 `###`。
>
> ⚠️ **13.5 的标题有两个版本**：`blog.man7.org` 的**早期**章节表写作 `Giving the Kernel Hints about I/O Patterns: posix_fadvise()`，**成书版 TOC** 是 `Advising the Kernel About I/O Patterns`。本仓库按**成书版**命名（`notes/13.5-advising-kernel.md`），并在该篇正文里同时记下两个版本。同一节的 API 是 `posix_fadvise(2)`，两种标题都指它。
>
> ⚠️ **习题正文的公开原文未能逐字核验（诚实标注）**：man7 只分发**源码**、不放习题正文。13-1 / 13-2 / 13-3 / 13-5 的题干是**转引**（两个独立公开解答仓库 + 一个中文译本，三家文字一致），**不是官方分发原文**。唯一的例外是 **13-4**：它的官方解答程序 `filebuff/mix23_linebuff.c` 是公开的（标注 "Solution to Exercise 13-4, page 250"），所以那一题的题面可以从解答程序**反推**，推理依据写在 13.9 里。

---

## 章节目标

- **把「两层缓冲」变成肌肉记忆**：`FILE*`（libc 用户态）→ **页缓存**（内核）→ 设备。`fflush()` 只推第一层，`fsync()` 只推第二层，**缺哪一层都会丢数据**
- **`write()` 返回 ≠ 落盘**：本章 ① 用「另一个 fd 立刻读得到」证明数据已进内核，② 用**脏页计数器**（`nr_dirty`）证明它还没上磁盘。两件事必须分开说
- **知道「stdout 到终端是行缓冲」不是约定，是代码**：glibc `libio/filedoalloc.c` 里**只有** `S_ISCHR(st.st_mode) && isatty(fd)` 才设 `_IO_LINE_BUF`；缓冲大小取 `min(BUFSIZ, st_blksize)`。这两条判据能解释全部现象
- **`fsync` 失败是 `EINVAL` 不是 `ESPIPE`**：内核 `fs/sync.c:180-190` 的 `if (!file->f_op->fsync) return -EINVAL;`——`/proc` 伪文件、`/dev/null`、管道**根本没有这个回调**
- **`posix_fadvise()` 返回的是错误码，`errno` 全程为 0**：`if (posix_fadvise(...) == -1) perror(...)` 是**永远不成立**的写法
- **`O_DIRECT` 的对齐是「两套值」**：偏移/长度按 `bdev_logical_block_size`、缓冲区地址按 `bdev_dma_alignment + 1`。书上「三样都按逻辑块大小对齐」是简化说法，实测 ext4 上 1 字节对齐的缓冲也能通过
- **`O_DIRECT ≠ O_SYNC`**：绕过页缓存不等于落盘；同一个 `O_DIRECT` fd 上 `fsync()` 依然返回 0（设备自己的写缓存还在）
- **赌上一切也不要无保护地混用 stdio 与 `read`/`write`**：乱序可复现（`[BBBBAAAA]`）、`dup` 救不了、外部 `lseek` 会造出 NUL 空洞
- **诚实**：本章 glibc 2.39 与 Linux v6.6 的坐标全部实读核准；**纠正了书上一处**（`O_DIRECT` 的三样对齐）；并明确标注 CE 沙箱**做不到**的三件事（无 pty ⇒ 观察不到「终端行缓冲」那一半；`RLIMIT_FSIZE=16 MiB`；`/tmp` 是 tmpfs ⇒ `O_SYNC`/`O_DIRECT` 的代价与语义失真）
- **溯源**：本章 **9 篇**笔记里的每一行实测输出都能在 CE 冻结日志 `tlpi-ch13-final.txt` 里定位（共 411 行抽检、0 未命中）

### 一条主线：所有反直觉行为，都来自「有两层缓冲」

| 你以为 | 实际 |
|--------|------|
| `fprintf(fp, "x")` 之后数据就在文件里 | 还在**用户态数组**里。另开 fd 看 `st_size = 0`（实测） |
| `fflush(fp)` 之后数据就安全了 | 只到**页缓存**。此刻拔电源就没了——`fflush` 没有任何持久化语义 |
| `fsync(fileno(fp))` 就能保住数据 | **它看不见 stdio 缓冲**。实测：只调 `fsync` 不 `fflush`，`fsync` 返回 **0** 而文件仍是 **0** 字节 |
| `write()` 返回 16 就说明写了 16 字节到文件 | 只说明**拷进了内核页缓存**（实测「另开 fd 立刻读得到」）；此时 `nr_dirty` 涨了 4092 页 |
| `close()` 很危险，会丢数据 | `fclose()` 会**隐式 `fflush`**（标准规定），所以它反而比「不 `fflush` 也不 `fclose`」安全——后者正是 `SIGKILL` 时的情形 |
| 开 `O_DIRECT` 就绕过了所有缓冲 | 只绕过**页缓存**；设备写缓存还在，该 `fsync` 还得 `fsync` |
| `O_SYNC` 能让文件更安全 | 它管的是 `write(2)`，**管不到 stdio 缓冲**。实测：`fprintf` 后 `st_size` 仍是 **0**，`fflush` 之后才变 **51** |
| `posix_fadvise` 失败了会返回 `-1` 并设 `errno` | 它**返回错误码本身**（`22`/`29`/`9`），`errno` **全程 0**——`perror()` 什么都打不出来 |
| 块大小只影响性能 | 它决定**系统调用次数**：写 1 MiB，`buf-size` 从 1 加到 65536，`write()` 次数从 **1048576** 降到 **16**（差 4.8 个数量级） |
| stdout 重定向之后就看不到输出了 | 是**全缓冲**在作怪（`socket`/管道/文件都不满足行缓冲判据）。这是「日志文件里少了几行」这一类事故的根因 |

一句话：**本章没有新 API，只有「同一份数据在三个地方各有一份拷贝」这个事实，以及它带来的全部后果。**

---

## 原书示例清单（man7 官方按章文件列表）

| Listing | 文件 | 页码 | 作用 |
|---------|------|------|------|
| **13-1** | `filebuff/direct_read.c` | **p.247** | 用 `O_DIRECT` 读文件：`getLong` 取 `length`/`offset`/`alignment`，`posix_memalign` 分配对齐缓冲，`read` 之后 `write(STDOUT_FILENO)` |
| **习题 13-4 解答** | `filebuff/mix23_linebuff.c` | **p.250**（习题页） | **Solution to Exercise 13-4**：显式 `setvbuf(stdout, ..., _IOLBF, ...)`，于是 `printf` 先出、`write` 后出 |
| 未编号 | `filebuff/mix23io.c` | — | 原书声明 "This file is not printed in the book"：故意混用 stdio 与 `write()`，演示全缓冲下的**相反**输出顺序 |
| 未编号 | `filebuff/write_bytes.c` | — | 原书声明是 "a supplementary file for Chapter 13"：文件 I/O 基准程序，带 `-DUSE_O_SYNC` / `-DUSE_FSYNC` / `-DUSE_FDATASYNC` 三个编译期开关 |

> 出处：[man7 — List of source code files, by chapter](https://man7.org/tlpi/code/online/all_files_by_chapter.html) 的 **Chapter 13** 一节——共 **4 个**文件，**全部**在 `filebuff/` 下。
> 本仓库在 `code/` 下**逐字镜像**了这 4 个文件（`sha256` 与原书下载件一致），并额外提供 `tlpi_hdr.h` / `get_num.h` 两个**最小替身**让它们能单文件编译——原书的这两个头依赖整个 `lib/` 目录（`lib/error_functions.c`、`lib/get_num.c`），且 `lib/get_num.c` 里带有以 `@` 结尾标记「已完成」的填充区。替身的语义逐条对照原书实现校准过（见 [`code/README.md`](code/README.md) 的「替身与原书的差异表」）。

三个容易踩的坑：

| 容易踩的坑 | 正解 |
|-----------|------|
| 以为「混用 stdio 与 `write` 只是顺序难看」 | 它是**真丢数据**：`fprintf` 之后直接 `write` 同一个 fd，实测文件内容是 `[BBBBAAAA]`；而更隐蔽的是**用另一个 fd 去 `lseek`**，会让 stdio 把字节写到你没想到的位置（实测造出 `[~100~AAAA]` 的 100 字节 NUL 空洞） |
| 以为 `O_DIRECT` 需要三样都按 **4096** 对齐 | **书上这里是简化说法**。实测 ext4 上 `len=100` / `off=1` 确实 `EINVAL`，但**缓冲区地址**从 1 字节对齐到 512 字节对齐**全部成功**。内核是两处判据、两个不同的值（`bdev_logical_block_size` vs `bdev_dma_alignment + 1`） |
| 以为 `posix_fadvise` 返回 `-1` | 它**返回错误码**（`EINVAL=22` / `ESPIPE=29` / `EBADF=9`），`errno` 不动。正确写法是 `int ret = posix_fadvise(...); if (ret != 0) fprintf(stderr, "%s", strerror(ret));` |

---

## 易错清单

1. **`write()` 返回成功 ≠ 数据落盘** —— 它只保证「拷进内核页缓存」。要落盘只有 `fsync` / `fdatasync` / `sync` / `O_SYNC` / `O_DSYNC` 这几条路（[13.1](notes/13.1-buffer-page-cache.md) / [13.3](notes/13.3-controlling-kernel-buffering.md)）。
2. **`fflush()` 与 `fsync()` 不是可选项，是一条链的两棒** —— 实测：`fprintf` 21 字节后只调 `fsync(fileno(fp))`，返回 **0**（成功）而另一个 fd 看到的文件大小仍是 **0**；补上 `fflush` 才变 **21**。**`fsync` 返回 0 不代表你的数据安全了**。
3. **`fsync()` 失败一律是 `EINVAL(22)`，不是 `ESPIPE(29)`** —— `/proc/version`、`/dev/null`、管道（读端与写端）实测全是 `-1`/`22`。根因在 `fs/sync.c:180-190`：这些 `file_operations` **没有 `.fsync` 回调**，在 VFS 层就被判 `EINVAL`。
4. **只读 fd 上 `fsync()` 也返回 0** —— `do_fsync()` 只做 `fdget()` + `vfs_fsync()`，**不看打开模式**。所以 `fsync(open(path, O_RDONLY))` 合法（但没用）。
5. **`fsync` 与 `fdatasync` 的唯一内核差别** —— `if (!datasync && (inode->i_state & I_DIRTY_TIME)) mark_inode_dirty_sync(inode);`。即：`fsync` 会把「只有时间戳脏」的 inode 提升为真正要同步；`fdatasync` 可以合法地什么都不写。**所以「`fdatasync` 更快」只在你不在乎时间戳时成立**。
6. **`O_SYNC` 管不到 stdio 缓冲** —— 它是给 `write(2)` 的语义，而 stdio 在它**上游**。实测：`fprintf` 之后立刻 `fstat(fd).st_size = 0`，`fflush` 之后才 = **51**。**FILE\* 上不 `fflush`，`O_SYNC` 一点用都没有**。
7. **`O_SYNC` 的代价随「块越小」指数恶化** —— 实测写 1 MiB（`buf=4096`）：普通 `O_WRONLY` **0.0007 s** vs `O_WRONLY|O_SYNC` **0.7527 s**。原书 Table 13-3 更极端：`BUF_SIZE=1` 写 1 MB，不带 `O_SYNC` 约 0.7 秒、带 `O_SYNC` 约 **1030 秒**。**本仓库故意没跑这一档**——不是测不出来，是测出来要上千秒。
8. **`O_RSYNC == O_SYNC`（Linux）** —— 实测两者都是 `04010000`。SUSv3 给 `O_RSYNC` 定义了「读也同步」，**Linux 没有实现**，直接把它定义成 `O_SYNC` 的同义词。这是「标准说要、实现不做」的经典例子。
9. **`O_DIRECT ≠ O_SYNC`** —— `O_DIRECT` 打开的 fd 上 `fsync()` 依然返回 **0**：绕过页缓存只是「不经过内核这块内存」，**设备自己的写缓存还在**。两者要叠加用。
10. **`O_DIRECT` 的对齐是两套值，而且都随设备变** —— `fs/iomap/direct-io.c:291-293` 管偏移/长度（`bdev_logical_block_size`），`include/linux/blkdev.h:1320-1325` 管缓冲区地址（`bdev_dma_alignment + 1`）。**别写死 4096**，用 `statx(STATX_DIOALIGN)` 问。
11. **`statx(STATX_DIOALIGN)` 可能什么都没告诉你** —— 实测 CE 上对 ext4 与 tmpfs 都是「`statx` 成功但 `stx_mask=0x173f` 不含 `DIOALIGN`」。**字段不存在 ≠ 对齐要求是 0**，得回退到保守值 + 试错。
12. **同一段 `O_DIRECT` 代码在 tmpfs 上「跑得通」不代表在 ext4 上能跑** —— 实测 `/tmp`（tmpfs）上 `len=100`、`off=1` **全部成功**；`/app`（ext4）上同样两个用例 **`EINVAL`**。tmpfs 上的 `O_DIRECT` 是**假的**（它压根不做对齐检查）。**必须在目标文件系统上验**。
13. **缓冲区地址的对齐在 ext4 上也没被强制** —— 实测 1 / 2 / 4 / 8 / 16 / 64 / 256 / 512 字节对齐**全部成功**。这与「必须 4096 对齐」的流传说法不符（根因见第 10 条：地址用的是 `bdev_dma_alignment`，本机队列参数给出的下界很小）。
14. **不是所有文件都能 `O_DIRECT`** —— 实测 `open("/dev/zero", O_RDONLY|O_DIRECT)` → `EINVAL(22)`；`open("/proc/version", ...)` 同样 `EINVAL`；而 `/etc/passwd` 在容器里位于 tmpfs 上，**能打开但那是个假的 `O_DIRECT`**。
15. **`posix_fadvise()` 返回错误码、不设 `errno`** —— 实测六种合法 advice 全返回 `0`；`advice=9999` → **22**；管道 → **29**；`fd=9999` / 已 `close` 的 fd → **9**；`/proc/version` → **0**。**五种情况 `errno` 全是 0**，`perror()` 什么都打不出来。根因是 glibc `sysdeps/unix/sysv/linux/posix_fadvise.c` 的 `if (INTERNAL_SYSCALL_ERROR_P (ret)) return INTERNAL_SYSCALL_ERRNO (ret); return 0;`。
16. **`posix_fadvise()` 只是建议** —— 返回 `0` **不代表**内核照做了。`POSIX_FADV_DONTNEED` 只影响**页缓存**，**不是 `fsync` 的替代品**，也不会把脏页丢掉（对脏页调它，内核会先把它们写回）。`POSIX_FADV_NOREUSE` 在 Linux 上基本是空操作。
17. **`posix_fadvise` 对 `/proc` 伪文件不报错** —— 实测返回 `0`。别把「`/proc` 上能调通」当成「这个调用有效」。
18. **「stdout 到终端是行缓冲」是代码不是约定** —— glibc `libio/filedoalloc.c` 的 `_IO_file_doallocate()`：**只有** `S_ISCHR(st.st_mode)` 成立**且** `isatty(fd)` 为真才 `fp->_flags |= _IO_LINE_BUF`。管道 / socket / 普通文件**一条都不满足**。
19. **`__fbufsize(stdout) ≠ BUFSIZ`** —— 实测 `BUFSIZ = 8192` 而 `__fbufsize(stdout) = 4096`。因为同一段代码里有 `if (st.st_blksize > 0 && st.st_blksize < BUFSIZ) size = st.st_blksize;`，而 socket 的 `st_blksize = 4096`。**读代码只看 `BUFSIZ` 会算错缓冲区**。
20. **`setvbuf()` 返回 0 不代表生效** —— 实测对**已经用过**的流再 `setvbuf(stdout, NULL, _IOFBF, 0)`，返回 **0**（成功）而 `__fbufsize` **一个字没变**（还是 `64`）。这是本章第二例「返回成功 ≠ 效果发生」（第一例是 `posix_fadvise`，跨章还有 Ch12 的 `/proc/sys` 写入）。
21. **`st_blksize` 只对普通文件/目录有意义** —— 实测 `/proc/version` 是 **1024**，而 `/`、`/tmp`、`/etc/passwd`、`/dev/null`、`/dev/zero` 全是 **4096**。在字符设备上这个值是**无用的**。`st_blksize` 是「首选 I/O 块大小」的**建议**，不是容量或扇区大小。
22. **小 IO 下 `nr_dirty` 不是有效指标** —— 实测 13.1 写 **16 MiB** 时 `nr_dirty` 增量 = **4092** 页（理论 4096 页，对得上）；而 13.4 只写 **16 字节**时 `nr_dirty` 全程 **14**（**看不出变化**）。**同一组计数器要用在匹配的量级上**，否则会得出「写 16 字节不脏页」这种荒谬结论。小 IO 请改用「另一个 fd 的 `st_size` + 可见性」判断。
23. **`nr_dirty` 是整机（宿主）口径，绝对值必然漂移** —— 别的容器也在写盘。但「**增量 == 页数**」和「`nr_dirty × 4 = Dirty`（kB）」在**同一次运行内**恒成立，可以当场对账（实测 `4551 × 4 = 18204`）。
24. **`O_SYNC` 在小块上是乘法陷阱** —— 同步代价 × `write()` 次数。`write()` 次数 = 总字节 / `buf-size`，所以**小缓冲 + `O_SYNC`** 是最坏组合（这也是「不要为了保险全局挂 `O_SYNC`」的定量理由）。
25. **「同步模式」的 benchmark 必须先算规模** —— 本仓库的 `ex13_2` 把「每次 `write` 后 `fsync`/`fdatasync`」的规模从 1 MiB **缩到 256 KiB**（文件头注释里写明是**故意**的）：否则 512 字节缓冲 + 每次 `fsync` = 2048 次设备往返。**「按模式定规模」是这类 benchmark 的正确做法**，不是偷懒。
26. **`dup()` 救不了混用** —— 实测用 `dup` 出来的 fd 去 `write`，文件内容仍然 `[BBBBAAAA]`。`dup` 出来的 fd 与原 fd **共享同一个文件偏移**，你避开的是 **fd 号**，不是 **stdio 缓冲**。
27. **外部 `lseek` 会让 stdio 写错位置** —— 实测先用另一个 fd `lseek(fd, 100, SEEK_SET)`、再 `fprintf` 并 `fclose`，文件里出现 **104** 字节，其中 **100 个 NUL 是空洞**（显示为 `[~100~AAAA]`）。stdio 只知道自己的逻辑偏移，**不知道别人动过 lseek**。
28. **`fclose()` 会隐式 `fflush()`（标准规定）** —— 所以「不 `fflush` 也不 `fclose`」才危险：那正是进程被 `SIGKILL` 时的情形。`fclose` 之后数据可靠地**到了页缓存**——但也只是页缓存。
29. **`fsync(fp)` 编译不过** —— `fp` 是 `FILE *`。必须写 `fsync(fileno(fp))`。`-Wall` 会提醒类型不匹配（`ex13_3` 的注释里专门记了这条）。
30. **反复 `fflush()` 不会更安全** —— 第二次是空操作。真正的成本在 `fsync`，所以「多刷几次保险」只会白烧 I/O 预算。
31. **行缓冲下 `'\n'` 真的会触发刷新，但 CE 上要换个办法看** —— 本沙箱 stdout 是 **socket**（`S_ISSOCK=1`、`isatty=0`）且**没有 pty**（`/dev/ptmx` 不存在、`posix_openpt()` 返回 `ENOENT`），所以「终端 → 行缓冲」这一半**观察不到**。13.2 的 ③ 段用「显式 `setvbuf(_IOLBF)` + `write()` 当参照物 + 事后从管道读回」把机制做成了**可判定**的实验（`__flbf` 从 0 变 1 就是证据）。
32. **`mix23io.c` 与 `mix23_linebuff.c` 是同一段机制的相反两面** —— 前者默认（socket/管道 → 全缓冲）输出顺序是 `write` 先；后者显式 `_IOLBF` 之后是 `printf` 先。实测两份输出的**首行正好相反**，这是本章最干净的一组对照。
33. **「行数 ≠ 换行数」** —— POSIX 的 `tail` 定义里，**最后一段没有结尾换行符的文字也算一行**。用「换行数」当行数的实现会在「进程被 `SIGKILL`、最后一行写不全」的日志上**多输出一行**——**恰恰是最需要 `tail` 的时候**（[13.9](notes/13.9-exercises.md) 习题 13-5 修正一）。
34. **`tail` 的「高效」是从尾部回扫，不是从头读** —— 实测 1000 行 / 82000 字节的样本，`-n 1/100/500/1000` 实际只读了 **4096 / 12288 / 45056 / 82000** 字节。「从头读到尾」的实现这一列会**恒等于 82000**。
35. **`-n 0` 必须单独处理** —— 否则「找不到第 skip 个换行」的 fallback 会变成「输出全部行」。实测 `-n 0` → 空输出。
36. **测试要挑「能抓住 bug 的那一列」** —— 本仓库在调试 `tail` 时踩到一个**未初始化堆空洞**的 bug：`-n 1000` 的「输出**行数**」仍然是 1000（**看不出问题**），只有「输出**首行**」从 `line-0001` 变成了 `line-0253` 才暴露。**「计数类」断言抓不到「顺序类」错误**——列的选择本身是测试设计的一部分。
37. **环境决定一切**：本沙箱**没有 pty**、**`/tmp` 只有 20 MiB 且是 tmpfs**、**`RLIMIT_FSIZE = 16 MiB`（写超了会被 `SIGXFSZ(25)` 杀掉）**、`/etc/passwd` **只有 50 字节 1 条**、`FOPEN_MAX = 16`。所以：`O_SYNC` 的绝对耗时**不可用**（tmpfs 无真实设备）、`O_DIRECT` 在 `/tmp` 上是假的、所有 demo 的规模都必须在 16 MiB 以内、**`SIGXFSZ` 不会刷 stdio 缓冲**（这点比信号本身更容易坑人）。

---

## 章节链路

```text
Ch2  系统调用与内核分工（谁做缓冲、谁做回写）
  → Ch3  syscall 约定 + errno 范式（本章到处是「返回 0 但没生效」）
  → Ch4  read/write 的基础语义（本章讲它们在「缓冲」这件事上到底做了什么）
  → Ch5  lseek / O_SYNC / O_DSYNC 标志位（本章 13.3 的四个打开标志）
  → Ch12 /proc/vmstat 的 nr_dirty、/proc/meminfo 的 Dirty、/proc/sys/vm/dirty_*
  → Ch13 两层缓冲：stdio（libc）→ 页缓存（内核）→ 设备
           13.1  write() 只是拷进页缓存（脏页计数器可对账）
           13.2  stdio 的缓冲模式是代码决定的，不是约定
           13.3  五个刷盘手段的作用域各不相同
           13.4  两层合起来看：「别的 fd 能不能看见」是第一判据
           13.5  posix_fadvise：返回错误码、不设 errno，而且只是建议
           13.6  O_DIRECT：对齐是两套值；绕过缓存 ≠ 落盘
           13.7  混用的全部后果与四种修法
           13.9  五道习题（tail 的实现里藏着「行数≠换行数」与堆空洞）
  → Ch14 页缓存的落点：inode / 日志 / 回写线程（本章只说「页缓存存在」，那一章说「它怎么被管理」）
  → Ch49 mmap 与页缓存是同一份内存的两种视图
  → Ch55 文件锁与 O_SYNC 的顺序关系（锁保护的是元数据操作）
  → Ch63 替代 I/O 模型（O_DIRECT / AIO / io_uring 的定位差别）
```

---

## 双线提示

| 路线 | |
|------|--|
| **HFT** | **落盘顺序就是钱**。① 交易日志（WAL / 成交回报归档）必须写清 `fflush` + `fsync` 两棒：只 `fsync(fileno(fp))` 是**最隐蔽的丢数据方式**（实测返回 0 而文件 0 字节）——要么全程 `write()` 无 stdio，要么每次切换接口前 `fflush`；② **不要全局挂 `O_SYNC`**：它是「每次 `write` 都等设备确认」，小块 + 高频 = 数量级灾难（原书 Table 13-3：`BUF_SIZE=1` 写 1 MB 要 **1030 秒**）。正确做法是**加大块** + 只在关键点 `fsync`/`fdatasync`；③ **`fdatasync` 而不是 `fsync`**：只关心数据、不在乎 mtime 时它更轻（内核唯一差别就是 `I_DIRTY_TIME` 那三行）；④ **延迟敏感路径不要 `O_DIRECT`**：它省的是拷贝与缓存污染，代价是**失去预读**，小 IO 会变成整块设备往返；适合的是自带缓存层的大型存储引擎；⑤ **`posix_fadvise(POSIX_FADV_DONTNEED)`** 是行情回放后释放页缓存的正确姿势（**不是** `fsync` 替代品，也**不会**丢脏页）；⑥ **日志丢了先怀疑缓冲**：stdout 被 `systemd`/Docker 接到 socket 或管道 → **全缓冲** → 崩溃时最后几行永远不在文件里。要么 `setvbuf(stdout, NULL, _IOLBF, 0)`，要么全程 `write(2)`；⑦ **`nr_dirty` 可以当压测的「未落盘量」指标**（写前写后各读一次 `/proc/vmstat`），但记住它是**整机**口径：只做**增量**、不做绝对值 |
| **嵌入式** | 板子上这章的权重比服务器更高，因为**闪存有寿命**。① `O_SYNC` / 每次 `fsync` 在高频日志场景会把 eMMC/SD 的擦写次数迅速烧掉——工业做法是「攒到一定量 + `fdatasync`」或者写环形缓冲区 + 定期落盘；② **`fflush` 只保证进内核**，掉电路径全靠 `fsync`；而**掉电时页缓存全丢**——所以「写完就断电」的设备必须显式同步（或者用带掉电保护的 `fsync` + `O_DSYNC` 组合）；③ **`O_DIRECT` 的可用性要看块设备驱动**：不是所有 MTD/MMC 路径都支持，且对齐要求由**队列参数**决定（`bdev_logical_block_size` / `bdev_dma_alignment`），同一份代码换块板子就可能 `EINVAL`——**必须在目标板上试**；④ **`tmpfs` 上的一切「落盘」结论都是假的**（嵌入式容器/initramfs 常见），`/tmp` 与 `/run` 默认就是 tmpfs；⑤ **`st_blksize` 在嵌入式常被驱动设成不寻常的值**（512/1024），于是「stdio 缓冲多大」也随板子变——想稳定就显式 `setvbuf`；⑥ `CONFIG_*` 决定 `/proc/vmstat` 里有没有 `nr_dirty`、`/proc/sys/vm/dirty_*` 能不能写（只读 rootfs + 精简内核时经常没有） |
| **两条线共同的坑** | 「**看得见**」与「**落盘**」是两件事。`write` 返回、别的 fd 读得到、`fflush` 返回 0 —— 三个都只证明「到了页缓存」。要判落盘只能靠 `fsync` 家族的返回值（还要明白它对 `/proc`、管道、`/dev/null` 一律 `EINVAL`）或内核侧计数器。第二件事：**「返回成功 ≠ 生效」在本章出现三次**（`posix_fadvise` 返回 0 只是建议、`setvbuf` 返回 0 但没改、`/proc/sys` 写入被静默忽略）——这个模式会一直跟到 Ch63 |

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 三层数据路径：**stdio 用户缓冲 → 内核页缓存 → 设备**。`fflush` 推第一层，`fsync` 推第二层 |
| 2 | `write()` 返回 = 字节已进**页缓存**（对别的 fd 立即可见），**不等于**落盘 |
| 3 | `fsync` 失败一律 **`EINVAL(22)`**（不是 `ESPIPE`）：那些 `file_operations` **没有 `.fsync` 回调** |
| 4 | 只读 fd 上 `fsync()` 也返回 **0**：`do_fsync()` **不看打开模式** |
| 5 | `fsync` 与 `fdatasync` 的唯一内核差别：`if (!datasync && (inode->i_state & I_DIRTY_TIME)) mark_inode_dirty_sync(inode);` |
| 6 | `fsync(fileno(fp))` **看不见 stdio 缓冲**；实测返回 0 而文件 0 字节。正确顺序：`fflush(fp)` **再** `fsync(fileno(fp))` |
| 7 | `O_SYNC = 04010000`、`O_DSYNC = 010000`、`O_DIRECT = 040000`，且 **`O_RSYNC == O_SYNC`**（Linux 没实现「读也同步」） |
| 8 | `O_SYNC` 管 `write(2)`，**管不到 stdio**：不 `fflush` 就一点用没有 |
| 9 | `O_SYNC` 代价 = **同步代价 × `write` 次数**；小块 + `O_SYNC` 是最坏组合（书上 `BUF_SIZE=1` 写 1 MB ≈ 1030 s） |
| 10 | `O_DIRECT` **≠** `O_SYNC`：绕过页缓存不解决持久化，设备写缓存还在 |
| 11 | `O_DIRECT` 对齐是**两套值**：偏移/长度 ← `bdev_logical_block_size`；缓冲地址 ← `bdev_dma_alignment + 1` |
| 12 | 实测 **1 字节对齐**的缓冲在 ext4 上也能 `O_DIRECT` 成功 → **书上「三样都按逻辑块对齐」是简化说法** |
| 13 | tmpfs 上的 `O_DIRECT` 是**假的**：`len=100` / `off=1` 全成功，换 ext4 全 `EINVAL`。**必须在目标文件系统验** |
| 14 | `statx(STATX_DIOALIGN)` 可能什么都不报（实测 `stx_mask=0x173f` 不含该位）→ 回退保守值 + 试错 |
| 15 | `posix_fadvise()` **返回错误码**（`22`/`29`/`9`），**`errno` 全程 0**；`== -1` 的判断永远不成立 |
| 16 | `posix_fadvise` 只是**建议**；`DONTNEED` 只影响页缓存，**不是** `fsync` 替代品，也不丢脏页 |
| 17 | 行缓冲判据（glibc `_IO_file_doallocate()`）：**只有** `S_ISCHR(st.st_mode) && isatty(fd)` 才设 `_IO_LINE_BUF` |
| 18 | stdio 缓冲大小 = **`min(BUFSIZ, st_blksize)`**；实测 `BUFSIZ=8192` 而 `__fbufsize(stdout)=4096` |
| 19 | `setvbuf()` 对**已用过**的流可能**返回 0 但不生效**（实测 `__fbufsize` 一个字没变） |
| 20 | `st_blksize`：`/`、`/tmp`、`/etc/passwd`、`/dev/null`、`/dev/zero` 全 **4096**；`/proc/version` 是 **1024**（且对字符设备无意义） |
| 21 | **小 IO 下 `nr_dirty` 无效**：写 16 MiB → 增量 4092 页（理论 4096）；写 16 字节 → 全程 14（看不出） |
| 22 | `nr_dirty × 4 = Dirty(kB)` 恒成立（实测 `4551 × 4 = 18204`）；但绝对值是**整机口径**，只做增量 |
| 23 | 混用 stdio 与 `write` 的实测结果：**① `[BBBBAAAA]` ② `fflush` → `[AAAABBBB]` ③ `_IONBF` → `[AAAABBBB]` ④ `dup` → `[BBBBAAAA]` ⑤ 外部 `lseek` → `[~100~AAAA]`** |
| 24 | `dup` 出来的 fd **共享文件偏移**，所以它**救不了**混用 |
| 25 | `fclose()` 会**隐式 `fflush`**（标准规定）；危险的是「既不 `fflush` 也不 `fclose`」= `SIGKILL` 时的情形 |
| 26 | `fsync(fp)` **编译不过**（`fp` 是 `FILE*`），必须 `fsync(fileno(fp))`；反复 `fflush` 是空操作 |
| 27 | 本沙箱 **没有 pty**（`/dev/ptmx` 缺失、`posix_openpt()` → `ENOENT`）且 stdout 是 **socket** → 「终端行缓冲」那一半**观察不到** |
| 28 | 原书本章 **4 个**文件全在 `filebuff/`：`direct_read.c`（**Listing 13-1, p.247**）、`mix23_linebuff.c`（**习题 13-4 解答, p.250**）、`mix23io.c`（原书未编号）、`write_bytes.c`（原书未编号） |
| 29 | 13.5 标题两版：早期章节表 `Giving the Kernel Hints about I/O Patterns`，**成书版** `Advising the Kernel About I/O Patterns` |
| 30 | 系统调用次数 = 总字节 / `buf-size`：写 1 MiB，`buf` 1→65536 时次数 **1048576→16**（4.8 个数量级） |
| 31 | `RLIMIT_FSIZE = 16 MiB`（CE）：写 32 MiB 被 **`SIGXFSZ(25)`** 杀掉；**且 `SIGXFSZ` 不会刷 stdio 缓冲** |
| 32 | 「高效 `tail`」= 从尾部按块回扫；实测 `-n 1/100/500/1000` 只读 **4096/12288/45056/82000** 字节（文件 82000） |

---

## 参考

- Kerrisk, *The Linux Programming Interface*, **Chapter 13 — File I/O Buffering**
- [man7 官方源码清单（按章）](https://man7.org/tlpi/code/online/all_files_by_chapter.html) · [OUTLINE](../OUTLINE.md) · [Ch12 系统与进程信息](../chapter-12-system-process-info/README.md) · [Ch14 文件系统](../chapter-14-file-systems/README.md)
- man-pages **6.19**：`read(2)`、`write(2)`、`open(2)`（`O_SYNC`/`O_DSYNC`/`O_DIRECT`）、`fsync(2)`、`fdatasync(2)`、`sync(2)`、`posix_fadvise(2)`、`setvbuf(3)`、`fflush(3)`、`fileno(3)`、`fdopen(3)`、`statx(2)`、`proc(5)`（`/proc/vmstat`、`/proc/meminfo`）、`stat(2)`（`st_blksize`）
- 内核源码（**v6.6**）：`fs/sync.c:180-190`（`vfs_fsync_range`）、`fs/sync.c:218-226`（`fsync`/`fdatasync` 的 syscall 入口）、`fs/iomap/direct-io.c:291-293`（O_DIRECT 的偏移/长度检查）、`include/linux/blkdev.h:1320-1325`（`bdev_dma_alignment` / `bdev_iter_is_aligned`）、`lib/iov_iter.c:866-875`（`iov_iter_is_aligned` 的 `addr_mask` / `len_mask`）、`block/bdev.c:1046-1047`（`stat->dio_mem_align` / `stat->dio_offset_align`）、`mm/page-writeback.c`（脏页阈值与回写）
- glibc **2.39**：`libio/filedoalloc.c`（`_IO_file_doallocate()` —— 缓冲大小与行缓冲的**唯一**判据）、`sysdeps/unix/sysv/linux/posix_fadvise.c`（返回值即错误码）、`sysdeps/unix/sysv/linux/fsync.c` / `fdatasync.c`、`libio/setvbuf.c`、`libio/stdio.c`
- 原书真实 `lib/`（用于校准本仓替身语义）：`lib/tlpi_hdr.h`、`lib/get_num.h`、`lib/get_num.c`、`lib/error_functions.c:49-78`

---

## 代码示例

本章 `code/` 下有：

- **11 个自编 demo**（`c13_*.c` 7 个 + `ex13_*.c` 4 个）
- **4 个原书文件**逐字镜像（`direct_read.c` = Listing 13-1、`mix23_linebuff.c` = 习题 13-4 解答、`mix23io.c`、`write_bytes.c`），`sha256` 与原书一致
- **2 个框架替身**（`tlpi_hdr.h` 只含 `errExit`/`fatal`/`usageErr`/`cmdLineErr`；`get_num.h` 是 `getInt`/`getLong`/`GN_GT_0` 的 `static inline` 版）

全部在 Compiler Explorer（gcc 13.3.0 / x86-64 / Ubuntu 24.04）上真实编译 + 运行过，共 **27 个作业**，全部 `build code = 0` / `didExecute = True` / **`diagnostics = 0`**；其中 **3 个作业的 `exit code = 1` 是应然行为**（`direct_read --help` 与 `write_bytes --help` 走 `usageErr`、`write_bytes` 传 `0` 触发 `getLong(..., GN_GT_0)` 报错）。输出原样抄在对应笔记的实测块里，**共 411 行抽检全部可回溯到冻结日志**。完整索引见 [`code/README.md`](code/README.md)。

| 文件 | 对应节 | 演示什么 | 需要什么 |
|------|--------|----------|---------|
| `c13_1_buffer_cache.c` | §13.1 | 三层数据路径的**下面两层**：① 另开 fd 立刻读得到（证明数据在内核）② 脏页计数器写前/写后/`fsync` 后的三段对照（**增量 4092 页 vs 理论 4096 页**）③ 固定总字节只改 `buf-size`，看 `write()` 次数从 **1048576** 降到 **16** | `/proc/vmstat` 可读 |
| `c13_2_stdio_buffering.c` | §13.2 | stdio 五段：① `isatty`/`st_mode`/`S_ISSOCK` + `BUFSIZ` vs `__fbufsize` ② 三种模式 `setvbuf` 前后 `__flbf`/`__fbufsize` ③ **行缓冲可判定实验**（`_IOLBF` + `write()` 当参照物 + 从管道读回）④ `fileno`/`fdopen` 的桥（`fstat` 看得到 `fflush` 前后的差别）⑤ 两层各管一段的总结 | — |
| `c13_3_sync_calls.c` | §13.3 | 五个刷盘手段：① 三调用 + 四标志（含 `O_RSYNC == O_SYNC`）② `fsync` 在 `/proc`/`/dev/null`/管道上失败一律 `EINVAL` ③ `vfs_fsync_range()` 里 `fsync` 与 `fdatasync` 的**唯一差别** ④ `O_SYNC` 写 1 MiB 的代价对比 ⑤ **`O_SYNC` 管不到 stdio**（`fprintf` 后 `st_size=0`，`fflush` 后 51） | — |
| `c13_4_two_layers.c` | §13.4 | 用「**另一个 fd 能看见多少**」给数据定位：0→4 级阶梯（不存在 / `fprintf` / `fflush` / `fsync` / `fclose`），并诚实标注「16 字节远小于一页，`nr_dirty` 在这个量级看不出变化」；末尾补读方向（stdio 读缓冲会**预读**） | — |
| `c13_5_fadvise.c` | §13.5 | `posix_fadvise` 四段：① 六个 advice 全返回 `0`、非法 advice 返回 **22**、`errno` 全程 0 ② 各类 fd 的失败码（管道 29、`fd=9999` 与已 close 的 9）③ 正确写法 vs `== -1` 的错法 ④ 六个 advice 的语义与两个误解 | — |
| `c13_6_direct_io.c` | §13.6 | `O_DIRECT` 六段：① 先 `statfs` 判文件系统（**ext4 vs tmpfs 必须分开说**）② `statx(STATX_DIOALIGN)` 实测未返回 ③ 偏移/长度/缓冲地址**逐项试错**（`len=100`、`off=1`、1~512 字节对齐）④ `O_DIRECT` 上 `fsync` 仍返回 0 ⑤ `/dev/zero`、`/etc/passwd` 能不能开 ⑥ 工程要点（别写死对齐值） | `/app`（ext4）与 `/tmp`（tmpfs）都可用 |
| `c13_7_mixing.c` | §13.7 | 把「两层打架」**物化到文件里**再 `strcmp`：① 直接混用 `[BBBBAAAA]` ② `fflush` 过渡 `[AAAABBBB]` ③ `_IONBF` `[AAAABBBB]` ④ `dup` 出来的 fd **照样乱序** ⑤ 外部 `lseek` 造出 `[~100~AAAA]`（100 字节 NUL 空洞）。末尾一张「什么时候能混」判定表 | — |
| `ex13_1_copy_bench.c` | 13.9 习题 13-1 | 给 `copy.c` 计时（CE 没有 shell 的 `time`）：`buf-size` 10/512/4096/65536 × `O_SYNC` 开/关，**同时打印与机器无关的「系统调用次数」**。刻意**没跑**「小缓冲 + `O_SYNC`」（书上要 1030 秒） | — |
| `ex13_2_write_bench.c` | 13.9 习题 13-2 | 给 `write_bytes.c` 的语义加自带计时：普通 / `O_SYNC` / 每次 `fsync` / 每次 `fdatasync` × 三种缓冲，并给出**「每次 write 平均耗时」**（把「次数」这个混杂因素除掉）；同步模式规模**故意缩到 256 KiB** | — |
| `ex13_3_fflush_fsync.c` | 13.9 习题 13-3 | 两条语句的作用：① 只 `fsync` 不 `fflush` → **返回 0 而文件 0 字节** ② 只 `fflush` 不 `fsync` → 可见但不持久 ③ 连用 → 三层状态表 ④ 等价写法与两个常见误写（`fsync(fp)`、反复 `fflush`） | — |
| `ex13_5_tail.c` | 13.9 习题 13-5 | 用 `lseek`/`read`/`write` 实现 `tail [-n num] file`：从尾部按块回扫、**自测四种 `-n` 并对账「实际读入字节 / 输出行数 / 输出首行」**、边界用例逐条自验（`-n 0`、`-n > 行数`、**不以 `\n` 结尾的最后一行**）。文件头注释记录了调试中真实踩到的**未初始化堆空洞** bug | — |
| `direct_read.c` | §13.6 | **原书 Listing 13-1（p.247）**：`posix_memalign` 分配对齐缓冲 + `O_DIRECT` 读 → 打到 stdout | `tlpi_hdr.h` + `get_num.h` |
| `mix23io.c` | §13.7 | **原书未编号**：默认缓冲下混用 stdio 与 `write()`，输出顺序是 `write` 先 | — |
| `mix23_linebuff.c` | 13.9 习题 13-4 | **原书习题 13-4 官方解答（p.250）**：显式 `setvbuf(_IOLBF)` 后 `printf` 先出——与 `mix23io.c` **正好相反** | — |
| `write_bytes.c` | 13.9 习题 13-2 | **原书未编号**：写入型基准程序，带 `-DUSE_O_SYNC` / `-DUSE_FSYNC` / `-DUSE_FDATASYNC` 三个编译期开关 | `tlpi_hdr.h` + `get_num.h` |

一次编完全部自编 demo（在 `code/` 目录下）：

```bash
for f in c13_*.c ex13_*.c; do
    gcc -O0 -Wall -Wextra -o "${f%.c}" "$f" || echo "FAIL $f"
done
```

4 个原书程序需要头替身（原书 `main(argc, argv)` 多数不用这两个参数，原书 Makefile 只开 `-Wall`，我们额外加 `-Wextra` 时要带 `-Wno-unused-parameter`）：

```bash
gcc -O0 -Wall -Wextra -Wno-unused-parameter -o direct_read   tlpi_hdr.h get_num.h direct_read.c
gcc -O0 -Wall -Wextra -Wno-unused-parameter -o write_bytes   tlpi_hdr.h get_num.h write_bytes.c
gcc -O0 -Wall -Wextra -Wno-unused-parameter -o mix23io       mix23io.c
gcc -O0 -Wall -Wextra -Wno-unused-parameter -o mix23_linebuff mix23_linebuff.c
```

`write_bytes.c` 的三种同步模式靠**编译期开关**（不是命令行参数）：

```bash
gcc -O0 -Wall -Wextra -Wno-unused-parameter -DUSE_O_SYNC    -o write_bytes_SYNC     tlpi_hdr.h get_num.h write_bytes.c
gcc -O0 -Wall -Wextra -Wno-unused-parameter -DUSE_FSYNC     -o write_bytes_FSYNC    tlpi_hdr.h get_num.h write_bytes.c
gcc -O0 -Wall -Wextra -Wno-unused-parameter -DUSE_FDATASYNC -o write_bytes_FDATASYNC tlpi_hdr.h get_num.h write_bytes.c
```

运行示例：

```bash
./c13_1_buffer_cache       # 页缓存 + 脏页计数器 + 系统调用次数
./c13_2_stdio_buffering    # stdio 三段判据 + 行缓冲可判定实验
./c13_3_sync_calls         # 五个刷盘手段 + EINVAL 的根因
./c13_4_two_layers         # 用「另一个 fd 能看见多少」给数据定位
./c13_5_fadvise            # 返回值即错误码（errno 全程 0）
./c13_6_direct_io          # O_DIRECT 的对齐试错（ext4 vs tmpfs）
./c13_7_mixing             # 混用的五种后果（读到文件里对账）
./ex13_1_copy_bench        # 习题 13-1：copy.c 的 buf-size / O_SYNC 计时
./ex13_2_write_bench       # 习题 13-2：四种同步模式的「每次 write 平均耗时」
./ex13_3_fflush_fsync      # 习题 13-3：fflush + fsync 是一条链的两棒
./ex13_5_tail              # 习题 13-5：自测（四种 -n + 边界用例）
./ex13_5_tail /etc/passwd  # 习题 13-5：命令行用法（默认为尾部 10 行语义）

# 原书程序
./direct_read /etc/passwd 512 0 512   # Listing 13-1（length=512 offset=0 alignment=512）
./direct_read --help                  # 看 usageErr 的文案（exit 1 是应然）
./mix23io                             # 全缓冲：write 先出
./mix23_linebuff                      # 行缓冲：printf 先出（与上一行相反）
./write_bytes /tmp/c13.bin 1048576 4096
./write_bytes_SYNC     /tmp/c13.bin 1048576 4096   # -DUSE_O_SYNC
./write_bytes_FSYNC    /tmp/c13.bin 1048576 4096   # -DUSE_FSYNC
./write_bytes_FDATASYNC /tmp/c13.bin 1048576 4096  # -DUSE_FDATASYNC
```

**几条实测结论**（都是本仓库跑出来的，不是书上抄的）：

- **stdout 在 CE 上是 socket**：`fstat(1)` → `mode=0140777`、`S_ISSOCK=1`、`S_ISFIFO=0`、`S_ISCHR=0`、`isatty=0`、`st_blksize=4096`。于是 stdio **一律全缓冲**——所以「终端 vs 重定向」这类演示在这里**只能观察到全缓冲那一半**
- **`BUFSIZ = 8192` 但 `__fbufsize(stdout) = 4096`**：后者恰好 = `st_blksize`，因为 glibc 取 `min(BUFSIZ, st_blksize)`。同一段代码里行缓冲**只有** `S_ISCHR && isatty` 才设
- **`setvbuf` 三次全返回 0 且模式真的变了**（`__flbf` 0→1→0），但**已经用过的流**上再 `setvbuf(stdout,NULL,_IOFBF,0)` 返回 **0** 而 `__fbufsize` 仍是 **64** ——**返回成功 ≠ 生效**
- **脏页计数器可以当场对账**：写前 `Dirty=1836 kB / nr_dirty=459`，写 16 MiB 后 `Dirty=18204 kB / nr_dirty=4551`（增量 **4092** 页，理论 4096 页），`fsync` 后 `Dirty=1948 kB / nr_dirty=487`；且 `nr_dirty × 4 = Dirty` 严格成立（`4551 × 4 = 18204`）
- **块大小决定系统调用次数**：写 1 MiB，`buf-size` = 1 / 16 / 256 / 4096 / 65536 → `write()` 次数 = **1048576 / 65536 / 4096 / 256 / 16**（耗时 0.3089 / 0.0200 / 0.0015 / 0.0003 / 0.0002 s，属另一台机器）
- **`fsync`/`fdatasync` 的失败一律 `EINVAL(22)`**：`/proc/version`、`/dev/null`、管道读端、管道写端全部 `-1`/`22`；普通文件（`O_RDWR` 与 `O_RDONLY` 两种）全部 `0`
- **`O_SYNC` 的代价**：写 1 MiB（`buf=4096`）普通 `0.0007 s` vs `O_SYNC` **0.7527 s**；`ex13_2` 的「每次 write 平均」更直观：普通模式 **0.63 μs** vs `O_SYNC` **2709.95 μs**
- **`O_DIRECT` 的两套对齐值（实测）**：ext4 上 `len=100` → `EINVAL`、`off=1` → `EINVAL`，但缓冲区地址 1/2/4/8/16/64/256/512 字节对齐**全部成功**；tmpfs 上连 `len=100`、`off=1` 都成功（**假 `O_DIRECT`**）
- **`posix_fadvise` 的五种结果**：六个 advice → `0`；`advice=9999` → **22**；管道 → **29**；`fd=9999` / 已 `close` → **9**；`/proc/version` → `0`。**`errno` 全程 0**
- **混用的五种后果（读到文件里 `strcmp`）**：① `[BBBBAAAA]` ② `[AAAABBBB]` ③ `[AAAABBBB]` ④ `[BBBBAAAA]` ⑤ `[~100~AAAA]`（104 字节，前 100 是 NUL 空洞）
- **`fflush` + `fsync` 是接力**：只 `fsync(fileno(fp))` → 返回 **0** 而另一个 fd 看到的文件大小 **0**；补 `fflush` 后 → **21**
- **`O_SYNC` 管不到 stdio**：`fprintf` 后 `fstat(fd).st_size = 0`，`fflush` 后 = **51**
- **`tail` 的自测对账**：样本 1000 行 / 82000 字节，`-n 1/100/500/1000` → 实际读入 **4096/12288/45056/82000** 字节、输出行数 **1/100/500/1000**、输出首行 **`line-1000/line-0901/line-0501/line-0001`**（正好差 `n-1` 行）
- **`tail` 的边界**：样本 `"a\nb\nc-no-newline"`（16 字节、2 个 `\n`、**3 行**）→ `-n 1` = `[c-no-newline]`、`-n 0` = `[]`、`-n 99` = 全部
- **两份官方程序的输出正好相反**：`mix23io` → `in accordance with his twofold attitude.` 先；`mix23_linebuff` → `I would have written you a shorter letter.` 先
- **文件系统与限制**：`/app` = **ext4（`0xef53`，可用 ≈ 10.95 GB）**、`/tmp` = **tmpfs（`0x1021994`，`f_blocks=5120` ⇒ 只有 20 MiB）**、`/` = tmpfs、`/proc` = `0x9fa0`；`RLIMIT_FSIZE = 16777216`（软硬同值）、`RLIMIT_NOFILE = 100`、`RLIMIT_CORE = 0`；写 32 MiB 被 **`SIGXFSZ(25)`** 杀掉（8/16 MiB 及以下全成功）
- **没有 pty**：`/dev/tty` 与 `/dev/ptmx` 都是 `ENOENT`，`posix_openpt()` 返回 `-1`/`ENOENT`；`readlink("/proc/self/exe")` = `[/app/output.s]`
- **`/etc/passwd` 只有 50 字节、1 条**，`FOPEN_MAX = 16`、`FILENAME_MAX = 4096`、`PIPE_BUF = 4096`；非阻塞 `write(1 MiB)` 到空管道一次写进 **65536**

> ⚠️ **会漂移的数字**（**重跑必变，不要在笔记里硬编码**）：`Dirty` / `nr_dirty` / `Writeback`（整机口径）、所有耗时列（**换台宿主机就变**）、`f_bavail` / `f_blocks`（`/` 与 `/tmp` 随容器变）、`/proc/meminfo` 的 `Dirty`/`MemTotal`、`pid` 与 `fd` 号、`ex13_2` 的「每次 write 平均」末位、`ex13_1` 的耗时分档顺序。
> **相对稳定**：`BUFSIZ = 8192`、`FILENAME_MAX = 4096`、`FOPEN_MAX = 16`、`PIPE_BUF = 4096`、`O_SYNC`/`O_DSYNC`/`O_DIRECT` 的常量值、`O_RSYNC == O_SYNC`、`RLIMIT_FSIZE`/`RLIMIT_NOFILE`、`dirty_*` 四个 sysctl（10/20/3000/500）、文件系统的 `f_type`（`0xef53` / `0x1021994` / `0x9fa0`）、各类 `fsync` 失败码 `EINVAL`、`posix_fadvise` 的 `22`/`29`/`9`、**系统调用次数与「输出首行」两列**（都是算出来的或确定的）。
> ⚠️ CE 容器会被调度到**不同宿主机**（Ch12 就抓到过 `totalram` 从 16.5 GB 变 8.15 GB）；而且 `/` 与 `/tmp` 是 tmpfs，其 `f_blocks` 每次都不一样。**本章所有实测数字均来自同一次冻结运行 `tlpi-ch13-final.txt`。**

---

## 与前后章

| 章 | 关联 |
|----|------|
| [Ch04](../chapter-04-file-io-universal/README.md) / [Ch05](../chapter-05-file-io-further/README.md) | 那两章讲 `read`/`write`/`lseek` **怎么用**，本章讲它们**在缓冲上到底做了什么**；习题 13-1 用的 `copy.c` 就出自 Ch04 的 Listing 4-1 |
| [Ch12](../chapter-12-system-process-info/README.md) | 本章的两把「量尺」都是 `/proc`：`/proc/vmstat` 的 `nr_dirty`、`/proc/meminfo` 的 `Dirty`；`/proc/sys/vm/dirty_*` 决定回写时机。**「返回成功 ≠ 生效」这个模式在两章各出现一次** |
| [Ch02](../chapter-02-basic-concepts/README.md) / [Ch03](../chapter-03-system-programming-concepts/README.md) | 用户态 / 内核态的分界就是本章「两层缓冲」的物理基础；`errno` 范式在 `posix_fadvise` 这里有个**例外**（它不用 `errno`） |
| [Ch14](../chapter-14-file-systems/README.md) | 本章只说「页缓存 + 回写」存在，Ch14 讲它**怎么被管理**（inode / journal / writeback / 文件系统布局） |
| [Ch49](../chapter-49-memory-mappings/README.md) | `mmap` 与 `read`/`write` 是**同一份页缓存**的两种视图；`MAP_SHARED` 的可见性与本章「另一个 fd 能看见」是同一套语义 |
| [Ch63](../chapter-63-alternative-i-o-models/notes/63.4-the-epoll-api.md) | `O_DIRECT`、AIO、`io_uring` 的定位差别——都建在本章「绕过哪一层缓冲」这个分类上 |
