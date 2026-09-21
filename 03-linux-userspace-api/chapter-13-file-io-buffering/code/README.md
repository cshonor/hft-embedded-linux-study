# Ch13 `code/` 目录说明

TLPI 第 13 章（File I/O Buffering）的可编译代码。分三类：

| 类别 | 数量 | 命名 |
|------|------|------|
| 自编 demo | **11** | `c13_<节>_<名字>.c`（7 个）+ `ex13_<n>_<名字>.c`（4 个） |
| 原书镜像 | **4** | `direct_read.c`(Listing 13-1, p.247)、`mix23_linebuff.c`(习题 13-4 解答, p.250)、`mix23io.c`(原书未编号)、`write_bytes.c`(原书未编号)，**逐字保真**（`sha256` 与原书一致） |
| 框架替身 | **2** | `tlpi_hdr.h`（含 `errExit` / `fatal` / `usageErr` / `cmdLineErr`）+ `get_num.h`（`getInt` / `getLong` / `GN_GT_0`） |

**全部 27 个作业**都在 Compiler Explorer（gcc 13.3.0 / x86-64 / Ubuntu 24.04）上跑过：`build code = 0`、`didExecute = True`、**`diagnostics = 0`**。其中 **3 个的 `exit code = 1` 是应然行为**：`direct_read --help` 与 `write_bytes --help` 走 `usageErr`（原书就是 `exit(EXIT_FAILURE)`）、`write_bytes ... 0 4096` 触发 `getLong("num-bytes", ..., GN_GT_0, ...)` 的报错路径。作业清单与其编译旗标由 run_ch13.py 驱动（本地脚本）。

> 为什么走 Compiler Explorer：本机环境里 `wsl.exe` 被安全策略禁用，且没有任何 C 编译器（`gcc`/`clang`/`tcc`/`cl`/`cc`/`zig` 全无）。CE 提供真实的 gcc 13.3 编译诊断与真实运行输出；笔记里凡引用输出都标注「CE 实测」，**不当成本机实测**。
>
> ⚠️ CE 是**一次性容器**，而且是个**功能残缺的容器**。本章要特别记住三件事：**① 没有 pty**（`/dev/tty` 与 `/dev/ptmx` 都 `ENOENT`，`posix_openpt()` → `ENOENT`），而 stdout 是 **socket**（`S_ISSOCK=1`、`isatty=0`）⇒ stdio 一律**全缓冲**，所以「终端 → 行缓冲」那一半现象**无法直接观察**；**② `RLIMIT_FSIZE = 16 MiB`**（软硬同值）⇒ 写超了会被 **`SIGXFSZ(25)`** 杀掉，而且 **`SIGXFSZ` 不会刷 stdio 缓冲**；**③ `/tmp` 是 tmpfs 且只有 20 MiB**（`f_blocks=5120`）⇒ 在 `/tmp` 上测 `O_SYNC` 的代价、或者测 `O_DIRECT` 的对齐，得到的都是**失真结果**。

---

## 文件表

### 自编 demo（11）

