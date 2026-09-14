# Ch2 demos — Fundamental Concepts

本目录的 **19 个 `.c` 文件**与本章 20 篇笔记对应（2.20 总结无独立 demo），**每个都在 Compiler Explorer（gcc 13.3.0，x86-64，Ubuntu 24.04 容器）上真实编译 + 运行过**，输出原样抄在对应笔记的「实测输出」块里。

**验证状态：19/19 `build code = 0` · `didExecute = True` · `diagnostics = 0`**（编译旗标 `-O0 -Wall -Wextra`；`c2_12` 额外 `-pthread`，`c2_9` 额外 `-ldl`）。

> 📌 **本章官方源码：无。** TLPI 第 2 章是概念章，原书**没有配套示例程序**——这 19 个 demo 全部是本模块自写并实跑验证的。每一节笔记的「代码」小节里那段 `c` 代码块，都与本目录的 `.c` 文件**逐字一致**（由 `sync_note_code.py` 灌入、`cmp_note_code.py` 校验），所以笔记正文引用的 gcc 行号（如 `<source>:33`）读者能直接对上。

---

## 19 个自写 demo

| 文件 | 对应节 | 演示什么 | 环境依赖 |
|------|--------|----------|----------|
| `c2_1_syscall.c` | 2.1 | 三条写路径（`printf` / `write` / 裸 `syscall`）走同一个 syscall；**缓冲时机实验**（`_exit` vs `exit`、`fflush` 位置） | 无 |
| `c2_2_minishell.c` | 2.2 | 迷你 shell：4 个内置命令 + `fork`/`exec`/`wait` 循环；`system()` 的 8 位退出码语义 | 需要 `/bin/sh`（容器无 → 返回 `32512`） |
| `c2_3_cred.c` | 2.3 | 三份身份（R/E/S）；**权限位三级匹配的完整推演**（纯逻辑）+ 用 mode 位做真实文件实验；`EACCES` vs `EROFS` | 读 `/proc/self/status`；`/tmp` 可写 |
| `c2_4_links.c` | 2.4 | 建硬/软链接、inode 与链接计数、`stat` 跟随 vs `lstat` 不跟随、删原文件后的行为、7 种文件类型一次认全 | `/tmp` 可写 |
| `c2_5_fd.c` | 2.5 | fd 是「最小可用整数」（不是递增）；`close(1)+open` 换 stdout；`O_CLOEXEC` 与 exec 的关系；`RLIMIT_NOFILE` | `/tmp` 可写 |
| `c2_6_exec.c` | 2.6 | `execve` 把子进程整个换掉；`execve` **不做 PATH 搜索**；三种 `errno`（`ENOENT`/`EACCES`/`ENOEXEC`）；换了什么/没换什么 | 用 `/proc/self/exe` 自举 |
| `c2_7_fork.c` | 2.7 | `fork` 一次调用两次返回；COW（地址相同、值不同）；僵尸与收尸；**`_exit()` 不 flush stdio**；fork 前不 `fflush` 打两遍；wait 宏 | 无 |
| `c2_8_mmap.c` | 2.8 | `MAP_PRIVATE`（改动不落回）/ `MAP_SHARED`（落回）/ `MAP_ANONYMOUS`（不要文件）三个实验 + 同一文件两映射互相可见 | `/tmp` 可写 |
| `c2_9_shared.c` | 2.9 | 启动时已加载的共享库；`dlopen`/`dlsym`/`dlerror` 运行时加载；静态 vs 动态链接 | **`-ldl`**；读 `/proc/self/maps` |
| `c2_10_ipc.c` | 2.10 | `pipe` 基本流；`PIPE_BUF`（原子写上限）vs `F_GETPIPE_SZ`（容量）；写端全关后读到 EOF；`socketpair`；`FIFO` | `/tmp` 可写（FIFO） |
| `c2_11_signal.c` | 2.11 | `sigaction` 注册；**屏蔽 ≠ 忽略**（屏蔽进 pending）；`SIGCHLD`；慢系统调用被信号打断（`EINTR`）；异步信号安全 | 无 |
| `c2_12_thread.c` | 2.12 | 线程与进程：pid 相同 tid 不同；`pthread_create`/`join`；共享 vs 私有；4 线程并发看 tid 与栈地址 | **`-pthread`** |
| `c2_13_pgroup.c` | 2.13 | 自己的进程组（`pid==pgid`）；fork 子进程默认同组；`setpgid(0,0)` 自成一组；**`kill(-pgid)` 广播**；`tcsetpgrp` | 无 |
| `c2_14_setsid.c` | 2.14 | 三层 ID（pid/pgid/sid）；控制终端四连测（`ttyname`/`ctermid`/`access`/`isatty`）；组长 `setsid` → `EPERM`；daemon 六步 | 探测 `/dev/tty`、`/dev/ptmx` |
| `c2_15_pty.c` | 2.15 | 试 `posix_openpt`（本容器 `ENOENT`）；PTY 结构与行规程职责；用 `socketpair` 复现双向骨架；谁在用 `-t` | **需 `/dev/ptmx` + devpts**（容器无） |
| `c2_16_clock.c` | 2.16 | 5 个 `clockid` 的当前值与分辨率；**同一段循环用三种时钟测**；REALTIME 与 MONOTONIC 的漂移；`CLOCK_REALTIME_COARSE` | 读 `/proc/uptime` |
| `c2_17_client_server.c` | 2.17 | `AF_UNIX`（路径）与 `AF_INET`（`127.0.0.1:47123`）各跑一遍 C/S；六种服务器形态表；`SIGPIPE` 机制 | `/tmp` 可写 + 可 `bind` 回环 |
| `c2_18_realtime.c` | 2.18 | `CapEff` 家底；`SCHED_FIFO` 的 `EPERM(1)` vs `EINVAL(22)`；CPU 亲和；`RLIMIT_MEMLOCK` 与 `mlockall` | 读 `/proc/self/status`；实时策略需权限 |
| `c2_19_proc.c` | 2.19 | `/proc` 的虚拟性（`st_dev=66`）；全局文件；**数进程**并找到自己；进程「全息档案」；工具 ↔ `/proc` 对照 | **必须能读 `/proc`** |

