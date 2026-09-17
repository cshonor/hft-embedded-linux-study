# TLPI 第 24 章 — Process Creation

**优先级**：🔴（shell、服务、多进程模型的地基）
**前置**：[Ch23 Timers and Sleeping](../chapter-23-timers-sleeping/README.md) · [Ch20–22 Signals](../chapter-20-signals-fundamentals/README.md)
**后置**：[Ch25 Process Termination](../chapter-25-process-termination/README.md) · [Ch26 Monitoring Child Processes](../chapter-26-monitoring-child-processes/README.md) · [Ch27 Program Execution](../chapter-27-program-execution/README.md) · [Ch28 Process Creation: Further Details](../chapter-28-process-creation-exec-detail/README.md)

---

有 7 节（与官方 TOC 一致，编号 24.1–24.7）。其中 **24.2 带 24.2.1 / 24.2.2**；按仓库约定，**同一节的子节收在同一篇里用 `###`**，不拆成多文件。

> ⚠️ **本章只有 6 个 Listing**（24-1 ~ 24-6），而 §24.1 **一个都没有** —— 它只有一张
> Figure 24-1（四个调用的关系图）。本仓库为此自写了 `c24_lifecycle.c` 把那张图跑成输出。
>
> ⚠️ **官方 Ch24 分发目录有 7 个文件，其中第 7 个是习题解答**（`vfork_fd_test.c` = 习题 24-2 的解），
> 不属于任何 Listing。对账表见下。
>
> ⚠️ **同目录的 `procexec/fork_stdio_buf.c` 不属于本章** —— 官方把它归 **Ch25 的 Listing 25-2**。
>
> ⚠️ **原书 §24.4 点名的 `/proc/sys/kernel/sched_child_runs_first` 今天已经不存在了。**
> 实测沙箱（内核 7.0.0-1012-aws）上该文件返回 `ENOENT`；upstream 源码对照显示
> v6.6 里它已成空壳（零消费点），**v6.7 起连定义都被删除**。详见 [24.4](notes/24.4-race-conditions-after-fork.md)。
>
> ⚠️ **官方编译参数不是 `-Wall -Wextra`** —— 官方 `Makefile.inc` 是
> `-std=c99 -D_XOPEN_SOURCE=600 -D_DEFAULT_SOURCE -g -I${TLPI_INCL_DIR} -pedantic -Wall -W -Wmissing-prototypes -Wimplicit-fallthrough -Wno-unused-parameter`。
> 本仓库在 CE 上用 `-O0 -Wall -Wextra` 跑出的 `unused parameter` 之类警告**不是原书代码的问题**。
>
> ⚠️ **`usageErr()` / `errExit()` 的输出全在 stderr**（本章 `whos_first_help`、`vfork_fd_test`
> 两个作业的输出都只有 stderr，用 `2>/dev/null` 什么都看不到）。

## 小节目录

- [24.1 Overview of fork(), exit(), wait(), and execve() 四个调用的分工](notes/24.1-overview-of-fork-exit-wait-and-execve.md)（**原书本节无 Listing**；本仓库自写 `c24_lifecycle.c` 把 Figure 24-1 跑成输出）
- [24.2 Creating a New Process: fork()](notes/24.2-creating-a-new-process-fork.md)（含 24.2.1 文件共享 / 24.2.2 内存语义；Listing 24-1/2/3）
- [24.3 The vfork() System Call](notes/24.3-the-vfork-system-call.md)（Listing 24-4；SUSv3 三条 UB）
- [24.4 Race Conditions After fork()](notes/24.4-race-conditions-after-fork.md)（Listing 24-5；实测 91.6% / 8.4%，并验证原书那个开关已消失）
- [24.5 Avoiding Race Conditions by Synchronizing with Signals](notes/24.5-avoiding-race-conditions-by-synchronizin.md)（Listing 24-6；为什么必须在 `fork()` **之前**屏蔽）
- [24.6 Summary 小结](notes/24.6-summary.md)（含 Further information 的四本参考书）
- [24.7 Exercises 习题](notes/24.7-exercises.md)（五道题 + 官方解 1 个 + 本仓库自写解 3 个）

---

## 章节目标