| 文件 | 对应节 | 演示什么 | 需要什么 |
|------|--------|----------|---------|
| `c13_1_buffer_cache.c` | §13.1 | 三层路径的下面两层：**①** `write(fd,"hello-page-cache",16)` 之后另开 fd 立刻读得到 16 字节 → 证明数据在**内核页缓存**里；**②** 脏页计数器三段对照（写前 `Dirty=1836 kB/nr_dirty=459` → 写 16 MiB 后 `18204/4551` → `fsync` 后 `1948/487`），并核对 `nr_dirty × 4 = Dirty`、`nr_dirty` 增量 **4092** ≈ 16 MiB/4096 = **4096** 页；**③** 固定写 1 MiB 只改 `buf-size`（1/16/256/4096/65536），`write()` 次数 **1048576→16**；**④** 结论段 | `/proc/vmstat` 可读 |
| `c13_2_stdio_buffering.c` | §13.2 | stdio 五段：**①** `isatty` / `st_mode` / `S_ISSOCK` / `BUFSIZ=8192` vs `__fbufsize=4096`（引 glibc `_IO_file_doallocate()` 的 `min(BUFSIZ, st_blksize)` 与「只有 `S_ISCHR && isatty` 才 `_IO_LINE_BUF`」）；**②** `_IOFBF`/`_IOLBF`/`_IONBF` 三种 `setvbuf` 前后 `__flbf`/`__fbufsize` 的变化；**③** **行缓冲可判定实验**——先 `setvbuf(_IOLBF)`，再用 `write()` 当「参照物」，把 stdout 重定向到管道后读回来看 `'\n'` 到底有没有触发刷新；**④** `fileno`/`fdopen` 的桥（`fstat` 看 `fflush` 前后 `st_size` 0→18）；**⑤** 两层各管一段 | — |
| `c13_3_sync_calls.c` | §13.3 | 五个刷盘手段：**①** `sync`/`fsync`/`fdatasync` 的语义 + 四个打开标志的值（含 **`O_RSYNC == O_SYNC`**，SUSv3 要求的「读也同步」Linux 没实现）；**②** `fsync`/`fdatasync` 在普通文件（`O_RDWR` 与 `O_RDONLY` 各一）上返回 0、在 `/proc/version`/`/dev/null`/管道读端/管道写端上 `-1`/`EINVAL(22)`（**不是 `ESPIPE`**）——并引 `fs/sync.c:180-190` 的 `if (!file->f_op->fsync) return -EINVAL;`；**③** `fsync` 与 `fdatasync` 的唯一内核差别（`I_DIRTY_TIME` 那三行）；**④** 同规模下 `O_WRONLY` vs `O_WRONLY\|O_SYNC` 的耗时对比（0.0007 s vs 0.7527 s，写 1 MiB / `buf=4096`）；**⑤** **`O_SYNC` 管不到 stdio**（`fprintf` 后 `st_size=0`，`fflush` 后 51） | — |
| `c13_4_two_layers.c` | §13.4 | 用「**另一个 fd 能看见多少**」给数据**定位**：0→4 级阶梯（文件不存在 / `fprintf` 后 / `fflush` 后 / `fsync` 后 / `fclose` 后），每级打「别处可见字节数」「文件大小」「内容」；再做「两条链路的完整写法」（写：`fprintf`→`fflush`→`fsync`；读：stdio 读缓冲会**预读**，所以「读了多少」两层不同）。**诚实标注**：16 字节远小于一页，`nr_dirty` 全程 14（看不出变化）——**小 IO 下它不是有效指标** | — |
| `c13_5_fadvise.c` | §13.5 | `posix_fadvise` 四段：**①** 六个 advice 逐个调用（全返回 `0`）、非法 `advice=9999` 返回 **22**，**`errno` 全程 0**；**②** 各类 fd 的失败码（管道 29、`fd=9999` 与已 `close` 的 fd 都是 9、`/proc/version` 返回 0）；**③** 正确写法 vs `if (posix_fadvise(...) == -1) perror(...)` 这种**永远不成立**的错法（引 glibc `posix_fadvise.c` 的 `INTERNAL_SYSCALL_ERRNO` 分支）；**④** 六个 advice 的语义表 + 两个常见误解（只是建议；`DONTNEED` 不是 `fsync` 替代品） | — |
| `c13_6_direct_io.c` | §13.6 | `O_DIRECT` 六段：**①** 先 `statfs` 判文件系统（`/app` = `ext4(0xef53)`、`/tmp` = `tmpfs(0x1021994)`，**两个结论必须分开说**）；**②** 用 `statx(STATX_DIOALIGN)` 问内核对齐要求——实测**没返回**（`stx_mask=0x173f`）；**③** 偏移/长度/缓冲区地址**逐项试错**：ext4 上 `len=100`/`off=1` → `EINVAL`，而缓冲地址 **1/2/4/8/16/64/256/512 字节对齐全部成功**；tmpfs 上连 `len=100`/`off=1` 都成功；**④** `O_DIRECT` 的 fd 上 `fsync` 仍返回 0（绕过缓存 ≠ 落盘）；**⑤** `open("/dev/zero", O_RDONLY\|O_DIRECT)` → `EINVAL`、`/etc/passwd` 能开但是**假 DIO**；**⑥** 工程要点（别写死对齐值、`posix_memalign`、必须在目标文件系统上验） | `/app`(ext4) 与 `/tmp`(tmpfs) 都可用 |
| `c13_7_mixing.c` | §13.7 | 把「两层打架」**物化到真实文件**里再 `strcmp`（不靠肉眼看 stdout 顺序，因为 CE 上 stdout 是 socket）：**①** `fprintf` 后直接 `write` → `[BBBBAAAA]`；**②** 混用前 `fflush` → `[AAAABBBB]`；**③** `setvbuf(_IONBF)` → `[AAAABBBB]`；**④** 用 `dup` 出来的 fd 去 `write` → **照样** `[BBBBAAAA]`（共享文件偏移，避开的是 fd 不是缓冲）；**⑤** 用另一个 fd `lseek(fd,100,SEEK_SET)` 再 `fprintf` → `[~100~AAAA]`（104 字节，前 100 是 NUL 空洞）。末尾附「什么时候能混」判定表 | — |
| `ex13_1_copy_bench.c` | 13.9 习题 13-1 | 给 Listing 4-1 的 `copy.c` 计时。CE 里**没有 shell 的 `time` 内建**，所以程序**自己**用 `clock_gettime(CLOCK_MONOTONIC)` 计时，并额外打印**与机器无关的「系统调用次数」**（`read+write` 之和）。六档：`buf-size` 10/512/4096/65536 × `O_SYNC` 关/开。**刻意没跑「小缓冲 + `O_SYNC`」**——原书 Table 13-3 那一档要 1030 秒 | — |
| `ex13_2_write_bench.c` | 13.9 习题 13-2 | 给 `write_bytes.c` 加自带计时：普通 / `O_SYNC` / 循环内每次 `fsync` / 每次 `fdatasync` × 三种缓冲（512/4096/65536），比原书多给一列**「每次 write 平均耗时」**——它把「调用次数」这个混杂因素除掉，才真正反映**单次同步**的代价（普通 0.63 μs vs `O_SYNC` 2709.95 μs）。同步模式规模**故意缩到 256 KiB**（文件头注释里写明理由） | — |
| `ex13_3_fflush_fsync.c` | 13.9 习题 13-3 | 两条语句的作用：**①** 只调 `fsync(fileno(fp))` 不先 `fflush` → 返回 **0**（成功）而另一个 fd 看到的文件大小仍是 **0**；**②** 只 `fflush` 不 `fsync` → 可见但不持久；**③** 两条连用的三层状态表（stdio 缓冲 / 页缓存 / 磁盘）；**④** 等价写法表 + 两个常见误写（`fsync(fp)` 编译不过、反复 `fflush` 是空操作） | — |
| `ex13_5_tail.c` | 13.9 习题 13-5 | 用 `lseek`/`read`/`write` 实现 `tail [-n num] file`：从**尾部按块回扫**（不是从头读），攒够 `n+1` 个换行就停。**自测**四种 `-n`（1/100/500/1000）并对账三列：**实际读入字节 / 输出行数 / 输出首行**；边界用例逐条自验（`-n 0`、`-n > 行数`、**不以 `\n` 结尾的最后一行**）。文件头注释记录了调试中真实踩到的**未初始化堆空洞** bug（`memcpy(nb+BLK, ...)` 应为 `nb+want`）——它只改顺序、不改行数，所以**只有「输出首行」这一列能抓住** | — |

