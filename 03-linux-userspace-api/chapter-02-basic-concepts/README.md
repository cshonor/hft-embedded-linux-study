# TLPI 第 02 章 — Fundamental Concepts

**优先级**：🔴 必读（全书地基）
**前置**：[Ch1 历史与标准](../chapter-01-introduction/README.md)（UNIX/Linux 简史、标准）
**后置**：[Ch3 系统编程概念](../chapter-03-system-programming-concepts/README.md)（系统调用 vs 库函数、`errno`）· [Ch4 文件 I/O](../chapter-04-file-io-universal/README.md)

---

## 小节目录

- [2.1 内核（Kernel）](notes/2.1-kernel.md) —— 内核组成 / 用户态与内核态 / **系统调用是唯一正规入口**；实测 **stdio 全缓冲让输出「迟到」**
- [2.2 Shell](notes/2.2-shell.md) —— Shell 是普通进程；读→`fork`→`exec`→`wait` 循环；**`exec` 成功即永不返回**
- [2.3 用户与组（Users and Groups）](notes/2.3-users-and-groups.md) —— UID/GID 家族、**文件权限的三级匹配**（owner→group→other）；**`euid=0` 但无 capability 时 mode 0000 照样 `EACCES`**
- [2.4 单根目录树、目录与链接（Directory Hierarchy）](notes/2.4-directory-hierarchy.md) —— 单根 `/`、硬链接 vs 软链接、**7 种文件类型**；`EEXIST(17)` vs `EXDEV(18)`
- [2.5 文件 I/O 模型（File I/O Model）](notes/2.5-file-io-model.md) —— 「一切皆文件」、fd 的**三层结构**（`fdtable`→`file`→`inode`）、`O_CLOEXEC`；`EMFILE(24)` vs `ENFILE(23)`
- [2.6 程序（Programs）](notes/2.6-programs.md) —— 程序 vs 进程、ELF、`execve` **换了什么/没换什么**；`ENOENT`/`EACCES`/`ENOEXEC` 三分
- [2.7 进程（Process）](notes/2.7-processes.md) —— 「进程 = 程序 + 数据 + 资源」、`TASK_*` 状态机、`fork` 与 COW；**`_exit()` 不 flush stdio**、fork 前不 `fflush(NULL)` 会打两遍
- [2.8 内存映射（Memory Mappings）](notes/2.8-memory-mappings.md) —— `mmap` 的**三种**映射（`MAP_SHARED`/`MAP_PRIVATE`/`MAP_ANONYMOUS`）、VMA；两个 `MAP_SHARED` 映射**互相可见**
- [2.9 静态库与共享库（Static & Shared Libraries）](notes/2.9-shared-libraries.md) —— 静态 vs 动态链接、`ld.so`、`dlopen`/`dlsym`/`dlerror`；用 `/proc/self/maps` **替代 `ldd`**
- [2.10 进程间通信与同步（IPC）](notes/2.10-ipc.md) —— IPC 分类（fd 型 / id 型 / 文件系统型）；**`PIPE_BUF` 与 `F_GETPIPE_SZ` 是两个不同问题**
- [2.11 信号（Signals）](notes/2.11-signals.md) —— 信号模型、`sigaction`、**「屏蔽」≠「忽略」**、`EINTR` 与异步信号安全
- [2.12 线程（Threads）](notes/2.12-threads.md) —— 线程 = 共享更多资源的 `task_struct`；`CLONE_*` 标志、`pid` vs `tgid`；**栈间距正好 8MB**、fd 是进程级的
- [2.13 进程组与作业控制（Process Groups）](notes/2.13-process-groups.md) —— PGID = 组长 PID、`setpgid`、**`kill(2)` 的负 `pid` = 组广播**、`Ctrl+C` 的真身
- [2.14 会话、控制终端与控制进程（Sessions）](notes/2.14-sessions.md) —— 会话 > 进程组 > 进程三层、`setsid` 的三件事与 `EPERM` 条件、**daemon 六步**；`ctermid()` 不是证据
- [2.15 伪终端（Pseudoterminals）](notes/2.15-pseudoterminals.md) —— master/slave、**行规程**才是 PTY 的价值、`docker -t` 是什么；**PTY 需要 devpts 挂载**
- [2.16 日期与时间（Date and Time）](notes/2.16-date-time.md) —— 日历时间 vs 进程时间、**7 个 `clockid` 的选择**、vDSO 免陷入；测间隔绝不用 `CLOCK_REALTIME`
- [2.17 客户端/服务器（Client-Server）](notes/2.17-client-server.md) —— C/S 四步循环、**同机 `AF_UNIX` vs 跨机 TCP**、六种服务器形态；`SIGPIPE` 能打挂服务器
- [2.18 实时（Realtime）](notes/2.18-realtime.md) —— 硬实时 vs 软实时、`SCHED_FIFO`/`SCHED_RR`、`mlockall`、`PREEMPT_RT`；**实时性改造的七层谱系**
- [2.19 /proc 文件系统（The /proc File System）](notes/2.19-proc-filesystem.md) —— procfs 虚拟性、**注册表驱动的实现**、进程「全息档案」；**所有监控工具都只是 `/proc` 解析器**
- [2.20 总结（Summary）](notes/2.20-summary.md) —— 19 个 demo 索引 + **本环境实测边界表** + 内核结构体↔章节对照 + HFT 最短路径