- **`fork()` 是「一调两返」** —— 父进程里返回**子进程 PID**、子进程里返回 **0**、失败返回 **−1**。这是全网唯一的分叉判据。实测 `c24_lifecycle`：父 `fork() 返回 3`、子 `fork() 返回 0`（沙箱里 pid 从 2 开始，见下）
- **内存是「复制」的，fd 是「共享」的** —— 实测 `t_fork`：子 `idata=333 istack=666`、父 `idata=111 istack=222`（各有一份）而 `fork_file_sharing` 显示 offset 与 `O_APPEND` 都**共享**（同一个 open file description）
- **信号三件事的继承规则不同** —— 实测 `c24_sigmask_inherit`：**掩码继承**、**pending 清零**、**handler 表继承**。机制在 `kernel/fork.c:1091` `*dst = *src;`（整个 `task_struct` 拷一份）与 `kernel/fork.c:2391` `init_sigpending(&p->pending);`（pending 显式清）
- **`vfork()` 不复制地址空间，父进程被挂起** —— 实测 `t_vfork`：子进程 `sleep(3)` 之后仍然先打印，父进程读到的 `istack` 是子进程改过的 **666**（初值 222）；`execTime = 3027` 就是那 3 秒挂起
- **`fork()` 之后谁先跑不确定，而且会「成簇」** —— 实测 `whos_first_1000`：1000 次里 parent 先 **91.6%**、child 先 **8.4%**；但 `j ∈ [800, 860]` 那 61 次里 **56 次（92%）** 是 child 先，而紧邻的 `j ∈ [861, 999]` 共 139 次**一次都没有**。**这不是随机，是外部负载**
- **原书那个「调节旋钮」已经失效** —— `/proc/sys/kernel/sched_child_runs_first` 实测 `ENOENT`；`probe24_sched` 还拿到 `nice = 19`（最低优先级）、`SCHED_OTHER`、2 个 CPU —— **这说明「本机实测 xx%」必须连同调度环境一起说**
- **同步信号必须在 `fork()` 之前屏蔽** —— 原书 p.528 明说；反过来写会让父进程 `sigsuspend()` **永久挂起**（子进程跑得快时信号被空 handler 消费掉）。实测 `fork_sig_sync` 的后两行顺序被保证，前两行自由（书上是 child 先，我们是 parent 先，**都对**）
- **`WCOREDUMP` 不是布尔值** —— glibc 里它是 `status & 0x80`，只能取 0 或 128。实测 `ex24_3_core_dump`：`status=134`、`WTERMSIG=6`、`WCOREDUMP=128`，**但磁盘上没有 core 文件**（`./core` 与 `/core` 都 `ENOENT`）
- **容器里拿不到 core 是常态** —— 实测沙箱 `RLIMIT_CORE = 0/0` 且 hard limit 提不动（`setrlimit` 报 `EPERM`），而 `core_pattern` 是管道（`|/usr/share/apport/apport ...`）⇒ 管道模式下内核**不看** `RLIMIT_CORE`（`fs/coredump.c:613` 直接设成 `RLIM_INFINITY`），所以流程走完、位置上了，但文件不落地
- **诚实**：本章所有实测都跑在 **CE 公开 API（gcc 13.3.0 / x86-64 / Ubuntu 24.04，沙箱内核 7.0.0-1012-aws）**，不是本机。笔记明确标注了 CE 环境的硬边界：**20 秒 SIGKILL**、**约 32 KB 输出采集上限**、**stdout 是 socket ⇒ 8 KB 全缓冲**、**PATH 为空、`/bin/*` 全部不存在**（所以 `c24_lifecycle` 用 `/proc/self/exe` 做 exec 演示）、**`argv[0]` 恒为 `./output.s`**
- **溯源**：本章 **7 篇**笔记里的每一行实测输出都能在 CE 冻结日志 `tlpi-ch24-final.txt` 里定位（**17 个作业**）

### 一条主线：本章只有「一个分叉」和它每一步的代价