### 原书镜像（4，逐字保真）

| 文件 | 原书位置 | 说明 | 需要什么 |
|------|---------|------|---------|
| `direct_read.c` | **Listing 13-1, p.247** | 用 `O_DIRECT` 读文件：`getLong` 取 `length`/`offset`/`alignment` → `posix_memalign` 分配对齐缓冲 → `open(O_RDONLY\|O_DIRECT)` → `read` → `write(STDOUT_FILENO)`. 它是「用对齐缓冲 + 指定偏移」这条工程规矩的模板 | `tlpi_hdr.h` + `get_num.h` |
| `mix23_linebuff.c` | **Solution to Exercise 13-4, p.250** | 先把 `stdout` 显式设成**行缓冲**，然后 `printf("If I had more time, \n")` + `write(STDOUT_FILENO, "I would have written you a shorter letter.\n", 43)`。因为行缓冲下 `'\n'` 触发刷新，所以 **`printf` 先出** | — |
| `mix23io.c` | **原书未编号**（声明 "This file is not printed in the book"） | 同一段混用逻辑，但**不设**缓冲模式 → stdout 是管道/socket 时**全缓冲** → **`write` 先出**。与上一个文件**首行正好相反** | — |
| `write_bytes.c` | **原书未编号**（声明是 "a supplementary file for Chapter 13"） | 写入型基准程序：`write(fd, buf, bufSize)` 循环 `numBytes/bufSize` 次，带 `-DUSE_O_SYNC` / `-DUSE_FSYNC` / `-DUSE_FDATASYNC` 三个编译期开关（分别是 `open` 加 `O_SYNC`、每次 `write` 后 `fsync`、每次 `write` 后 `fdatasync`） | `tlpi_hdr.h` + `get_num.h` |