> 二十节的划分与 TLPI 原书一致（2.1 内核 / 2.2 Shell / 2.3 用户组 / 2.4 目录与链接 / 2.5 文件 I/O / 2.6 程序 / 2.7 进程 / 2.8 内存映射 / 2.9 库 / 2.10 IPC / 2.11 信号 / 2.12 线程 / 2.13 进程组 / 2.14 会话 / 2.15 伪终端 / 2.16 日期时间 / 2.17 C-S / 2.18 实时 / 2.19 /proc / 2.20 Summary）。
>
> 📌 **本章官方源码：无。** TLPI 第 2 章是概念章，原书**没有配套示例程序**。全部 19 个 demo 都是本模块自写并在 Compiler Explorer 上实跑验证的（见下）。

---

## 章节定位

**全书核心地基**：定义 UNIX/Linux 编程模型的核心术语，后面 60 多章反复复用。

- 打通 CSAPP 的「进程、虚拟内存、用户态/内核态」；
- 建立 **「内核边界 + 权限 + 进程/线程 + 内存 + IPC + 信号 + 时间」** 七块心智；
- 每块都有对应的内核对象（`sys_call_table` / `cred` / `task_struct` / `vm_area_struct` / `signal_struct`），深入时跳 `05-linux-kernel` / `06-linux-mm` 追源码。

**内核坐标已逐行核对**（Linux **v6.6**，源码逐行核对（本地 v6.6 缓存））。核对中发现并修正 **1 处真错**：`kuid_t`/`kgid_t` 在 `include/linux/uidgid.h:21-28`，**不在** `include/linux/types.h:37-38`（详见 [2.3 源码坐标](notes/2.3-users-and-groups.md)）。

---

## 双线提炼

### 嵌入式 Linux 应用

- fd、权限、进程继承、`/dev` 设备文件 = 应用与驱动交互的基础；
- 用户态**不能**绕过 syscall 直接操作硬件；
- **`EROFS(30)` 是嵌入式最常见的「假权限错」**——只读根文件系统不是权限问题（[2.3](notes/2.3-users-and-groups.md)）；
- `/proc` 是**唯一**在只读根 FS 上始终可写的调参通道（[2.19](notes/2.19-proc-filesystem.md)）；
- 串口控制台是嵌入式唯一的真终端，**拔串口 = `SIGHUP`**（[2.14](notes/2.14-sessions.md)）。

### HFT 低延迟