| 阶段 | 调用 | 它给了什么 | 它带来的问题 |
|------|------|-----------|-------------|
| 一 | `fork()` | 一个几乎完全相同的副本（内存 COW、fd 共享） | 谁先跑**不确定** ⇒ 竞态 |
| 二 | `fork()` + 靠顺序 | 「反正父进程先跑」的直觉 | 2.2.19 上 99.97% 成立、2.6.30 上反转成 child 先 99.98%、今天按 EEVDF 排 —— **同一个程序三种行为** |
| 三 | `vfork()` | 子进程借住父进程地址空间、父进程挂起 ⇒ **顺序确定** | SUSv3 三条 UB；COW 之后性能优势消失 ⇒ 原书结论是**避免使用** |
| 四 | `fork()` + **信号同步** | 用「先屏蔽、再 `sigsuspend`」把顺序**显式**定下来 | 信号只传一个 bit、会合并、handler 里几乎什么都不能调 |
| 五 | `fork()` + 管道/信号量/文件锁 | 真正工程化的同步（Ch44 / Ch53 / Ch55） | —— 本章只开个头 |

一句话：**本章没有「新概念」，只有「一个不确定的分叉点，和五种把它约束住的手段」。**

---

## 原书示例清单（man7 官方按章文件列表）

man7.org 在 Ch24 下分发 **7 个文件**（全部在 `procexec/` 目录），书里印了 **6 个 Listing** ——
差额是**习题 24-2 的官方解答**：

| 官方文件 | 原书 Listing | 本仓库位置 | 性质 |
|---------|-------------|-----------|------|
| `procexec/t_fork.c` | Listing 24-1 | `code/t_fork.c` | 正文程序（§24.2，父子各一份 stack/data） |
| `procexec/fork_file_sharing.c` | Listing 24-2 | `code/fork_file_sharing.c` | 正文程序（§24.2.1，共享 open file description） |
| `procexec/footprint.c` | Listing 24-3 | `code/footprint.c` | 正文程序（§24.2.2，比对内存足迹） |
| `procexec/t_vfork.c` | Listing 24-4 | `code/t_vfork.c` | 正文程序（§24.3，vfork 共享内存 + 父挂起） |
| `procexec/fork_whos_on_first.c` | Listing 24-5 | `code/fork_whos_on_first.c` | 正文程序（§24.4，父子竞态） |
| `procexec/fork_sig_sync.c` | Listing 24-6 | `code/fork_sig_sync.c` | 正文程序（§24.5，信号同步） |
| `procexec/vfork_fd_test.c` | —— | `code/vfork_fd_test.c` | **习题 24-2 的官方解答** |
| ⚠️ `procexec/fork_stdio_buf.c` | Listing **25-2** | 见 [Ch25](../chapter-25-process-termination/README.md) | **同目录，但不属本章** |

> ⚠️ 这批文件用到 TLPI 的公共头 `lib/tlpi_hdr.h`（及其依赖 `get_num.c` / `curr_time.c`）。
> 本仓库用一个**替身** `code/tlpi_hdr.h` 代替，差异逐条列在
> [`code/README.md`](code/README.md) 的「替身差异」表里。

---

## 易错清单