### 需要额外能力才能看全的

| demo | 容器里的表现 | 需要什么 |
|------|-------------|----------|
| `c2_2_minishell.c` | `system()` 返回 `32512`（= `127<<8`） | 系统里有 `/bin/sh` |
| `c2_15_pty.c` | `posix_openpt` → `ENOENT(2)` | 挂载了 devpts（`docker run -t` 或裸机） |
| `c2_18_realtime.c` | `SCHED_FIFO` → `EPERM(1)`；但 **`mlockall` 成功** | `CAP_SYS_NICE`（实时策略）/ 更大的 `RLIMIT_MEMLOCK` |
| `c2_14_setsid.c` | 只验到语义层（三层 ID、`EPERM` 条件） | 真终端 + 真按键（才能验「`Ctrl+C` 打到哪个组」） |
| `c2_3_cred.c` | `euid=0` 但 `CapEff=0x0` → mode 0000 文件 `EACCES` | 有 `CAP_DAC_OVERRIDE` 才能绕过权限位 |

> 本仓库所有 demo 的统一约定：**做不成的实验要写清楚做不成，不打印假的成功输出**（`c2_15` 段 ⑤ 就是范例：「本环境在第一步就 `ENOENT` 了，后面全部无法执行 —— 如实记录」）。

---

## 编译

**全部 19 个 demo**（注意 `c2_12` 要 `-pthread`、`c2_9` 要 `-ldl`；为了省事统一带上这两个旗标也无害）：

```bash
for f in c2_1_syscall c2_2_minishell c2_3_cred c2_4_links c2_5_fd \
         c2_6_exec c2_7_fork c2_8_mmap c2_9_shared c2_10_ipc \
         c2_11_signal c2_12_thread c2_13_pgroup c2_14_setsid c2_15_pty \
         c2_16_clock c2_17_client_server c2_18_realtime c2_19_proc; do
    gcc -O0 -Wall -Wextra -pthread -ldl -o "$f" "$f.c" || echo "FAIL $f"
done
```

**只编单节**（例如 fd 模型）：

```bash
gcc -O0 -Wall -Wextra -o c2_5_fd c2_5_fd.c
gcc -O0 -Wall -Wextra -pthread -o c2_12_thread c2_12_thread.c
gcc -O0 -Wall -Wextra -ldl -o c2_9_shared c2_9_shared.c
```

## 运行