1. **少 syscall、少上下文切换**——`epoll` / `io_uring` 而非 thread-per-conn（[2.17](notes/2.17-client-server.md)）；
2. **测耗时别用墙上时钟**——`CLOCK_MONOTONIC`（[2.16](notes/2.16-date-time.md)）；
3. **信号要管控**——`SIGPIPE` 会打挂服务器、`EINTR` 要循环重试（[2.11](notes/2.11-signals.md) / [2.17](notes/2.17-client-server.md)）；
4. **理解 VA 隔离 → `mlock` 防换页 + CPU 亲和**（[2.8](notes/2.8-memory-mappings.md) / [2.18](notes/2.18-realtime.md)）；
5. **同机通信改 `AF_UNIX`**——业务代码一行不改，省掉整个 TCP/IP 栈（[2.17](notes/2.17-client-server.md)）；
6. **只看长尾**——「把 p99/max 砍掉」而不是追平均值（[2.18](notes/2.18-realtime.md)）。

---

## 本章 19 个 demo

所有 demo 都在 Compiler Explorer（gcc 13.3.0 / x86-64 / Ubuntu 24.04 容器）上**真实编译 + 运行**过，**19/19 `build code = 0` · `didExecute = True` · `diagnostics = 0`**（`-O0 -Wall -Wextra`；`c2_12` 加 `-pthread`，`c2_9` 加 `-ldl`）。输出原样抄在各节笔记的「实测输出」块里。

| 文件 | 节 | 演示什么 |
|------|----|----------|
| `code/c2_1_syscall.c` | 2.1 | 三条写路径（`printf`/`write`/`syscall`）+ **缓冲时机**实验 |
| `code/c2_2_minishell.c` | 2.2 | 迷你 shell：4 个内置命令 + `fork`/`exec`/`wait` 循环 |
| `code/c2_3_cred.c` | 2.3 | 三份身份 + **权限位三级匹配的完整推演** + mode 位真实文件实验 |
| `code/c2_4_links.c` | 2.4 | 建硬/软链接、inode 与链接计数、删原文件后的行为、7 种文件类型 |
| `code/c2_5_fd.c` | 2.5 | fd 是「最小可用整数」、`close(1)+open` 重定向、`O_CLOEXEC`、`RLIMIT_NOFILE` |
| `code/c2_6_exec.c` | 2.6 | `execve` 换进程、PATH 搜索、三种 `errno`、**换了什么/没换什么** |
| `code/c2_7_fork.c` | 2.7 | fork 两次返回、COW、僵尸进程、**`_exit` 不 flush**、fork 前不 flush 打两遍 |
| `code/c2_8_mmap.c` | 2.8 | `MAP_PRIVATE`/`MAP_SHARED`/`MAP_ANONYMOUS` 三实验 + 对照表 |
| `code/c2_9_shared.c` | 2.9 | 启动时已加载的库、`dlopen`/`dlsym`/`dlerror`、静态 vs 动态 |
| `code/c2_10_ipc.c` | 2.10 | `pipe`、容量与原子写上限、EOF、`socketpair`、`FIFO` |
| `code/c2_11_signal.c` | 2.11 | `sigaction`、**屏蔽 vs 忽略**、`SIGCHLD`、`EINTR`、异步信号安全 |
| `code/c2_12_thread.c` | 2.12 | `pthread_create`/`join`、共享 vs 私有、4 线程 tid 与栈地址 |
| `code/c2_13_pgroup.c` | 2.13 | 进程组、`setpgid(0,0)`、`kill(-pgid)` 广播、作业控制链路 |
| `code/c2_14_setsid.c` | 2.14 | 三层 ID、控制终端四连测、`setsid` 的 `EPERM`、daemon 六步 |
| `code/c2_15_pty.c` | 2.15 | PTY 结构、行规程职责、`socketpair` 替代骨架、**如实记录 `ENOENT`** |
| `code/c2_16_clock.c` | 2.16 | 一族 `clockid` 对比、**用哪个测耗时**实验、REALTIME 漂移、`COARSE` |
| `code/c2_17_client_server.c` | 2.17 | `AF_UNIX` 与 `AF_INET` 各跑一遍、六种服务器形态、`SIGPIPE` |
| `code/c2_18_realtime.c` | 2.18 | capability 家底、`SCHED_FIFO` 的 `EPERM`/`EINVAL`、亲和、`mlockall` |
| `code/c2_19_proc.c` | 2.19 | `/proc` 虚拟性、全局文件、数进程、进程全息档案、工具原理 |

