# Ch04 `code/` 目录说明

TLPI 第 4 章（File I/O: The Universal I/O Model）的可编译代码。分三类：

| 类别 | 数量 | 命名 |
|------|------|------|
| 自编 demo | **11** | `c4_<节>_<名字>.c`（8 个）+ `ex4_<n>_<名字>.c`（3 个） |
| 原书镜像 | **2** | `copy.c`（Listing 4-1）、`seek_io.c`（Listing 4-3），**逐字保真** |
| 框架替身 | **1** | `tlpi_hdr.h` |

**全部 15 个作业**（11 自编 + 2 个原书程序 + `copy.c` 的 3 种调用姿势）都在 Compiler Explorer（gcc 13.3.0 / x86-64 / Ubuntu 24.04）上跑过：`build code = 0`、`didExecute = True`、`diagnostics = 0`。

> 为什么走 Compiler Explorer：本机环境里 `wsl.exe` 被安全策略禁用，且没有任何 C 编译器（`gcc`/`clang`/`tcc`/`cl`/`cc`/`zig` 全无）。CE 提供真实的 gcc 13.3 编译诊断与真实运行输出；笔记里凡引用输出都标注「CE 实测」，**不当成本机实测**。

---

## 文件表

### 自编 demo（11）

| 文件 | 对应节 | 演示什么 | 需要什么 |
|------|--------|----------|---------|
| `c4_1_fd_basics.c` | 4.1 | fd 三个常量；「最小可用号」分配（`3`→`4`）；`close` 后号**立即回收复用**；`/proc/self/fd` 逐个列出 fd；`close(0)` 后新 `open` 竟拿到 **0 号** | — |
| `c4_2_universal.c` | 4.2 | **同一套 `open/read/write/close`** 打四种对象：普通文件 / `/dev/null` / `/proc/version` / 匿名管道。每个对象回答「读多少、写多少、能不能 seek」 | — |
| `c4_3_open.c` | 4.3 | 四组 `umask` 对 `0666`/`0777` 的实测影响（`0000`/`0022`/`0077`/`0027`）；`O_CREAT\|O_EXCL` 的原子创建（第二次 `EEXIST`）；`O_TRUNC` 只打开就清空；`FD_CLOEXEC` 与 `O_CLOEXEC` **分属两层**；`F_GETFL` 比入参多出内核补的 `O_LARGEFILE` | — |
| `c4_4_read.c` | 4.4 | 循环读到 EOF；**真短读**（只剩 6 字节却请求 8）；`EBADF` 的两种成因（只写 fd / `fd=999`）；`EFAULT`（坏地址）；管道短读（3 字节对象读 64）；**`read(fd, buf[64], 1GiB)` 竟然成功返回** | — |
| `c4_5_write.c` | 4.5 | 一次写满；`write(fd, buf, 0)` 返回 `0` 且 `errno` 干净（**合法**）；`/dev/full` 尝试（沙箱无此设备 → 自动降级跳过）；`SIGPIPE` 杀进程 vs `signal(SIGPIPE, SIG_IGN)` 后拿到 `EPIPE`；`fsync`/`fdatasync` 落盘 | —（会 `fork`） |
| `c4_6_close.c` | 4.6 | `close` 后号立即回收；`close(999)` 与**重复 `close(3)`** 都是 `EBADF`（`close` **不幂等**）；`dup` 两 fd **共享偏移**；`fork` 后父子**共享偏移** | —（会 `fork`） |
| `c4_7_lseek.c` | 4.7 | 三种 `whence` 基准；`lseek` 只动游标不动数据；越尾写造**文件空洞**（`st_size` vs `st_blocks`）；空洞读出来是 `0`；`SEEK_HOLE`/`SEEK_DATA` 按**文件系统块**对齐；管道上 `ESPIPE` | `_GNU_SOURCE`（`SEEK_HOLE`/`SEEK_DATA`） |
| `c4_8_ioctl.c` | 4.8 | 把 `TIOCGWINSZ` 按位拆成 `dir`/`size`/`type`/`nr`，**证明它没按 `_IOR` 编码**（`dir=0`、`size=0`）；`_IO('T',0x13)` 与真值相等、`_IOR('T',104,…)` 对不上；`isatty()` 的真相；`FIONREAD`（9→5）；`FIONBIO` 切非阻塞（`EAGAIN`）；普通文件 `ENOTTY` | — |
| `ex4_1_tee.c` | 4.10 练习 4-1 | 原书 Exercise 4-1：`getopt` 解析 `-a`；一份 stdin 同时写文件与 stdout；**两处都用 `writeAll` 循环**（stdout 被重定向时也会部分写） | — |
| `ex4_2_cpholes.c` | 4.10 练习 4-2 | 原书 Exercise 4-2：用 `SEEK_DATA`/`SEEK_HOLE` 逐段拷贝，**保留源文件的洞**；无参数时自造「3 段数据 + 3 处洞（含尾部洞）」的源文件；`ftruncate` 补齐尾部洞的逻辑长度 | `_GNU_SOURCE` |
| `ex4_3_partial_write.c` | 4.10 练习 4-3 | 自编：`fcntl(F_SETPIPE_SZ, 4096)` 把管道容量压到 4 KiB，再用非阻塞 `write(8192)` **制造一次真实的部分写**（返回 4096）；`writeAll` 循环版补满 8192 | — |

