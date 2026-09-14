# 5.1 strace 入门（基本用法 / 输出格式 / 参数与 errno 解读）

> 🔴 精读 · 系统调用追踪

## 本节要点

strace 是**系统调用追踪器**：它用 `ptrace` 接管目标进程，把进程发出的**每一个系统调用**（名字、参数、返回值）打印出来。系统调用是用户态与内核的唯一接口——文件读写、网络收发、内存分配、进程管理全都走 syscall。所以 strace 输出是「程序到底让内核做了什么」的完整流水账，是定位「卡在哪、多做了什么、参数对不对」的第一工具。

## 基本用法

```bash
strace ./prog              # 从头追踪一个程序
strace -o trace.log ./prog # 输出写到文件（stderr 会混进程序自己的输出）
strace -p 12345            # attach 到已运行进程
strace -f ./prog           # 也追踪 fork/线程出来的子进程
strace -c ./prog           # 只输出汇总统计，不逐条打印
```

```bash
# 追踪 orderbook 启动过程
strace ./orderbook 2>&1 | head -20
execve("./orderbook", ["./orderbook"], 0x7fff...) = 0
brk(NULL)                               = 0x555555559000
mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7f...
access("/etc/ld.so.preload", R_OK)      = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
...
write(1, "id=3 price=99.50 qty=-1\n", 22) = 22
exit_group(0)                           = ?
```

一眼看到程序生命周期：`execve` 加载自己 → `brk`/`mmap` 建堆和映射 → `openat` 加载动态库 → 干活 → `exit_group` 退出。

## 输出格式解读

每一行的结构是：

```
syscall_name(参数...) = 返回值
```

| 部分 | 例子 | 含义 |
|------|------|------|
| syscall 名 | `openat` | 系统调用名 |
| 参数 | `(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY\|O_CLOEXEC)` | 参数（已解码成可读形式） |
| 返回值 | `= 3` | 返回值：fd、字节数、0 成功等 |
| errno | `= -1 ENOENT (No such file or directory)` | 失败时 `-1` 后跟错误名 + 说明 |

### 返回值的含义（关键！）

| 返回值 | 含义 |
|--------|------|
| `= 3` | open 成功，返回 fd 3 |
| `= 0` | 成功（如 close、成功退出） |
| `= 22` | write 写了 22 字节 |
| `= -1 ENOENT (...)` | **失败**，errno=ENOENT（文件不存在） |
| `= -1 EAGAIN (...)` | 失败，资源暂时不可用（非阻塞 IO 常见） |

> **排查第一原则：grep 所有 `= -1` 的行**。失败的系统调用（尤其 `ENOENT`、`EACCES`、`EAGAIN`、`ECONNREFUSED`）往往是 bug 的直接证据。

### 参数解码

strace 会尽量把参数还原成人话：

```bash
# 字符串参数 → 加引号
openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3

# flags → 符号化（O_RDONLY 而不是 0）
# fd → 数字，但 -y 可显示 fd 对应的路径
strace -y -e trace=openat ./prog
openat(AT_FDCWD, "data.csv", O_RDONLY) = 3</dev/pts/0>   # -y 显示 fd 3 指向终端
```

## 常见 syscall 速览

| 类别 | syscall | 说明 |
|------|---------|------|
| 进程 | `execve` / `fork` / `clone` / `exit_group` | 加载 / 创建 / 退出 |
| 内存 | `brk` / `mmap` / `munmap` / `mprotect` | 堆与映射（malloc 底层） |
| 文件 | `openat` / `read` / `write` / `close` / `lseek` / `stat` | 文件 IO |
| 网络 | `socket` / `connect` / `sendto` / `recvfrom` / `accept` | 网络 |
| 同步 | `futex` / `epoll_wait` / `poll` | 锁 / 事件等待 |
| 时间 | `clock_gettime` / `nanosleep` | 计时 / 睡眠 |

