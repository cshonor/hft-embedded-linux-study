# TLPI 第 25 章 — Process Termination

**优先级**：🔴（退出码只有 8 位 · `SIGKILL` 绕过全部用户态清理 · `fork` + stdio 重复输出）
**前置**：[Ch24 Process Creation](../chapter-24-process-creation/README.md)（`fork()` 复制的是用户态内存）
**后置**：[Ch26 Monitoring Child Processes](../chapter-26-monitoring-child-processes/README.md)（僵尸怎么被回收）· [Ch27 Program Execution](../chapter-27-program-execution/README.md)（`execve`）

> ⚠️ **本章是 TLPI 全 64 章里最短的一章**：书页 **531–539**，一共 **9 页**；
> man7 官方按章清单里也只有 **2 个程序**（`procexec/exit_handlers.c` 与 `procexec/fork_stdio_buf.c`）。
> 本仓库为此自写了 8 个源文件（见下面的「代码示例」），把原书只是**用文字描述**的
> 那些语义（三步顺序、插队首、exec 清空注册……）全部做成可运行、可核对的实测。

---

## 小节目录

- [25.1 Terminating a Process: `_exit()` and `exit()` 终止的两条路](notes/25.1-terminating-a-process-exit-and-exit.md)
- [25.2 Details of Process Termination 内核与库到底回收了什么](notes/25.2-details-of-process-termination.md)
- [25.3 Exit Handlers 注册、逆序执行与那个印错的数字](notes/25.3-exit-handlers.md)
- [25.4 Interactions Between fork(), stdio Buffers, and `_exit()` 那个打印两遍的 Hello world](notes/25.4-interactions-between-fork-stdio-buffers-.md)
- [25.5 Summary 全章速查](notes/25.5-summary.md)
- [25.6 Exercise 全章唯一一道题](notes/25.6-exercise.md)

---

## 章节目标

读完本章应当能回答这 6 个问题：

| # | 问题 | 答案所在的节 |
|---|------|-------------|
| 1 | `exit()` 与 `_exit()` 差在哪三步？ | §25.1 |
| 2 | 为什么 `exit(-1)` 到父进程手里是 255 而不是 −1？ | §25.1 / §25.6 |
| 3 | 进程一死，内核/库分别替你回收了什么？哪些**不**回收？ | §25.2 |
| 4 | exit handler 按什么顺序跑？为什么「插队首」？ | §25.3 |
| 5 | 为什么 `fork()` 之后 `Hello world` 会打印两遍？ | §25.4 |
| 6 | 为什么 `SIGKILL` 一定绕过所有清理？ | §25.1 / §25.3 |

### 一条主线：一个进程怎么消失，以及消失时谁替它收拾

```
        用户态                              内核态
   ┌──────────────────┐
   │ exit(status)     │  ① exit handler（逆序）
   │                  │  ② flush stdio
   │                  │  ③ _exit(status) ──────► do_exit((status & 0xff) << 8)
   └──────────────────┘                              │
                                                     ├─ exit_sem / exit_shm / exit_files / exit_fs
        用户态（另一条路）                            ├─ disassociate_ctty（控制终端 + SIGHUP）
   ┌──────────────────┐                              ├─ exit_mm（unmap / 解锁）
   │ _exit(status)    │ ─────────────────────────────┤
   │ 或被信号杀死      │  ①② 全部跳过                 └─ exit_notify ⇒ EXIT_ZOMBIE（等父进程 wait）
   └──────────────────┘
```

⭐ 这张图就是本章全部内容：**两条入口、三个只属于 `exit()` 的动作、一串内核清理、一个僵尸态**。

---

## 原书示例清单（man7 官方按章文件列表）

man7.org 的 TLPI 按章分发清单里，Ch25 只有 2 个文件：