1. **把 `fork()` 理解成「重新运行程序」** —— 子进程从 `fork()` **返回处**继续，不清空变量、不重跑 `main`
2. **以为 `fork()` 之后内存是共享的** —— 是**复制**（COW）；共享的是 **fd 背后的 open file description**
3. **以为文件偏移不共享** —— offset 与 `O_APPEND` 状态都在 open file description 里，**父子共享**（实测 `fork_file_sharing`：`offset 0 → 1000`）
4. **忘了 `fork()` 之后 fd 是「加引用计数」而不是「复制对象」** —— 关掉一个不影响另一个；`dup`/`fork` 出来的 fd 指向同一个对象
5. **以为 `fork()` 之后父进程/子进程**一定**谁先跑** —— 两个方向都是错的；实测 91.6% / 8.4%（§24.4）
6. **用「本机多次运行都一样」当「行为确定」** —— 实测 `j ∈ [800,860]` 92% child 先、`j ∈ [861,999]` 0% —— **机器一忙，结论就翻**
7. **照抄书上的 `sched_child_runs_first` 调参** —— 实测内核上该文件 `ENOENT`；v6.7 起 upstream 已删除
8. **在多核上讨论「谁先」** —— 书 p.525 明说多处理器上两者可能**同时**拿到 CPU
9. **`printf` 到 stdout 却不 `setbuf(stdout, NULL)`** —— 父子各持一份全缓冲副本，顺序被缓冲吃掉；严重时同一行打印两次
10. **在 `fork()` **之后**才屏蔽同步信号** —— 原书 p.528 点名的错误；子进程跑得快时信号被空 handler 消费掉 ⇒ 父进程**永久挂起**
11. **在 `fork()` 之后才 `sigaction()`** —— 若信号在装 handler 之前递送，`SIGUSR1` 的**默认动作是终止进程**
12. **用 `pause()` 代替 `sigsuspend()`** —— 不是原子的，信号可能在两步之间丢失（经典竞态）
13. **`sigsuspend()` 的 `-1` 当错误处理** —— 正常被信号唤醒时**也**返回 −1 + `EINTR`；要写成 `if (rc == -1 && errno != EINTR)`
14. **以为 `fork` 之后子进程的信号掩码是干净的** —— **掩码继承**（实测），子进程也屏蔽着同一个信号
15. **以为 pending 信号会跟着走** —— **pending 清零**（实测 `c24_sigmask_inherit`：父 1 / 子 0）
16. **在 `vfork()` 子进程里改数据** —— 直接改到**父进程**的内存上（实测 `istack=666`）
17. **在 `vfork()` 子进程里 `return` / `exit()` / 调库函数** —— SUSv3 三条 UB；`return` 通常直接 SIGSEGV
18. **在 `vfork()` 子进程里用 `printf`** —— stdio 缓冲在**共享的**用户地址空间里；原书为此用裸 `write()`
19. **以为 `vfork` 之后 fd 表也共享** —— fd 表在**内核**里，vfork 仍然 `copy_files()`（实测 `vfork_fd_test`：父进程第一次 `close(1)` 成功）
20. **以为 `vfork` 比 `fork` 快所以该用** —— COW 之后差距很小；原书结论是**避免使用**
21. **在 NOMMU 平台上用 `fork`** —— 内核直接 `-EINVAL`（`kernel/fork.c` 的 `#ifndef CONFIG_MMU` 分支）
22. **`WCOREDUMP(status)` 当布尔值** —— 它是 `status & 0x80`，取值 **0 或 128**（实测 128）
23. **以为 `WCOREDUMP` 非 0 就是「磁盘上有 core 文件」** —— 实测 `WCOREDUMP=128` 而 `./core` 不存在
24. **以为把 `RLIMIT_CORE` 设大就能拿到 core** —— hard limit 是 0 时非 root **提不动**（实测 `EPERM`）；且 `core_pattern` 是管道时内核**根本不看** `RLIMIT_CORE`
25. **以为 `fork();fork();fork();` 是 3 个新进程** —— 是 `2³ − 1 = `**7** 个（实测 `ex24_1_fork_count`）
26. **忘了父进程必须 `wait()`** —— 子进程变僵尸；父进程先死则子进程变孤儿被 `init` 收养（实测出现 `ppid=1`）

---

## 章节链路

```text
                        fork()  ← 一个调用，两个返回点
                          │
      ┌───────────────────┼────────────────────┬──────────────────┐
      │                   │                    │                  │
   内存层面            文件层面             信号层面           顺序层面
      │                   │                    │                  │
   复制(COW)        共享 open file       掩码继承            「谁先跑？」
   §24.2.2          description          pending 清零         §24.4
   t_fork           §24.2.1              handler 继承         实测 91.6/8.4
   footprint        fork_file_sharing    c24_sigmask_inherit  成簇!
      │                   │                    │                  │
      └───────────────────┴────────────────────┘                  │
                          │                                       │
                    §24.3 vfork()                                 │
              不复制地址空间 / 父挂起                              │
              ⇒ 顺序「确定」，但语义危险                          │
                                                                  │
                                                    ┌─────────────┘
                                                    ▼
                                    §24.5 信号同步（Listing 24-6）
                                    ① fork 之前屏蔽 SYNC_SIG
                                    ② 子进程 kill() → 父进程 sigsuspend()
                                    ⇒ 后两行顺序被保证
                                                    │
                                                    ▼
                                    Ch44 管道 / Ch53 信号量 / Ch55 文件锁
                                    （工程上更常用的同步手段）
```

---

## 双线提示