完整索引、编译/运行一览与已知坑见 [`code/README.md`](code/README.md)。

**质量纪律**（三个脚本保证）：

| 脚本 | 保证什么 |
|------|----------|
| `sync_note_code.py` | 笔记内联 `c` 代码块与 `code/*.c` **逐字一致** |
| `cmp_note_code.py` | 校验上一条（防止手改笔记后不同步） |
| `verify_note_outputs.py` | 笔记引用的实测输出**能在 CE 日志里找到** |

---

## 本环境实测边界（读各节实测输出前必看）

全部实测跑在 CE 的 Ubuntu 24.04 容器里，与普通 Linux 有几处关键差异——**很多「意外」结论其实是这些差异造成的**：

| 事实 | 值 | 影响节 |
|------|-----|--------|
| `uid=0` 但 **`CapEff = 0x0`** | 没有任何 capability | [2.3](notes/2.3-users-and-groups.md) · [2.18](notes/2.18-realtime.md) |
| **没有 `/bin`** | `system()` 必返回 `32512` | [2.2](notes/2.2-shell.md) |
| **没有 `/dev/ptmx`、`/dev/pts`、`/dev/tty`、`/dev/shm`** | 相关调用全 `ENOENT` | [2.14](notes/2.14-sessions.md) · [2.15](notes/2.15-pseudoterminals.md) |
| 0/1/2 都是 **socket**（`isatty = 0`） | stdio **全缓冲**（`BUFSIZ=8192`） | [2.1](notes/2.1-kernel.md) · [2.19](notes/2.19-proc-filesystem.md) |
| `/` 只读（`EROFS`）；`/tmp`(1777)、`/app` 可写 | `O_CREAT` 到 `/` 报 `EROFS` | [2.3](notes/2.3-users-and-groups.md) · [2.4](notes/2.4-directory-hierarchy.md) |
| `ncpu = 2` · `RLIMIT_NOFILE = 100` · `RLIMIT_MEMLOCK ≈ 2GB` | 亲和集 `0 1`；可撞 `EMFILE`；**无 capability 也能 `mlockall`** | [2.5](notes/2.5-file-io-model.md) · [2.18](notes/2.18-realtime.md) |
| `pid_max = 4194304`，但只见 **2 个进程** | PID namespace 隔离 | [2.19](notes/2.19-proc-filesystem.md) |
| 内核 `7.0.0-1012-aws`，**`PREEMPT`** | 非 `PREEMPT_RT` | [2.19](notes/2.19-proc-filesystem.md) |

> **「实测」只对行为有效，不对版本号有效**——CE 的 `7.0.0` 内核比你本地的可能新，所以**笔记里的源码坐标统一以 v6.6 为准**。

---

## 自测（答案）

1. **为何进程不能直接访问对方内存？靠什么隔离？**
   各有独立**虚拟地址空间**；由**内核 + MMU/页表**隔离保护。

2. **系统调用为何有性能开销？**
   特权级切换、保存/恢复上下文、进内核执行路径——**上下文切换**成本。

3. **stdin/stdout/stderr 编号？**
   **0 / 1 / 2**。

4. **硬链接 vs 软链接？**
   硬链接：多名字 → **同一 inode**（`st_size` 跟着目标变）；软链接：存**路径字符串**（`st_size == 路径长度`，可指不存在目标）。