| 文件 | 编号 | 所属节 | 书页 | 说明 |
|------|------|-------|------|------|
| `procexec/exit_handlers.c` | **Listing 25-1** | §25.3 | 535 | `atexit()` + `on_exit()` 混用，输出与原书**逐字一致** |
| `procexec/fork_stdio_buf.c` | **Listing 25-2** | §25.4 | 537–538 | `fork` + stdio 缓冲的经典坑（只演示病，不给药） |

⚠️ 三条必须说清的事实：

1. **§25.1 一个 Listing 都没有**（只有文字 + 三行 bullet 讲 `exit()` 的三步）。
2. **§25.2 也没有 Listing**（全是清单文字，没有代码）。
3. **§25.5 Summary / §25.6 Exercise 照例没有程序**；§25.6 **只有一道题**，
   而且原书**没有给答案**（答案在另一本 *Exercise Solutions* 里）。

⚠️ `fork_stdio_buf.c` 在前一章（Ch24）的 README 里就被标为「原书把它归 Ch25 的 Listing 25-2」——
本轮它正式归位到本章的 `code/` 目录（Ch24 的 `code/` 里那份是**旧 demo 性质的副本**，两章各自保留）。

---

## 易错清单

| # | 易错点 | 正确认识 |
|---|--------|---------|
| 1 | 以为 `exit()` 的 `status` 是 32 位的 | **只有低 8 位**走到父进程；内核在 syscall 入口就 `(code & 0xff) << 8`（`kernel/exit.c:991`） |
| 2 | 以为 `WEXITSTATUS()` 里做了截断 | 截断在**子进程调 `exit()` 那一刻**；`WEXITSTATUS` 只是把高字节取回来 |
| 3 | 用 `exit(130)` / `exit(139)` 表示「被中断/越界」 | 与「被信号 2 / 11 杀死」在 shell 里**无法区分**；退出码用 **0–127** |
| 4 | 以为 `atexit` 能兜住 `SIGKILL` | `SIGKILL` 连 `exit()` 都不走，**一条 handler 都不会跑** |
| 5 | 在 exit handler 里调 `_exit()` 想「提前收场」 | 剩下的 handler **和 stdio flush** 一起取消，连前面 `printf` 的都丢 |
| 6 | 在 exit handler 里调 `exit()` | SUSv3 判 **UB**；Linux 上「照常跑」，换个系统可能无限递归到栈溢出 |
| 7 | 以为 handler 里注册的新 handler 会排到**队尾** | 会**插到剩余列表的队首**（实测顺序 `C / B / D / A`） |
| 8 | 抄书上的 `_SC_ATEXIT_MAX` | 书上印的是 `2,147,482,647`，**实测是 `2,147,483,647`（= `INT_MAX`）**，原书勘误 |
| 9 | fork 出来的子进程用 `exit()` | 父进程的 stdio 缓冲被复制 ⇒ 输出**重复**、handler 跑两遍；子进程用 `_exit()` |
| 10 | 在 CE 上想复现「终端 vs 重定向」两种输出 | 沙箱 stdout 是 **socket**（`S_ISSOCK=1`）⇒ 只能看到**全缓冲**那一半 |
| 11 | 以为进程死了 System V 共享内存段 / 信号量名就没了 | 内核只 **detach / close**，对象是**持久化**的，要显式 `IPC_RMID` / `*_unlink` |
| 12 | 靠 `fcntl()` 记录锁做跨进程互斥 | 锁**随进程消失**（不是随 fd）；用 `F_OFD_SETLK` 或 `flock()` |
| 13 | `main` 不写 `return` | C89 下是 **UB**（实测退出码 42，来自返回寄存器残留值）；C99 起等价 `exit(0)` |
| 14 | `#define _BSD_SOURCE` 直接用 | glibc 2.39 报弃用警告（`include/features.h:196`）；新代码用 `_DEFAULT_SOURCE` |
| 15 | 以为 `atexit` 能「注销」 | 没有 API；用全局 flag 让 handler 自己跳过 |

---

## 章节链路