## 时间戳：给每行 syscall 打时间

```bash
strace -t ./prog      # 时:分:秒
# 17:30:01 write(1, "id=3...", 22) = 22

strace -tt ./prog     # 加微秒
# 17:30:01.123456 write(1, "id=3...", 22) = 22

strace -ttt ./prog    # unix 时间戳 + 微秒（精确、可机器处理）
# 1725358201.123456 write(1, "id=3...", 22) = 22
```

> 配合 5.2 的 `-T`（每个 syscall 的耗时），时间戳能还原「哪个 syscall 之间卡了多久」。

## 一个完整的入门示例：追踪文件读取

```c
// readfile.c —— 读一个文件并打印
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char **argv) {
    FILE *f = fopen(argv[1], "r");
    if (!f) { perror("fopen"); exit(1); }
    char buf[128];
    while (fgets(buf, sizeof(buf), f))
        fputs(buf, stdout);
    fclose(f);
    return 0;
}
```

```bash
gcc -g -O0 -o readfile readfile.c
echo hello > /tmp/a.txt
strace ./readfile /tmp/a.txt 2>&1 | tail -12
openat(AT_FDCWD, "/tmp/a.txt", O_RDONLY) = 3      # fopen → openat
fstat(3, {st_mode=S_IFREG|0644, st_size=6, ...}) = 0
read(3, "hello\n", 4096)                     = 6   # fgets → read，读了 6 字节
write(1, "hello\n", 6)                       = 6   # fputs → write 到 stdout
read(3, "", 4096)                            = 0   # 读到 EOF
close(3)                                     = 0
exit_group(0)                                 = ?
```

关键洞察：**C 的 `fopen`/`fgets` 都是库函数，底层要落成 `openat`/`read`/`write` 这些 syscall**。strace 让你看到库函数背后真正的内核交互——这是理解「用户态 API vs 系统调用」的绝佳视角（也是 TLPI 反复强调的分层）。

> 📌 上面这段 strace 输出是**按格式给出的示意**（本环境跑不了 strace）。想拿到**实测的**返回值语义，用本节「动手」里的 `code/c5_2_syscall_map.c`——它把 `open` 拿到 fd 3、`ENOENT=2`、`EBADF=9` 这些数字真跑出来给你看。

## 动手：不用 strace，也能把「一行 syscall」跑出来（实测）

⚠️ **先说清楚限制**：本仓库的验证环境跑不了 strace —— 它靠 `ptrace` 接管目标进程，而编译服务容器里既没有 strace 也不允许 ptrace（真实 Linux 主机上没问题）。所以本节下面的 **strace 输出格式示例是手册格式**。

但**输出的语义可以实测**。我写了三份 demo，把「strace 那一行到底在说什么」变成能真跑、真看的事实：

### ① 返回值与 errno：`= -1 ENOENT` 是什么感觉

`code/c5_2_syscall_map.c` 每做一件事，就把「这条语句会产生什么 syscall、返回值是多少、errno 是什么」自己打印出来。实测（gcc 13.3.0）：

```text
【1】openat —— 创建/打开文件（strace 里最常见的第一行）
  open(O_CREAT|O_WRONLY|O_TRUNC, 0644)             -> 返回 3    errno=0(-)
【2】write —— 真正把数据交给内核
  write(fd, "hello\n", 6)                          -> 返回 6    errno=0(-)
【3】打开一个不存在的文件 —— 学会读 `= -1 ENOENT`
  open("/tmp/definitely_not_here_12345", O_RDONLY) -> 返回 -1   errno=2(No such file or directory)
【4】读回刚写的文件 —— read 的返回值是「字节数」，不是 0/成功
  open(path, O_RDONLY)                             -> 返回 3    errno=0(-)
  read(fd, buf, 63)                                -> 返回 6    errno=0(-)  内容=hello
【5】对一个已经关闭的 fd 操作 —— 学会读 EBADF
  write(已关闭的 fd, "x", 1)                       -> 返回 -1   errno=9(Bad file descriptor)
【6】写 /dev/full —— 学会读 ENOSPC
  open("/dev/full", O_WRONLY)                      -> 打不开（本环境无 /dev/full），跳过
【7】unlink —— 删除文件
  unlink(path)                                     -> 返回 0    errno=0(-)
```