| 线 | 本章拿什么去用 |
|----|--------------|
| **HFT** | ① **启动路径上的「父先做 A、子再做 B」是实盘事故高发点** —— 测试机永远是对的原因见 §24.4（机器闲 ⇒ 成簇偏向一侧）；正确做法是 pipe / `eventfd` / 信号**握手**，不是靠调度顺序；② `fork()` 的代价与**父进程地址空间大小**成正比（要复制页表）⇒ 想让 fork+exec 快，就该把父进程地址空间做小（别在父进程里 mmap 大块行情缓冲）；③ **实时调度（`SCHED_FIFO`/`SCHED_RR`）下不要把 `fork` 放在热路径** —— 通行做法是启动时一次性 fork/exec 好，之后只做进程间通信；④ 热路径里量出来的任何「调度顺序」结论都要连同 `nice` / CPU 数 / 宿主负载一起记录（本章实测环境就是 `nice=19`、2 核、共享宿主） |
| **嵌入式** | ① **NOMMU 平台（uClinux 等）`fork()` 直接不可用**（内核返回 `-EINVAL`）⇒ 只能 `vfork()+exec()`；这也是 BusyBox 在 NOMMU 下编译出不同代码的原因；② **单核上「谁先跑」只有两种可能，成簇效应更明显** ⇒ 更容易被误解成「确定行为」，写代码时更危险；③ 多线程进程 + `fork` 只保留调用线程（Ch33）⇒ 其它线程持有的锁可能永久锁死，**嵌入式固件里要 `fork` 就立刻 `exec`**；④ `daemon` 化的经典流程（双 fork + 父等子发信号）机制上就是 §24.5 那一套；⑤ 交叉工具链上 `_BSD_SOURCE` 这类 feature-test 宏要留意（`vfork()` 的声明依赖它） |
| **两条线共同的坑** | **顺序是「观测到的」而不是「保证的」** —— 本章所有百分比（99.97% / 99.98% / 91.6%）都是**特定内核 + 特定负载**下的测量值；CE 的 PATH 为空、`/bin/*` 不存在、`argv[0]` 恒为 `./output.s`、20 秒 SIGKILL、32 KB 输出上限都会影响演示形式 |

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | `fork()` 一调两返：父 = 子 PID，子 = 0，错 = −1 |
| 2 | 内存**复制**（COW）；fd **共享**（同一 open file description，含偏移） |
| 3 | `fork()` 复制的页表开销与**父进程地址空间大小**成正比 |
| 4 | 掩码**继承**；pending **清零**；handler **继承** |
| 5 | 机制：`*dst = *src;`（整个 `task_struct`）+ `init_sigpending(&p->pending)` |
| 6 | `vfork()` = `clone(CLONE_VM \| CLONE_VFORK \| SIGCHLD)` |
| 7 | `vfork` 子进程**只能碰 fd**，不能碰 stdio、不能 `return`、不能调库函数 |
| 8 | `vfork` 的父进程挂在 `wait_for_vfork_done()`，子进程 `exec`/`_exit` 时唤醒 |
| 9 | `fork()` 之后**谁先跑不确定** —— 不能假设，也不能靠「多跑几次」 |
| 10 | 顺序由 `wake_up_new_task()` → `check_preempt_curr(..., WF_FORK)` 定 |
| 11 | 原书那个 `sched_child_runs_first` **今天已不存在**（v6.7 起删除） |
| 12 | 同步信号必须在 **`fork()` 之前**屏蔽 —— 否则父进程可能永久挂起 |
| 13 | 同步用 `sigsuspend()` 而不是 `pause()`（后者有窗口） |
| 14 | 双向握手要**两个信号**，且两个都在 fork 前屏蔽 |
| 15 | `WCOREDUMP(status)` = `status & 0x80` ⇒ **0 或 128**，不是布尔值 |
| 16 | `WCOREDUMP` 非 0 ≠ 磁盘上有 core 文件 |
| 17 | 管道模式的 `core_pattern` 会**忽略** `RLIMIT_CORE` |
| 18 | `fork();fork();fork();` ⇒ 8 个进程 / **7** 个新的 |
| 19 | 父进程不 `wait()` ⇒ 僵尸；父进程先死 ⇒ 孤儿被 `init` 收养 |
| 20 | 本章 6 个 Listing；`vfork_fd_test.c` 是习题 24-2 的解；`fork_stdio_buf.c` 属 **Ch25** |