```
Ch24 Process Creation（fork/exit/wait/execve 四个调用的分工）
   │  §24.1 Figure 24-1：exit() 负责「少一个」
   ▼
Ch25 Process Termination  ← 本章
   │  §25.1 两条入口 + exit() 三步          → code/c25_exit_three_steps.c
   │  §25.2 内核清理 10 条                  → code/probe25.c + kernel/exit.c 坐标
   │  §25.3 exit handler：顺序 + 陷阱        → code/exit_handlers.c（官方 Listing 25-1）
   │                                          code/c25_atexit_reentry.c
   │                                          code/c25_exit_handler_no_return.c
   │                                          code/c25_exec_clears_handlers.c
   │  §25.4 fork + stdio 缓冲                → code/fork_stdio_buf.c（官方 Listing 25-2）
   │                                          code/c25_fork_stdio_three_fixes.c
   │  §25.6 习题 25-1（exit(-1) ⇒ 255）      → code/ex25_1_exit_minus_one.c
   ▼
Ch26 Monitoring Child Processes（wait/waitpid/W* 宏家族/僵尸/SIGCHLD）
```

⭐ 本章与 Ch26 的**接缝**只有一行代码：`kernel/exit.c:739` 的 `tsk->exit_state = EXIT_ZOMBIE;`
——本章讲到「变成僵尸」为止，把「僵尸怎么被回收」留给 Ch26。

---

## 双线提示

| 主线 | 本章要抓的点 |
|------|-------------|
| **C / 系统编程** | `exit()` 的三步顺序；低 8 位的**截断点在 syscall 入口**；`WIFEXITED` / `WIFSIGNALED` 的位段编码；`atexit` 的 LIFO 与「插队首」 |
| **内核（LKD3rd 对照）** | `do_exit()` 的清理序列（`exit_sem` / `exit_shm` / `exit_files` / `exit_fs` / `disassociate_ctty`）；`exit_notify()` 里的 `EXIT_ZOMBIE` 与 `kill_orphaned_pgrp()`；`do_group_exit()` 是信号杀的入口 |
| **HFT** | 热路径退出用 `_exit()`；**关键状态不能只靠 `atexit` 落盘**；日志改走 `write()` 避开 `fork` 复制缓冲；退出码做「模块 + 原因」两位编码 |
| **嵌入式** | daemon 的优雅退出 = `SIGTERM` → 主循环退出 → `exit()` → handler 收尾；`SIGKILL` 只能靠 watchdog 兜；musl **没有** `on_exit()` |

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | `exit()` = ①exit handler（**逆序**）+ ②flush stdio + ③`_exit()` |
| 2 | `_exit()` / 被信号杀：①②**都不做**；内核清理照做 |
| 3 | 退出码只有**低 8 位**；`status = (code & 0xff) << 8`，**截断在 syscall 入口** |
| 4 | `exit(-1)` ⇒ `status = 65280`、`WEXITSTATUS = 255` |
| 5 | 退出码用 **0–127**；128–255 留给「被信号 n 杀死」（`$? = 128 + n`） |
| 6 | `SIGKILL` **一定**绕过全部用户态清理（这是「先发 `SIGTERM`」的根本理由） |
| 7 | handler **LIFO**；handler 里再注册 ⇒ **插到剩余列表队首** |
| 8 | handler **不返回** ⇒ 剩余 handler **与 flush 一起取消** |
| 9 | `fork()` 复制 handler **注册副本**；`exec()` **清空**全部注册 |
| 10 | `_SC_ATEXIT_MAX` = **`INT_MAX`**（书上印错了 1000） |
| 11 | `fork()` 复制**用户态**缓冲 ⇒ `printf` 重复；`write` 直进内核 ⇒ 不重复 |
| 12 | 一个程序里**只让一个进程**（通常是父进程）用 `exit()` 收场 |
| 13 | 内核只 **detach / close**（shm、信号量名、消息队列名都是**持久对象**） |
| 14 | 内核保证清理**资源**，但**不保证落盘** |