| 命令 | 说明 |
|------|------|
| `./c2_1_syscall` | **先看段 ④**：`printf` 与 `write` 的输出顺序颠倒（`B→A→C→D`），全缓冲的实证 |
| `./c2_2_minishell` | 4 个内置命令；`system("nosuchcmd")` 返回 `32512` 要会读 |
| `./c2_3_cred` | 三份身份 + 权限位推演；注意段 ④ 的 `EACCES(13)` 与段 ⑤ 的 `EROFS(30)` |
| `./c2_4_links` | 段 ② 看 `st_nlink` 与 inode，段 ④ 看删原文件后软链变「断链」 |
| `./c2_5_fd` | 段 ② 是重点：关掉一个 fd 再开，证明「最小可用」不是「递增」 |
| `./c2_6_exec` | 段 ④ 的「换了什么/没换什么」表最值得记 |
| `./c2_7_fork` | **段 ④ 与段 ⑤ 是本章最值钱的两处**（`_exit` 不 flush；fork 前不 flush 打两遍） |
| `./c2_8_mmap` | 段 ③ 看两个 `MAP_SHARED` 映射互相可见 |
| `./c2_9_shared` | 段 ② 故意 `dlopen` 一个不存在的库，看 `dlerror()` 的文案 |
| `./c2_10_ipc` | 段 ② 的 `PIPE_BUF=4096` vs `F_GETPIPE_SZ=65536` 别搞混 |
| `./c2_11_signal` | 段 ② 的「屏蔽 ≠ 忽略」是重点；段 ④ 看 `errno=4 (EINTR)` |
| `./c2_12_thread` | 段 ④ 四个栈地址**正好差 `0x800000`**（8MB） |
| `./c2_13_pgroup` | 段 ② 两个 `pgid` 的对比（`2` → `3`）就是全节结论 |
| `./c2_14_setsid` | **段 ② 四连测**（`ctermid()` 有输出但设备不存在）；段 ④ 的「再调一次 `EPERM`」是彩蛋 |
| `./c2_15_pty` | 容器里只跑到段 ① 的 `ENOENT`，段 ③ 的 `socketpair` 骨架能跑 |
| `./c2_16_clock` | **段 ② 三个数字不同**（`6889676` / `5901373` / `5827561` ns）是本节的实验核心 |
| `./c2_17_client_server` | 段 ① 与段 ② 对比，看「换地址不换代码形状」 |
| `./c2_18_realtime` | **段 ① 与段 ④ 要对着看**：无 `CAP_IPC_LOCK` 却 `mlockall` 成功 |
| `./c2_19_proc` | 段 ③ 数进程（容器里只有 2 个）；段 ④ 是全息档案 |

### 在真机（非容器）上跑能看到更多

```bash
# 看完整的 / 目录只读与否差异
sudo mkdir -p /test-ro && sudo mount -o ro,loop /dev/null /test-ro 2>/dev/null

# 实时三件套成功路径（需要 root 或 setcap）
sudo ./c2_18_realtime
# 或只给需要的 capability（比给 root 安全）
sudo setcap cap_sys_nice+ep ./c2_18_realtime && ./c2_18_realtime

# PTY 系列：在真机上 c2_15 会走完全部路径
./c2_15_pty

# 迷你 shell 在真机上 system() 会返回 0
./c2_2_minishell
```

---

## 9 个必须知道的坑

1. **`_exit()` 不 flush stdio，`exit()` 才 flush。** `c2_7` 段 ④：子进程里 `printf` 之后直接 `_exit(0)`，那段输出**全丢**。而 `fork` 又**会复制未 flush 的缓冲**——所以 `c2_7` 段 ⑤ 里同一行被打印了**两遍**。**教训：`fork` 之前先 `fflush(NULL)`；`fork` 出的子进程用 `exit` 还是 `_exit` 要想清楚。**

2. **stdout 是 socket（非 tty）时是「全缓冲」，输出会「迟到」。** `c2_1` 段 ④：`printf`（走 stdio 缓冲，不立刻写出）与 `write`（立刻写出）混用时，实测顺序是 `B→A→C→D` 而不是代码顺序。**教训：日志与二进制 I/O 混用时，要么统一用 `write`，要么每步 `fflush`。**

3. **`euid == 0` 不等于「什么都能干」。** `c2_3` 实测：`euid = 0` 但 `CapEff = 0x0`，所以打开 `mode 0000` 的文件照样 `EACCES(13)`。**判据是 capability（`CAP_DAC_OVERRIDE` bit 1），不是 uid。**

4. **`EROFS(30)` 与 `EACCES(13)` 是完全不同的病。** `c2_3` 段 ⑤ / `c2_4`：写只读文件系统是 `EROFS`（挂载问题），写无权限文件是 `EACCES`（权限问题）。**嵌入式上看到「权限错」先查是不是只读根文件系统。**

5. **`kill(2)` 的 `pid` 参数是「选择器」，负数 = 广播。** `c2_13` 段 ③：`kill(-pgid, SIGTERM)` 给**整个进程组**发信号。⚠️ **广播前必须验证目标组不含自己**（demo 里的 `if (cpgid != getpgrp())`），否则进程在下一行之前就自杀了。

6. **`ctermid()` 不是「环境有没有终端」的证据。** `c2_14` 段 ②：`ctermid()` 打印 `"/dev/tty"`，但 `access("/dev/tty", F_OK) = -1`（`ENOENT`）。**判断终端只有 `isatty()`**（shell 里 `test -t 0`）。