5. **测代码耗时用哪种时钟？**
   用**单调/CPU 相关计时**（`CLOCK_MONOTONIC` 或 `CLOCK_MONOTONIC_RAW`）；**不要**用会跳变的墙上日历时间当唯一依据（`CLOCK_REALTIME` 被 NTP 回拨会算出负区间）。书中「CPU 时间」适合看占核；测墙钟耗时用**单调时钟**更稳妥。

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 用户态受限；进内核靠 **syscall**（x86-64 经 `sys_call_table`） |
| 2 | 进程 = 独立 VA + 资源集合；线程**共享 VA**（`clone(CLONE_VM)`），故共享状态必须同步 |
| 3 | 一切皆文件；fd 0/1/2；`fork` 继承 fd（`CLONE_FILES` 让线程**共用** fd 表） |
| 4 | inode 无文件名；硬链同 inode，软链存路径 |
| 5 | 权限检查看 **euid/fsuid + 补充组**；**`uid=0` ≠ 万能**（看 `CapEff`） |
| 6 | `fork` 是 `clone` 只设 `exit_signal` 的特例；线程/进程是**同一机制的两个参数组合** |
| 7 | `kill(2)` 的**负 `pid` = 向整个进程组广播**——`Ctrl+C` 的真身 |
| 8 | 控制终端是**会话**的属性；`setsid` 三件事且**拒绝组长**（`EPERM`） |
| 9 | 测间隔用 `CLOCK_MONOTONIC`；`CLOCK_MONOTONIC` **不含 suspend**（要 `BOOTTIME`） |
| 10 | `clock_gettime` 走 **vDSO**——用户态直读，不陷入内核 |
| 11 | 同机 `AF_UNIX` > TCP loopback；**换地址不换代码形状** |
| 12 | 实时 = **确定性**（最坏情况有上界），HFT 只看 **p99/max** |
| 13 | `mlockall` 需要 `CAP_IPC_LOCK` **或** `RLIMIT_MEMLOCK > 0`（**不是必须 root**） |
| 14 | `/proc` 是**伪文件系统**：`read` 就是调一段内核 show 函数；所有监控工具都是它的文本解析器 |

---

## 参考

- Kerrisk · TLPI Ch2 Fundamental Concepts
- `man 2 syscall` · `man 2 fork` · `man 2 execve` · `man 7 signal` · `man 7 pthreads` · `man 7 credentials` · `man 2 setsid` · `man 3 posix_openpt` · `man 2 clock_gettime` · `man 7 vdso` · `man 2 socket` · `man 7 unix` · `man 7 sched` · `man 2 mlockall` · `man 5 proc` · `man 7 capabilities`
- Linux **v6.6** 源码（逐行核对）：`arch/x86/entry/syscall_64.c` · `arch/x86/entry/syscalls/syscall_64.tbl` · `include/linux/{sched.h,uidgid.h,fdtable.h,mm_types.h}` · `include/linux/sched/signal.h` · `kernel/{fork.c,signal.c,sched/core.c}` · `kernel/sched/core.c` · `mm/{mmap.c,mlock.c}` · `fs/{namei.c,open.c,read_write.c}` · `fs/proc/{base.c,meminfo.c}` · `lib/vdso/gettimeofday.c` · `net/unix/af_unix.c` · `net/ipv4/tcp.c` · `drivers/tty/pty.c`
- [OUTLINE](../OUTLINE.md) · [模块 README](../README.md)
- 下一章：[Ch3 系统编程概念](../chapter-03-system-programming-concepts/README.md)

---

## 代码示例

本章的 demo 都在 [`code/`](code/README.md) 目录下，每个都可独立编译、无外部依赖（不需要原书源码）。快速上手：

```bash
# 一次跑完全部 19 个 demo
cd code && for f in c2_*.c; do
  gcc -O0 -Wall -Wextra -pthread -ldl -o "${f%.c}" "$f" && "./${f%.c}"
done
```

单个运行（例：fd 模型）：

```bash
gcc -O0 -Wall -Wextra -o c2_5 code/c2_5_fd.c && ./c2_5
```

> 若某个 demo 想更贴近本机（而不是容器）的环境，注意 `c2_15_pty.c` 需要系统挂载了 **devpts**（普通桌面 Linux 默认有，容器需要 `docker run -t`）。