---

## 参考

- Kerrisk · TLPI **Ch25**（书页 531–539），§25.5 的 Further information 指向 §24.6 的四本书
  （Bach / Goodheart / Bovet & Cesati / Love）
- `man 2 exit` · `man 2 exit_group` · `man 3 exit` · `man 3 _exit` · `man 3 atexit` · `man 3 on_exit`
- `man 2 wait` · `man 3 setvbuf`（为 §25.4 打底）
- 源码：Linux **v6.6** `kernel/exit.c` · glibc **2.39** `stdlib/exit.c` / `stdlib/exit.h` /
  `stdlib/cxa_atexit.c` / `stdlib/atexit.c` / `stdlib/on_exit.c` / `include/features.h`

---

## 代码示例

本章 `code/` 下共 **16 个源文件**，分四类：

### 原书镜像件（2 个，逐字取自 man7 官方 `procexec/`）

| 文件 | 出处 | 用途 |
|------|------|------|
| `exit_handlers.c` | Listing 25-1（书 p.535） | `atexit()` + `on_exit()` 的注册与逆序执行 |
| `fork_stdio_buf.c` | Listing 25-2（书 p.537–538） | `fork` + stdio 缓冲的重复输出 |

### 本仓库自写（8 个，**原书没有**）

| 文件 | 属于 | 做什么 |
|------|------|-------|
| `probe25.c` | §25.2 | 沙箱探针：`_SC_ATEXIT_MAX`、注册 20 万 handler、stdout 类型、rlimit |
| `c25_exit_three_steps.c` | §25.1 | 用 `write` / `printf` 混用把 `exit()` 三步顺序**变成可观察量** |
| `c25_fall_off_main.c` | §25.1 | 同一源文件编 `-std=c89` / `-std=c99`，比对「掉出 main 末尾」的退出码 |
| `c25_exit_handler_no_return.c` | §25.3 | handler 里 `_exit()` vs `exit()` 的两种后果并排跑 |
| `c25_atexit_reentry.c` | §25.3 | handler 里再注册 ⇒ 验证「插队首」（实测 `C / B / D / A`） |
| `c25_exec_clears_handlers.c` | §25.3 | `exec` 是否真的清空 handler 注册 |
| `c25_fork_stdio_three_fixes.c` | §25.4 | 原书的三种修法放进一个源文件（`argv[1]` 选模式 0–3） |
| `ex25_1_exit_minus_one.c` | §25.6 | 习题 25-1 的解：16 个边界值 + 被信号杀的对照 |

### 公共库镜像（3 个，来自 TLPI `lib/`）

`tlpi_hdr.h` · `get_num.c` · `get_num.h`（与 Ch23 / Ch24 用的是**同一份**，逐字一致）

⚠️ 本仓库的 `tlpi_hdr.h` 是**替身**（原书 `lib/tlpi_hdr.h` 引了一堆 `lib/` 内的头文件）。
替身唯一的**行为**偏差：`errExit()` / `fatal()` 打印 errno 助记名时没有 `ename` 表，
会退化成 `?UNKNOWN?`。本章的官方件 `exit_handlers.c` 会用到 `fatal()`，
但**整章没有触发过 `fatal()` 路径**，所以这个偏差在本章**观察不到**。

### 旧 demo（3 个，Ch25 早期版本遗留，**不是**原书内容）

`atexit_order.c` · `exit_vs_exit.c` · `fork_atexit.c`

它们只做「看一眼」级别的演示，且与本章结论**没有冲突**（`fork_atexit.c` 的
`./fork_atexit exit` 在**终端**上看起来「没问题」，正是 §25.4 要解释的那类假象）。
本轮的正式演示一律用上面两组文件。

### ⚠️ 会漂 / 会被截断的量（不要写进断言）