把这张表和上面的「输出格式解读」对着看，就发现**它和 strace 的输出是一一对应的**：

| demo 打印的 | 对应的 strace 行 | 数值 |
|-------------|-----------------|------|
| `open(...) -> 返回 3` | `openat(AT_FDCWD, "...", O_CREAT\|O_WRONLY\|O_TRUNC, 0644) = 3` | fd = **3**（0/1/2 被 stdin/out/err 占了） |
| `open(...) -> 返回 -1 errno=2` | `openat(AT_FDCWD, "...", O_RDONLY) = -1 ENOENT (No such file or directory)` | ENOENT = **2** |
| `write(已关闭的 fd, "x", 1) -> -1 errno=9` | `write(3, "x", 1) = -1 EBADF (Bad file descriptor)` | EBADF = **9** |
| `unlink(path) -> 返回 0` | `unlinkat(AT_FDCWD, "...", 0) = 0` | 0 = 成功 |

**三条从实测里读出来的结论**：

1. **`= -1` 必须配 errno 才有意义**。`-1` 只说「失败」，是 `ENOENT(2)`（路径不存在）还是 `EACCES(13)`（没权限）还是 `EBADF(9)`（fd 非法）**决定了完全不同的排查方向**。这就是本节说的「grep 所有 `= -1` 行」之后要做的第二件事：**读 errno**。
2. **`errno` 只在返回 -1 时可信**。demo 里每步都先 `errno = 0` 再看，源码注释里也写了同一条口诀：**返回 ≥0 时 errno 的值是历史残留**，别去看它。strace 同样如此——它只在 `-1` 后面打 `[errno]`。
3. **fd 从 3 开始**是常态：0/1/2 已被 stdin/stdout/stderr 占用，所以程序里第一个 `open` 拿到的就是 3。看到 `= 3` 而不是 `= 1`，不是 bug。

### ② write 什么时候发生：stdio 缓冲（新手读 strace 的第一大困惑）

新手读 strace 最常见的疑问是：**「我 printf 了 5 次，为什么只有一条 `write(1, ...)`？」** 答案在 `code/c5_1_write_buffering.c` 里——三种缓冲模式各跑一次，用一个必然 SIGFPE 的结尾把差别变成**看得见的事实**：

| 模式 | `setvbuf` 参数 | 实测输出 | 退出码 |
|------|---------------|----------|--------|
| `nobuf` | `_IONBF`（无缓冲） | 3 行全打出来 | 136 |
| `line` | `_IOLBF`（行缓冲） | 3 行全打出来 | 136 |
| `buf` | 不动（默认，非 tty 时全缓冲） | **一行都没有** | 136 |

```bash
./c5_1_write_buffering nobuf   # → mode=nobuf ... 第 1/2/3 行都在
./c5_1_write_buffering buf     # → stdout 完全为空，直接 SIGFPE
```

**为什么 `buf` 模式下程序「什么都没输出」？** 因为 `printf` 只是把数据放进**用户态缓冲区**，真正调 `write(2)` 的时刻由缓冲模式决定。全缓冲下那 3 行还躺在缓冲区里，进程就被 SIGFPE 杀了——**缓冲区随进程一起消失**。

反过来看 strace，你就明白那一条 `write` 从哪来了：

```text
# nobuf：每次 printf 都立刻落到内核 → strace 里看到 3 条 write
write(1, "mode=nobuf  stdout_isatty=0 ...\n", 62) = 62
write(1, "第 1 行：printf 已经把数据放进用户态缓冲区\n", 52) = 52
write(1, "第 2 行：这时进程还没调用过 write(2)\n", 46) = 46

# buf（重定向到文件/管道）：3 次 printf 攒在缓冲区，一条 write 都没发出去就死了
#   → strace 里【看不到任何 write(1, ...)】
```