### 原书镜像（2，逐字保真）

| 文件 | Listing | 页码 | 说明 | 需要什么 |
|------|---------|------|------|---------|
| `copy.c` | **4-1** | p.71 | 通用拷贝：`BUF_SIZE 1024`；`usageErr`/`errExit`/`fatal` 三个助手；`errExit("opening file %s", argv[1])` 是**变参**用法；写不满就 `fatal` | `tlpi_hdr.h` |
| `seek_io.c` | **4-3** | p.84 | 交互式 `lseek` 演示器：`seek_io file {r<len>\|R<len>\|w<string>\|s<offset>}...`，一条命令串起读/写/定位；`r` 文本模式把不可打印字符打成 `?`，`R` 用 hex | `tlpi_hdr.h`（`getLong` / `cmdLineErr`） |

> 页码出处：man7 单文件页原文（`This is fileio/copy.c (Listing 4-1, page 71), an example from the book, The Linux Programming Interface.` / `... (Listing 4-3, page 84)`）。

> 镜像来源：`https://man7.org/tlpi/code/online/dist/fileio/<file>`（返回**原始源文件**，含 GPL 头与原作者注释）。
> 文件清单出处：[man7 — List of source code files, by chapter](https://man7.org/tlpi/code/online/all_files_by_chapter.html) 的 **Chapter 4** 一节——它只列了这两个文件。
> 之所以取 `.c` 原始文件而不是网页 HTML 版：man7 的 HTML 渲染把源码切成多个 `<pre>` 块，会丢空行与缩进——**保真度不够**。

⚠️ `seek_io.c` 的 `open` 用的是 `O_RDWR | O_CREAT`，**没有 `O_TRUNC`**（`seek_io.c:54-56`）。对**同一个文件**重复运行，新内容会盖在旧内容上——本地反复跑请换文件名。

### 框架替身（1）

| 文件 | 说明 |
|------|------|
| `tlpi_hdr.h` | 原书 `lib/tlpi_hdr.h` 的**最小可用子集**，但按原书语义实现了 5 个助手：`errExit` / `fatal` / `usageErr` / `cmdLineErr`（`error_functions.c`）+ `getLong`（`get_num.c`，含 `GN_*` 旗标）。原书版依赖整个 `lib/` 目录，无法单文件编译 |

> ⚠️ 这份 `tlpi_hdr.h` 与 Ch10 的同名替身**不能互换**：Ch10 那版 `errExit` 只吃单个字符串，喂 `copy.c`（`errExit("opening file %s", argv[1])`）会编译失败。

---

## 编译

一次编完 11 个自编 demo（在 `code/` 目录下）：

```bash
for f in c4_*.c ex4_*.c; do
    gcc -O0 -Wall -Wextra -o "${f%.c}" "$f" || echo "FAIL $f"
done
```

两个原书程序都需要 `tlpi_hdr.h`（**同目录即可**，在 `code/` 里直接编）：

```bash
gcc -O0 -Wall -Wextra -o copy    copy.c
gcc -O0 -Wall -Wextra -o seek_io seek_io.c
```

## 运行示例

```bash
./c4_1_fd_basics
./c4_3_open
./c4_7_lseek
./c4_5_write                           # 想看 SIGPIPE/EPIPE 分支；不加参数也能跑

./copy /proc/version /tmp/out.txt      # 原书 Listing 4-1
./copy --help                          # 参数不足 → usageErr

./seek_io /tmp/seek.txt wABCDEFGHIJ s4 r3 s0 R10 s20 wXYZ s20 r3   # 原书 Listing 4-3

printf 'hello tee\nsecond line\n' | ./ex4_1_tee /tmp/tee_out.txt
./ex4_2_cpholes                        # 无参数：自造带洞源文件再拷
./ex4_3_partial_write
```

## 沙箱环境注意（这些「失败」是环境限制，不是代码 bug）

| 现象 | 原因 |
|------|------|
| `ioctl(stdout, TIOCGWINSZ)` 返回 `ENOTTY` | stdout 被重定向/接管，不是 tty。**本地终端里会成功** |
| `isatty()` 三个都是 `0` | 同上，沙箱里没有任何 fd 是终端 |
| `/dev/full` 打不开 | CE 沙箱没有这个设备；`c4_5_write.c` 检测到就**降级跳过**，不算失败 |
| `/dev/stdout` 不能作为 `copy.c` 的目标 | 沙箱里它是只读挂载（`Read-only file system`） |
| `/etc/os-release` 不存在 | 沙箱精简掉了。`copy_ok` 改用 `/proc/version` 当源文件 |
| `SEEK_DATA`/`SEEK_HOLE` 行为随文件系统变 | 通用兜底 `default_llseek` 假设「洞 = 文件末尾」；真实洞检测由具体 FS 提供 |