| 量 | 本轮实测 | 历史值 | 说明 |
|----|---------|-------|------|
| `_SC_CHILD_MAX` | **27191** | Ch24 那轮是 **54956** | CE 会换宿主机，**差近一倍** |
| 各作业 `execTime` | 18–41 ms | — | 取决于宿主机负载与调度 |
| `probe25` 的 `getppid()` | 1 | — | 沙箱里进程直接挂在 init 下 |

⚠️ CE 的 stdout 采集上限约 **32 KB**、单作业墙钟上限约 **20 秒**（超时被 `SIGKILL`）。
本章作业都很短，没有触及上限。

### 相对稳定（可以引用）

| 量 | 值 | 说明 |
|----|----|------|
| `_SC_ATEXIT_MAX` | `2147483647`（`0x7fffffff` = `INT_MAX`） | glibc 常量，不随宿主机变 |
| `EXIT_SUCCESS` / `EXIT_FAILURE` | `0` / `1` | 标准约定 |
| `BUFSIZ` | `8192` | glibc 全缓冲块大小 |
| `S_ISSOCK(1)` / `isatty(1)` | `1` / `0` | 沙箱 stdout 是 socket，**表征稳定** |
| 低 8 位规则 | `WEXITSTATUS == (值 & 0xff)` | 内核编码，零例外 |
| `exit(-1)` | `status = 65280`、`WEXITSTATUS = 255` | 内核编码 |
| 「插队首」顺序 | `C / B / D / A` | glibc 实现决定，2.39 上稳定 |
| 三种修法的输出 | 修法 A/B 都是 `Ciao` / `Hello world`；修法 C 是 `Hello world` / `Ciao` | 缓冲语义决定 |
| 掉出 `main` 末尾 | `c89` ⇒ 42、`c99` ⇒ 0 | 42 是**实现细节**（不可移植），0 是标准保证 |

---

## 与前后章

| 关系 | 章节 | 衔接点 |
|------|------|-------|
| **前置** | [Ch24 Process Creation](../chapter-24-process-creation/README.md) | `fork()` 复制**用户态地址空间**这件事，是 §25.3（handler 注册被复制）与 §25.4（stdio 缓冲被复制）的共同根因 |
| **前置** | [Ch13 File I/O Buffering](../chapter-13-file-io-buffering/README.md) | §13.2（stdio 缓冲在用户态）与 §13.7（stdio 与系统调用混用）是 §25.4 的两块地基 |
| **后置** | [Ch26 Monitoring Child Processes](../chapter-26-monitoring-child-processes/README.md) | 本章停在 `EXIT_ZOMBIE`；Ch26 讲 `wait()` 家族怎么把僵尸回收、`W*` 宏家族的全部编码 |
| **后置** | [Ch27 Program Execution](../chapter-27-program-execution/README.md) | `exec()` 会清空 handler 注册（§25.3 实测过），Ch27 展开 `execve()` 的整体语义 |
| **相关** | [Ch33 Threads: Further Details](../chapter-33-threads-further/README.md) | glibc 的 `exit()` 走 `exit_group` ⇒ **任一线程调 `exit()` 会结束整个进程** |
| **相关** | [Ch34 Process Groups, Sessions](../chapter-34-process-groups-sessions/README.md) | §25.2 第 5 / 8 条（控制终端 `SIGHUP`、孤儿进程组）的完整展开 |
| **相关** | [Ch47 System V Semaphores](../chapter-47-sysv-semaphores/README.md) · [Ch48 System V Shared Memory](../chapter-48-sysv-shared-memory/README.md) | §25.2 第 3 / 4 条（detach shm、`semadj` 加回） |
| **相关** | [Ch49 Memory Mappings](../chapter-49-memory-mappings/README.md) · [Ch50 Virtual Memory](../chapter-50-virtual-memory/README.md) | §25.2 第 9 / 10 条（`mlock` 解除、`mmap` unmap） |
