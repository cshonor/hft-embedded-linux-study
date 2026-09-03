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

> 说明进程阻塞在**等 socket fd 5 收数据**——要么对端没发数据，要么网络断了但 TCP 没感知。这是「等 IO」而非「死循环/死锁」（死循环会看到大量重复 syscall，死锁会看到 `futex` 等待）。strace 的最后一行往往就是卡住的位置。

</details>

## 交叉引用

- [5.2 strace 实战分析](02-strace-practical-analysis.md)
- [5.3 ltrace 库调用追踪](03-ltrace-library-calls.md)
- [2.5 加载 core 回溯](../../chapter-02-crash/notes/05-load-core-backtrace.md)
- [03.6 模块导读](../../README.md)