> 📌 **顺带解释了另一个经典现象**：「同一个程序，输出到终端正常、重定向到文件就没输出了」——因为 tty 让 stdout 默认变成**行缓冲**（每条 `\n` 刷一次），重定向到文件/管道则变成**全缓冲**（攒满 4096 字节才刷）。demo 里 `stdout_isatty=0` 这一行就是把这个前提打出来给你看。`c1_2_shrink_demo.c` 每条记录后写 `fflush(stdout)`，也是在对抗这件事。

### ③ read 的三种返回值：「卡住」的现场就是一行没返回值的 read

`code/c5_3_read_semantics.c` 故意用 16 字节的小缓冲区逐轮读 stdin，把 `read` 的返回值原样打印。实测（喂一个 40 字节的文件）：

```text
第 1 轮: read(0, buf, 16) = 16   请求 16 得到 16   内容="Hello, this is a"
第 2 轮: read(0, buf, 16) = 16   请求 16 得到 16   内容=" 40-byte test pa"
第 3 轮: read(0, buf, 16) =  7   请求 16 得到 7 ← 部分读（不足请求值，正常！）  内容="yload!
"
第 4 轮: read(0, buf, 16) =  0   ← EOF（对端关闭 / 数据读完）
```

| 返回值 | 含义 | 别误判成 |
|--------|------|----------|
| `N > 0` | 读到 N 字节，**可能远小于请求值**（第 3 轮的 7 < 16） | ❌「一次读完」——必须循环读 |
| `N == 0` | **EOF**：对端关闭 / 文件读完 | ❌「读失败」——这是正常结束 |
| `N == -1` | 失败，看 errno（`EINTR` 被打断 / `EAGAIN` 暂无可读） | ❌ 一律当致命错误退出 |

**而 strace 里「进程卡住」的现场长什么样**——就是一行**只有前半截、没有 `=` 的** read：

```text
recvfrom(5, 
```

它没有返回值，因为**它还没返回**。这正是本节的判据：strace 的最后一行就是卡住的位置；`recvfrom`/`read` 是等数据（行为类），`futex` 是等锁（并发类），两者排查方向不同（回想 1.2 的决策树）。

## HFT 关联

1. **启动自检**：新进程上线前 `strace` 一遍，确认它打开的文件、绑定的端口、读的配置都对，`= -1` 行全扫一遍，能提前暴露权限/路径/配置问题。
2. **「卡住」第一刀**：进程不动了，`strace -p <PID>` 看它最后停在哪个 `recv`/`read`/`futex`，比 gdb 更轻、无需符号。
3. **`= -1 EAGAIN` 语义**：非阻塞 socket 里 `EAGAIN` 是「暂时没数据」的正常信号，不是错误；但 `ECONNREFUSED`/`ETIMEDOUT` 是真故障——能区分这两类是读 strace 的基本功。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** strace 追踪的是「系统调用」还是「库函数」？`fopen` 会出现在 strace 输出里吗？

> strace 追踪**系统调用**（syscall），不是库函数。`fopen` 是 glibc 库函数，不会直接出现；strace 里看到的是它底层的 `openat`（以及可能的 `fstat` 等）。同理 `fgets`→`read`、`malloc`→`brk`/`mmap`。库函数要用 ltrace 追（见 5.3）。

**Q2:** strace 输出 `= -1 ENOENT (No such file or directory)` 是什么意思？

> 系统调用**失败**了：返回值 -1，`errno` 是 `ENOENT`，即「文件或目录不存在」。排查时 grep 所有 `= -1` 行，这类失败往往是 bug 或配置错误的直接证据。