---

## 参考

- Kerrisk · TLPI Ch24（专有名词：copy-on-write / race condition / open file description / orphaned process group）
- man-pages：`man 2 fork` · `man 2 vfork` · `man 2 clone` · `man 2 wait` · `man 2 exit` · `man 2 execve` · `man 2 sigprocmask` · `man 2 sigsuspend` · `man 2 kill` · `man 2 core` · `man 5 core` · `man 3 pthread_atfork`
- Linux v6.6 源码：`kernel/fork.c`（`kernel_clone()` / `copy_process()` / `dup_task_struct()` / `copy_mm()` / `copy_files()` / `copy_sighand()` / `SYSCALL_DEFINE0(vfork)`） · `fs/file.c`（`dup_fd()`） · `mm/memory.c`（`copy_page_range()`） · `kernel/sched/fair.c`（`task_fork_fair()` / `place_entity()` / `check_preempt_wakeup()`） · `kernel/sched/core.c`（`wake_up_new_task()`） · `kernel/signal.c`（`rt_sigprocmask` / `rt_sigsuspend` / `kill` / `dequeue_signal` / `get_signal`） · `fs/coredump.c`（`do_coredump()` / `coredump_wait()`）
- glibc 2.39：`bits/waitstatus.h`（`__WCOREFLAG 0x80` / `__WCOREDUMP`） · `posix/sys/wait.h`
- **原书 §24.6 的 Further information**（推荐书目）：[Bach, 1986] *The Design of the UNIX Operating System* · [Goodheart & Cox, 1994] *The Magic Garden Explained* · [Bovet & Cesati, 2005] *Understanding the Linux Kernel* · [Love, 2010] *Linux Kernel Development, 3rd ed.*（**就是本仓库 05 模块对照的 LKD3rd**）
- 实测环境：Compiler Explorer 公开 API（gcc 13.3.0 / x86-64 / Ubuntu 24.04，沙箱内核 7.0.0-1012-aws），冻结日志 `tlpi-ch24-final.txt`（17 个作业）

---

## 代码示例

本章 `code/` 下有 **22 个源文件**（**7 个原书镜像 + 4 个公共库镜像 + 1 个替身头 + 7 个本仓库自写 + 3 个旧 demo**）。

### 原书镜像件（7 个，逐字取自 man7 官方 `procexec/`）

| 文件 | 出处 | 备注 |
|------|------|------|
| `code/t_fork.c` | **Listing 24-1** | 父子各一份 stack/data 副本（`t_fork` 有 3 秒 `sleep`） |
| `code/fork_file_sharing.c` | **Listing 24-2** | 共享 open file description（含 `O_APPEND` 开关对照） |
| `code/footprint.c` | **Listing 24-3** | 不改父进程内存足迹地调 `func()`；支持命令行参数 |
| `code/t_vfork.c` | **Listing 24-4** | vfork 共享内存 + 父进程挂起（`sleep(3)` 是**刻意的演示性违规**） |
| `code/fork_whos_on_first.c` | **Listing 24-5** | 父子竞态；支持 `[num-children]` 与 `--help` |
| `code/fork_sig_sync.c` | **Listing 24-6** | 信号同步的样板 |
| `code/vfork_fd_test.c` | **习题 24-2 的官方解答** | 证明 vfork 之后 fd 表是独立的 |

### 本仓库自写（7 个，**原书没有**）

| 文件 | 对应节 | 演示什么 |
|------|--------|---------|
| `code/probe24.c` | 全章 | **探针**（每章固定动作）：pid/ppid、`PATH`、`access(X_OK)` 扫描、`sysconf` 与 `rlimit` |
| `code/probe24_sched.c` | 24.4 | **探针二**：`/proc/sys/kernel/sched_child_runs_first` 在不在、`nice` / 调度策略；配套跨版本源码对照 |
| `code/c24_lifecycle.c` | 24.1 | **§24.1 原书没有 Listing** —— 把 Figure 24-1 的四个调用跑成输出（含 `exec` 不换 PID 的证明） |
| `code/c24_sigmask_inherit.c` | 24.2 | **延伸 demo**：掩码继承 / pending 不继承（**原书 Ch24 正文没有这条**） |
| `code/ex24_1_fork_count.c` | 24.7 习题 24-1 | 用「fork 之前建好的管道」数清 7 个新进程；顺带观察 `ppid=1` 的孤儿收养 |
| `code/ex24_3_core_dump.c` | 24.7 习题 24-3 | **fork 一个子进程让它崩** ⇒ 拿到 core 而父进程继续跑；顺便实测 `RLIMIT_CORE` 与 `core_pattern` |
| `code/ex24_5_fork_sig_sync2.c` | 24.7 习题 24-5 | 双向信号握手（`SIGUSR1` 子→父、`SIGUSR2` 父→子） |