> 页码出处：man7 单文件页原文（`This is filebuff/direct_read.c (Listing 13-1, page 247), an example from the book, The Linux Programming Interface.`）。
> 镜像来源：`https://man7.org/tlpi/code/online/dist/filebuff/<file>`。
> 文件清单出处：[man7 — List of source code files, by chapter](https://man7.org/tlpi/code/online/all_files_by_chapter.html) 的 **Chapter 13** 一节——共 **4 个**文件，**全部**在 `filebuff/` 下。
> 逐字校验：四个文件的 `sha256`（`57b45c9a82ed0d5f…` / `4b116618ee5fe922…` / `fe1c00381448d883…` / `b8aa772328ba56c2…`）与镜像下载件一致。

⚠️ **本章原书只有 4 个示例文件**（其中 2 个连编号都没有），但**实测数据的密度是全模块最高的章之一**——真正的知识量在「两层的可见性如何被一步步证明」。

⚠️ **习题原文未逐字引用（诚实标注）**：`ex13_1_*` / `ex13_2_*` / `ex13_3_*` / `ex13_5_*` 的头注释里只写【任务】，不引原书题干——因为 man7 只分发源码、不放习题正文，**本节这些题干是转引，不是官方分发原文**。**例外是 13-4**：它的官方解答程序 `mix23_linebuff.c` 是公开的，所以 13.4 的题面由该程序**反推**得出，推理依据写在 13.9 的对应小节里。

### 框架替身（2）

| 文件 | 说明 |
|------|------|
| `tlpi_hdr.h` | 原书 `lib/tlpi_hdr.h` 的**最小可用子集**，按原书语义实现 4 个助手：`errExit`（`fmt: strerror(errno)` + `exit(1)`）/ `fatal`（带 `"ERROR: "` 前缀）/ `usageErr`（打 `Usage: ` 前缀 + `exit(1)`）/ `cmdLineErr`。原书结构是「`tlpi_hdr.h` 声明 + `error_functions.c` 实现」，为单文件可控，这里把实现做成 `static inline` 放在头里 |
| `get_num.h` | 原书 `lib/get_num.h` + `lib/get_num.c` 的 `static inline` 版：`getInt`/`getLong` 支持 `GN_GT_0` 之类的下限检查，失败时打 `"<name> error (in <arg>): <msg>"` + `"offending text: <str>"` 再 `exit(EXIT_FAILURE)`。**本章两个原书程序真的会走这条路径**（`write_bytes ... 0 4096` 那条作业就是去验证它） |

> ⚠️ **本章替身与 `lib/error_functions.c` 的差异（3 条，必须知道）**：
> 1. **报文格式退化**：原书 `outputError()`（`lib/error_functions.c:49-78`）打的是 `"ERROR%s %s\n"`，其中 `errText` 为 `" [%s %s]"`（= `ename[err]` + `strerror(err)`），也就是**同时给人话和错误名**；本替身只打 `strerror` 文本。
> 2. **只提供 4 个入口**：原书有 7 个（`errMsg` / `errExit` / `err_exit` / `errExitEN` / `fatal` / `usageErr` / `cmdLineErr`），本章只用到 `errExit` / `fatal` / `usageErr` / `cmdLineErr`。
> 3. **`usageErr` / `cmdLineErr` 的文案逐字保留**，因为本章的 `direct_read.c` / `write_bytes.c` **会真的触发它们**（两条 `--help` 作业就是去验证 `usageErr` 的文案与 exit code）。
>
> ⚠️ `get_num.h` 里 `getLong` 的两条报错行**缩进逐字对齐了原书**（`write_bytes_bad` 作业的输出里那两行错位正是原书的样子）——不要「顺手修正」它，否则就不再是逐字镜像。
>
> ⚠️ 这份 `tlpi_hdr.h` 与 Ch04 / Ch05 / Ch10 / Ch11 / Ch12 的同名替身**不能互换**（每章只保留自己用到的最小集）。别跨章复制粘贴——这是本仓库故意保持的「每章最小化」策略。

---

## 编译

一次编完全部自编 demo（在 `code/` 目录下）：

```bash
for f in c13_*.c ex13_*.c; do
    gcc -O0 -Wall -Wextra -o "${f%.c}" "$f" || echo "FAIL $f"
done
```

11 个自编 demo **不需要任何额外旗标**（`_GNU_SOURCE` 没用到；`sys/statfs.h`、`sys/statvfs.h`、`sys/random.h` 之类的头都显式包含）。两个 `ex13_*` benchmark 会创建 `/app/ex13_1_*.bin`、`/app/ex13_2.bin`，`ex13_5_tail` 会在 `/app` 下建样本文件——**这些路径是配合 CE 的 ext4 挂载点选的**，换环境请改宏 `OUT`/`SRC`。

4 个原书程序都需要头替身（**同目录即可**）：

```bash
gcc -O0 -Wall -Wextra -Wno-unused-parameter -o direct_read   tlpi_hdr.h get_num.h direct_read.c
gcc -O0 -Wall -Wextra -Wno-unused-parameter -o write_bytes   tlpi_hdr.h get_num.h write_bytes.c
gcc -O0 -Wall -Wextra -Wno-unused-parameter -o mix23io       mix23io.c
gcc -O0 -Wall -Wextra -Wno-unused-parameter -o mix23_linebuff mix23_linebuff.c

# write_bytes.c 的三种同步模式 = 三个编译期开关（不是命令行参数）
gcc -O0 -Wall -Wextra -Wno-unused-parameter -DUSE_O_SYNC    -o write_bytes_SYNC     tlpi_hdr.h get_num.h write_bytes.c
gcc -O0 -Wall -Wextra -Wno-unused-parameter -DUSE_FSYNC     -o write_bytes_FSYNC    tlpi_hdr.h get_num.h write_bytes.c
gcc -O0 -Wall -Wextra -Wno-unused-parameter -DUSE_FDATASYNC -o write_bytes_FDATASYNC tlpi_hdr.h get_num.h write_bytes.c
```

> `-Wno-unused-parameter` 是因为这几份原书代码的 `main(int argc, char *argv[])` **多数不用这两个参数**。原书 Makefile 只开 `-Wall`，我们额外加的 `-Wextra` 会把它报成 `warning: unused parameter`。**原书代码一字不改**，改旗标。

## 运行示例

```bash
./c13_1_buffer_cache       # 页缓存 + 脏页计数器 + 系统调用次数
./c13_2_stdio_buffering    # stdio 三段判据 + 行缓冲可判定实验
./c13_3_sync_calls         # 五个刷盘手段 + 为什么失败是 EINVAL
./c13_4_two_layers         # 用「另一个 fd 能看见多少」给数据定位
./c13_5_fadvise            # 返回值即错误码（errno 全程 0）
./c13_6_direct_io          # O_DIRECT 的对齐试错（ext4 vs tmpfs）
./c13_7_mixing             # 混用的五种后果（物化到文件里对账）
./ex13_1_copy_bench        # 习题 13-1
./ex13_2_write_bench       # 习题 13-2
./ex13_3_fflush_fsync      # 习题 13-3
./ex13_5_tail              # 习题 13-5 自测
./ex13_5_tail -n 5 /etc/passwd   # 习题 13-5 命令行用法

# 原书程序
./direct_read /etc/passwd 512 0 512   # Listing 13-1（length offset alignment）
./direct_read --help                  # usageErr 文案（exit 1 是应然）
./mix23io                             # 全缓冲：write 先出
./mix23_linebuff                      # 行缓冲：printf 先出（与上一行相反）
./write_bytes /tmp/c13.bin 1048576 4096
./write_bytes /tmp/c13.bin 0 4096     # 触发 getLong(GN_GT_0) 报错（exit 1 是应然）
./write_bytes_SYNC     /tmp/c13.bin 1048576 4096
./write_bytes_FSYNC    /tmp/c13.bin 1048576 4096
./write_bytes_FDATASYNC /tmp/c13.bin 1048576 4096
```

## 沙箱环境注意（这些「失败」是环境限制，不是代码 bug）

| 现象 | 原因 |
|------|------|
| **没有 pty**：`/dev/tty` 与 `/dev/ptmx` 都 `ENOENT`，`posix_openpt()` → `-1`/`ENOENT` | CE 容器不开 pty。所以「把子进程 stdout 接到 pty 看行缓冲」这条路走不通（`probe13b` 试过 `fork+execl` 接 pty，接不到）。**替代办法**：13.2 的 ③ 段显式 `setvbuf(_IOLBF)` + 用 `write()` 当参照物 + 事后从管道读回——机制照样可判定 |
| **stdout 是 socket**：`mode=0140777`、`S_ISSOCK=1`、`isatty=0`、`st_blksize=4096` | 所以 stdio **一律全缓冲**，「终端行缓冲 → `printf` 先出」那一半**观察不到**；`mix23io.c` 与 `mix23_linebuff.c` 在 CE 上只能分别演示「全缓冲」与「显式行缓冲」两种**配置**下的差异 |
| `BUFSIZ = 8192` 而 `__fbufsize(stdout) = 4096` | **正常**。glibc `_IO_file_doallocate()` 取 `min(BUFSIZ, st_blksize)`，而 socket/文件的 `st_blksize = 4096` |
| `setvbuf(stdout,NULL,_IOFBF,0)` 返回 0 但 `__fbufsize` 没变 | **流已经用过**。glibc 允许改但（在已分配缓冲的情况下）实际没换。**返回 0 ≠ 生效**——规范用法是紧跟 `fopen`/`fdopen` 之后调用 |
| **`RLIMIT_FSIZE = 16777216`（16 MiB，软硬同值）** | 写 32 MiB 会被 **`SIGXFSZ(25)`** 杀掉（实测 exit 153）。所以本目录所有程序都把规模压在 16 MiB 及以下（`ex13_2` 的同步模式甚至缩到 256 KiB）。⚠️ **`SIGXFSZ` 不会刷 stdio 缓冲**，所以「超大写入 + 靠 exit 刷缓冲」会连已写数据一起丢 |
| **`/tmp` 是 tmpfs 且只有 20 MiB**：`f_blocks=5120` ⇒ `f_bavail×f_frsize = 20971520` 字节 | 所以 `write_bytes` 的默认输出选在 `/app`（**ext4**，`f_bavail×f_frsize = 10950397952` 字节 ≈ 10.95 GB）。**tmpfs 上没有真实设备**：`O_SYNC`/`fsync` 的耗时在这里**严重失真**，`O_DIRECT` 是**假的**（实测不做对齐检查） |
| **`O_DIRECT` 在 ext4 上 `len=100`/`off=1` → `EINVAL(22)`，在 tmpfs 上全成功** | 这不是 bug：tmpfs 根本不实现 `O_DIRECT` 的对齐检查。**同一段代码的结论必须绑定文件系统**——这正是 `c13_6` 第 ① 段先 `statfs` 判类型的原因 |
| **缓冲区地址 1 字节对齐也能 `O_DIRECT` 成功** | 与「必须 4096 对齐」的流传说法不符，但内核就是这么写的：偏移/长度用 `bdev_logical_block_size`，缓冲地址用 `bdev_dma_alignment + 1`，后者在本机很小。**别写死对齐值**，用 `statx(STATX_DIOALIGN)` 问或保守给 4096 |
| `statx(STATX_DIOALIGN)` 成功但 `stx_mask` 不含该位（`0x173f`） | 该文件系统**不上报**这个字段。**字段缺失 ≠ 要求为 0**——回退到保守值 + 试错 |
| `open("/dev/zero", O_RDONLY\|O_DIRECT)` → `EINVAL` | 字符设备没有块设备的 direct I/O 能力。`/proc/version` 同样 `EINVAL` |
| `open("/etc/passwd", O_RDONLY\|O_DIRECT)` **成功** | 因为 `/etc/passwd` 在本容器里位于 **tmpfs** 上——能打开但那个 `O_DIRECT` 是**假的**。换到 ext4 上才是真 `O_DIRECT`（且有对齐要求） |
| `fsync`/`fdatasync` 在 `/proc/version`、`/dev/null`、管道上全是 `EINVAL(22)` | 不是 `ESPIPE`。这些 `file_operations` **没有 `.fsync` 回调**，在 VFS 层（`fs/sync.c:180-190`）就被判 `EINVAL` |
| **只读 fd 上 `fsync()` 返回 0** | `do_fsync()` 只做 `fdget()` + `vfs_fsync()`，**不看打开模式**。所以「只读文件不能 `fsync`」是误解 |
| `posix_fadvise` 的 `errno` 永远是 0 | glibc `sysdeps/unix/sysv/linux/posix_fadvise.c` **直接 `return` 错误码**，不设 `errno`。`perror()` 在它后面什么都打不出来（跨章对照：Ch12 的 `/proc/sys` 写入、本章的 `setvbuf`——**「返回成功 ≠ 生效」在本章出现两次**） |
| `nr_dirty` 只写 16 字节时**看不出变化**（全程 14） | 16 字节远小于一页；而且这个计数器是**整机（宿主）口径**。**小 IO 下它不是有效指标**——改用「另一个 fd 的 `st_size` + 可见性」判断。13.1 那种 16 MiB 的量级才能用它（增量 4092 页 vs 理论 4096） |
| `/etc/passwd` 只有 **50 字节、1 条**（`ce:x:10240:10240:…:/app:/bin/bash`） | 所以 `ex13_5_tail -n 5 /etc/passwd` 会走「行数不够就是全部」这条路（并往 stderr 打一行「实际只读了 50 字节」）——**这是应然行为，不是错误** |
| `/proc/meminfo` 的 `Dirty` / `MemTotal`、`/` 与 `/tmp` 的 `f_blocks` 每次都不同 | `/` 与 `/tmp` 是 tmpfs（随容器伸缩），`Dirty` 是整机口径。**别把某一台机的数字当常量**——本章所有引用数字冻结在同一次运行 `tlpi-ch13-final.txt` |
| 同一份代码在两次 CE 运行里耗时差几倍 | **CE 会把容器调度到不同宿主机**。所以本章所有 benchmark 都遵循「**先给与机器无关的定量指标（系统调用次数 / 字节数），再给耗时作佐证**」，并在输出里明说耗时属于「另一台宿主机」 |