**Q3:** 为什么 strace 能把参数还原成 `O_RDONLY`、`"/etc/ld.so.cache"` 这样的可读形式？

> 因为 strace 内置了每个 syscall 的**参数解码器**：知道 `openat` 第二参数字符串、第三参数是 flags 位掩码（能翻译成 `O_RDONLY|O_CLOEXEC`），知道 fd 数字可配合 `-y` 查 `/proc/<pid>/fd` 还原路径。这些解码规则是 strace 按 syscall 语义硬编码的。

**Q4:** `read(3, "hello\n", 4096) = 6` 里，`4096` 和 `6` 分别是什么？为什么不一样？

> `4096` 是**请求读的缓冲区大小**（第三个参数），`6` 是**实际读到的字节数**（返回值）。文件只有 6 字节，所以返回 6，没填满 4096。`read` 返回值 ≠ 请求值 是常态（尤其管道/socket），读 strace 要分清这两个数字。

**Q5:** 进程「卡住不动」，用 strace 看最后一行是 `recvfrom(5, `，能得出什么结论？

> 说明进程阻塞在**等 socket fd 5 收数据**——要么对端没发数据，要么网络断了但 TCP 没感知。这是「等 IO」而非「死循环/死锁」（死循环会看到大量重复 syscall，死锁会看到 `futex` 等待）。strace 的最后一行往往就是卡住的位置。注意那一行**只有前半截、没有 `=`**——因为它还没返回，这正是「卡住」的视觉标志。

**Q6:** 为什么「我 printf 了 5 次，strace 里只有一条 write」？

> 因为 `printf` 是 stdio 库函数，只把数据写进**用户态缓冲区**，真正调 `write(2)` 的时机由缓冲模式决定：无缓冲（`_IONBF`）每次立刻 write、行缓冲（`_IOLBF`）见 `\n` 就 write、全缓冲（默认用于文件/管道，4096 字节）攒满才 write。所以 5 次 `printf` 完全可能只对应 1 条 `write(1, ...)`。实测见 `code/c5_1_write_buffering.c`：`buf` 模式下进程被 SIGFPE 杀死时，缓冲区里 3 行输出**全部丢失**，stdout 为空——这就是「缓冲区还没刷到内核」的直接证据。

**Q7:** `read` 返回 `0` 和返回 `-1` 有什么区别？

> 完全不同，必须分清。返回 **0 = EOF**（对端关闭 / 文件读完），是**正常的结束条件**，循环该退出；返回 **-1 = 失败**，要看 `errno`：`EINTR`（被信号打断，应该重试）、`EAGAIN`（非阻塞下暂时没数据，也是重试）、其余（如 `EBADF`）才是真错误。把 EOF 当错误处理会导致「读完了却报错」，把错误当 EOF 处理会导致「静默截断数据」。实测对照见 `code/c5_3_read_semantics.c`。

**Q8:** 实测里 `open` 成功返回的 fd 是 `3`，为什么不是 `1`？

> 因为 0/1/2 已被占用：0=stdin、1=stdout、2=stderr，进程启动时就由 shell 配好了。内核分配 fd 时总是取**当前最小可用的非负整数**，所以程序里第一个 `open`/`socket` 拿到的通常是 3。看到 `= 3` 不是异常；但如果 `open` 返回 `= -1 EBADF`，那才是 fd 非法。

</details>

## 交叉引用

- [1.5 从源码到断点：gcc -g / gdb / strace 全链路](../../chapter-01-methodology/notes/05-build-to-debug-pipeline.md) —— strace 与 gdb 同走 ptrace、为何不需要 `-g`、以及两者的接力分工
- [5.2 strace 实战分析](02-strace-practical-analysis.md)
- [5.3 ltrace 库调用追踪](03-ltrace-library-calls.md)
- [2.5 加载 core 回溯](../../chapter-02-crash/notes/05-load-core-backtrace.md)
- [03.6 模块导读](../../README.md)