7. **`mlockall()` 不需要 `CAP_IPC_LOCK`。** `c2_18` 段 ④：`CapEff = 0x0` 却 `mlockall` 成功——因为 `can_do_mlock()` 是**或**关系：`RLIMIT_MEMLOCK != 0` **就够**。所以「必须有 root」是错的。⚠️ 反过来，**`RLIMIT_MEMLOCK` 太小会导致部分锁定失败**，实测错误码是 `EPERM`。

8. **`PIPE_BUF`（原子写上限）与 `F_GETPIPE_SZ`（管道容量）是两个不同问题。** `c2_10` 段 ②：前者 `4096` 决定「多写者时一次 `write` 最多多少字节不会交错」，后者 `65536` 决定「管道能缓存多少」。**多进程写同一管道做消息传递时，消息必须 ≤ `PIPE_BUF`。**

9. **不处理 `SIGPIPE`，一个客户端就能打挂服务器。** `c2_17` 段 ④：客户端提前 `close`，服务器再 `write()` 会收到 `SIGPIPE`，**默认动作是终止整个进程**（不是返回 `EPIPE`！）。**服务器启动时必须 `signal(SIGPIPE, SIG_IGN)`；库代码用 `MSG_NOSIGNAL`。**

---

## 已知告警（都是故意的）

- **本章 19 个 demo 在 `-Wall -Wextra` 下全部零告警。** 这是刻意的目标——所以任何一条新告警都是「新引入的问题」，不要靠 `-Wno-*` 压。前置的设计手段包括：
  - `c2_5_fd.c` 用 `volatile` 变量承接 `getrlimit` 的可选出参，而不是声明后不用；
  - `c2_19_proc.c` 的 `char p[320], tgt[320]` 是为了让 `snprintf(p, sizeof(p), "/proc/self/fd/%s", d_name)` **不触发 `-Wformat-truncation`**（`d_name` 可达 255 字节，`p[128]` 会告警）；
  - `c2_19_proc.c` 注释里**不能出现 `/*` 字面**（会触发 `-Wcomment`）——所以写「`/proc/PID/` 目录下那一批」而不是「`/proc/PID/*`」。
- **`c2_15_pty.c` 段 ① 的 `ENOENT` 不是失败，是结论。** 它证明「PTY 需要 devpts 挂载」，而 `ENOENT`（而非 `EACCES`）正好定位到「设备不存在」而不是「权限不够」。**这是本 demo 的设计目标之一。**
- **`c2_18_realtime.c` 在容器里 `SCHED_FIFO` 全 `EPERM`，但整个程序仍返回 0。** 因为 demo 的目的就是**观察并解释**这些失败码（`EPERM` vs `EINVAL`），不是「把进程变成实时的」。所以失败是预期输出的一部分。
- **`c2_2_minishell.c` 里 `system("nosuchcmd")` 返回 `32512`** 是因为容器没有 `/bin`——这不是 demo 的 bug，而是**用来演示 8 位退出码语义**（`32512 = 127 << 8`）的素材。
- **`c2_17_client_server.c` 会在 `/tmp` 留一个 `c2_17.sock`。** 程序开头有 `unlink()`，所以重复运行不会 `EADDRINUSE`；但**若把 `unlink` 删掉**，第二次运行就会失败——这是刻意保留的教学点（见 [2.17 要点二](../notes/2.17-client-server.md)）。

---

## 相关脚本（在 `D:\.kernel-ref\`）

| 脚本 | 用途 |
|------|------|
| `sync_note_code.py <章目录>` | 把 `code/*.c` **逐字灌入**笔记里对应 `../code/X.c` 引用之后的第一个 ```c 块 |
| `cmp_note_code.py <章目录>` | 校验笔记内联代码与 `code/*.c` **是否一致**（不一致就说明手改了笔记没同步） |
| `verify_note_outputs.py <章目录> <日志>` | 校验笔记引用的**实测输出**能在 CE 日志里找到 |
| `ce_batch.py` / `ce_run.py` | 批量/单个提交到 Compiler Explorer 并收集输出 |
| `verify_ksrc.py` | 对 Linux **v6.6** 逐行核对笔记引用的**内核源码坐标** |
| `linkcheck.py <模块目录>` | 全模块 markdown **坏链检查**（必须 0 坏链） |

用法（PowerShell，因本机 bash 环境已损坏）：

```powershell
$PY = "C:\Users\12392\.workbuddy\binaries\python\envs\default\Scripts\python.exe"
& $PY D:\.kernel-ref\cmp_note_code.py chapter-02-basic-concepts
& $PY D:\.kernel-ref\verify_note_outputs.py chapter-02-basic-concepts tlpi-ch02/ch02-final.txt
```