### 公共库镜像（4 个，来自 TLPI `lib/`）

| 文件 | 说明 |
|------|------|
| `code/get_num.c` + `code/get_num.h` | `getInt()` / `getLong()`（`footprint` / `fork_whos_on_first` / `fork_sig_sync` 用） |
| `code/curr_time.c` + `code/curr_time.h` | `currTime()`（`fork_sig_sync` 用） |

### 替身（1 个）

| 文件 | 说明 |
|------|------|
| `code/tlpi_hdr.h` | 代替原书的 `lib/tlpi_hdr.h`（含 `error_functions` 的 `static inline` 最小实现）。**唯一行为偏差**：`ename[]` 表缺失 ⇒ 错误信息显示 `ERROR [?UNKNOWN? ...]` 而不是 `ERROR [EBADF ...]`（`vfork_fd_test` 的实测正是这个）。差异见 [`code/README.md`](code/README.md) |

### 旧 demo（3 个，Ch24 早期版本遗留）

| 文件 | 说明 |
|------|------|
| `code/fork_basic.c` | 返回值 / PID / COW 全局变量 |
| `code/fork_stdio_buf.c` | 缓冲重复输出 / `fflush`（⚠️ 这个主题**官方归 Ch25 的 Listing 25-2**） |
| `code/fork_fd_offset.c` | 父子共享文件偏移 |

编译与运行（在 `code/` 目录下）：

```bash
# 自带 tlpi_hdr.h 替身、依赖 get_num.c 的
gcc -O0 -Wall -Wextra -o t_fork t_fork.c get_num.c && ./t_fork
gcc -O0 -Wall -Wextra -o fork_file_sharing fork_file_sharing.c get_num.c && ./fork_file_sharing
gcc -O0 -Wall -Wextra -o footprint footprint.c get_num.c && ./footprint
gcc -O0 -Wall -Wextra -o t_vfork t_vfork.c get_num.c && ./t_vfork
gcc -O0 -Wall -Wextra -o fork_whos_on_first fork_whos_on_first.c get_num.c && ./fork_whos_on_first 1000
gcc -O0 -Wall -Wextra -o vfork_fd_test vfork_fd_test.c get_num.c && ./vfork_fd_test

# 需要 curr_time.c 的
gcc -O0 -Wall -Wextra -o fork_sig_sync fork_sig_sync.c get_num.c curr_time.c && ./fork_sig_sync
gcc -O0 -Wall -Wextra -o ex24_5_fork_sig_sync2 ex24_5_fork_sig_sync2.c curr_time.c && ./ex24_5_fork_sig_sync2

# 自包含（只依赖 libc）
gcc -O0 -Wall -Wextra -o probe24 probe24.c && ./probe24
gcc -O0 -Wall -Wextra -o probe24_sched probe24_sched.c && ./probe24_sched
gcc -O0 -Wall -Wextra -o c24_lifecycle c24_lifecycle.c && ./c24_lifecycle
gcc -O0 -Wall -Wextra -o c24_sigmask_inherit c24_sigmask_inherit.c && ./c24_sigmask_inherit
gcc -O0 -Wall -Wextra -o ex24_1_fork_count ex24_1_fork_count.c && ./ex24_1_fork_count
gcc -O0 -Wall -Wextra -o ex24_3_core_dump ex24_3_core_dump.c && ./ex24_3_core_dump
```

> ⚠️ **`t_fork` / `t_vfork` / `fork_sig_sync` / `ex24_5_fork_sig_sync2` 各含 2~3 秒 `sleep`**
> —— 它们不是慢，是在让「挂起 / 同步」这件事**看得见**。
>
> ⚠️ **`vfork_fd_test` 的输出全在 stderr**（它自己把 stdout 关掉了），用 `2>/dev/null` 什么都看不到。

### ⚠️ 会漂 / 会被截断的量（不要写进断言）

| 量 | 观察到的变化 |
|----|-------------|
| `whos_first_1000` 的 parent/child 占比 | **宿主负载一变就翻**（实测同一日志内 `[800,860]` 段 92% child 先、`[861,999]` 段 0%） |
| 沙箱 `pid` / `ppid` | 容器 PID namespace，每次运行都从 2 开始 |
| 各作业的 `execTime` | 受宿主负载影响（本章 3 秒 `sleep` 类作业实测 2047~3027 ms） |
| 子进程先跑的位置 `j` | 每次运行都不同 |
| `WCOREDUMP` 对应的 core 落盘情况 | 取决于 `core_pattern` 与目标目录是否可写 |

### 相对稳定（可以引用）

| 量 | 值 |
|----|----|
| `2³` 的结果 | 8 个进程 / **7** 个新进程 |
| `WIFEXITED` / `WEXITSTATUS(status)` | `7 << 8 = 1792` ⇒ `WEXITSTATUS = 7` |
| `WIFSIGNALED` / `WTERMSIG` / `WCOREDUMP` | `134 & 0x7f = 6`（SIGABRT）；`WCOREDUMP = status & 0x80 = 128` |
| `RLIMIT_CORE`（沙箱） | `0 / 0`，`setrlimit` 提 hard limit 报 `EPERM` |
| `core_pattern`（沙箱） | 以 `\|` 开头 ⇒ **管道模式** |
| `_SC_OPEN_MAX` / `_SC_CHILD_MAX` | `100` / `54956` |
| `PATH`（沙箱） | **空**；`/bin/*` 与 `/usr/bin/*` 全部 `access(X_OK) = -1` |
| `nice`（沙箱） | **19** |
| `fork()` 的两个返回点 | 父 = 子 PID，子 = 0 |

---

## 与前后章

| | 章 | 关系 |
|--|----|------|
| ← 前置 | [Ch23 Timers and Sleeping](../chapter-23-timers-sleeping/README.md) | `fork`/`exec` 对三类定时器的继承规则（`setitimer` 保留、POSIX 定时器清除、`timerfd` 保留）—— 与本章「掩码继承」是同一类问题的不同侧面 |
| ← 前置 | [Ch20 Signals: Fundamental Concepts](../chapter-20-signals-fundamentals/README.md) ~ [Ch22 Signals: Advanced Features](../chapter-22-signals-advanced/README.md) | `sigprocmask` / `sigsuspend` / async-signal-safe / realtime 信号 —— §24.5 的全部前提 |
| → 后置 | [Ch25 Process Termination](../chapter-25-process-termination/README.md) | `exit()` 与 `_exit()` 的差别、`atexit()`、stdio 缓冲在 `fork` 之后的行为（**官方把 `fork_stdio_buf.c` 归在这里，Listing 25-2**） |
| → 后置 | [Ch26 Monitoring Child Processes](../chapter-26-monitoring-child-processes/README.md) | `wait()` / `waitpid()` / 僵尸 / `SIGCHLD` —— 本章 `ex24_1_fork_count` 里的 `ppid=1` 就是它的引子 |
| → 后置 | [Ch27 Program Execution](../chapter-27-program-execution/README.md) | `exec()` 家族；`exec` 之后掩码与 handler 怎么变 |
| → 后置 | [Ch28 Process Creation: Further Details](../chapter-28-process-creation-exec-detail/README.md) | `clone()` 的全部标志；`CLONE_VM` / `CLONE_VFORK` 在完整标志序里的位置 |
| → 后置 | [Ch33 Threads: Further Details](../chapter-33-threads-further/README.md) | `fork` 与多线程的交互、`pthread_atfork()`、掩码 / pending 在**线程**语境下的差别 |
| → 后置 | [Ch44 Pipes and FIFOs](../chapter-44-pipes-fifos/README.md) · [Ch53 POSIX Semaphores](../chapter-53-posix-semaphores/README.md) · [Ch55 File Locking](../chapter-55-file-locking/README.md) | 原书 p.527–528 点名的另外三种同步手段 —— 工程上比信号更常用 |
