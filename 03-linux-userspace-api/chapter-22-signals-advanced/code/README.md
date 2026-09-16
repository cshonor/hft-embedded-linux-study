# Ch22 code — Signals: Advanced Features

> 实测环境：**macOS 26.6.2 (arm64) + clang 23.1.0**（micromamba `cdev`）。
> 本章是信号三部曲的 Linux 专有重头（RT 信号 / sigwaitinfo / signalfd），macOS 上
> **5 个程序可编译实测、5 个 Linux 专有只做源码核验**——全部如实标注。

## 文件清单

### 原书 Listing 逐字镜像（man7 官方 dist `signals/`）

| 文件 | 镜像 | 覆盖 | 节 |
|------|------|------|----|
| [`signal.c`](signal.c) | **Listing 22-1** | `signal()` 的可移植实现（仅 Solaris/SGI 生效，其他平台 dummy） | 22.7 |
| [`t_sigqueue.c`](t_sigqueue.c) | **Listing 22-2** | `sigqueue()` 发带 payload 的 RT 信号 | 22.8.1 |
| [`catch_rtsigs.c`](catch_rtsigs.c) | **Listing 22-3** | SA_SIGINFO + RT 信号排队验证 | 22.8.2 |
| [`t_sigsuspend.c`](t_sigsuspend.c) | **Listing 22-5** | `sigsuspend()` 原子等待（`USE_PAUSE` 版对照） | 22.9 |
| [`t_sigwaitinfo.c`](t_sigwaitinfo.c) | **Listing 22-6** | 同步取信号（无 handler） | 22.10 |
| [`signalfd_sigval.c`](signalfd_sigval.c) | **Listing 22-7** | 信号 → 文件描述符（可 epoll） | 22.11 |
| [`demo_SIGFPE.c`](demo_SIGFPE.c) | 官方补充 | SIGFPE 触发 + 忽略/阻塞组合 | 22.4 |
| [`sig_speed_sigsuspend.c`](sig_speed_sigsuspend.c) | 官方补充 | 父子 sigsuspend 往返测速 | 22.9 |

> 书上无 Listing 22-4 对应文件。dist `signals/` 混装三章程序，本章只放这 8 个。

### 自编 demo

| 文件 | 覆盖 | 节 |
|------|------|----|
| [`sigsuspend_wait.c`](sigsuspend_wait.c) | 阻塞 + `sigsuspend` 安全等待范式（**先阻塞才不丢**） | 22.9 |
| [`sigwaitinfo_loop.c`](sigwaitinfo_loop.c) | `sigwaitinfo` 同步循环取信号（无 handler） | 22.10 |
| [`sigqueue_rt.c`](sigqueue_rt.c) | `sigqueue` 自发 RT 信号带 int payload | 22.8 |

### 支撑文件

| 文件 | 说明 |
|------|------|
| [`tlpi_hdr.h`](tlpi_hdr.h) | macOS 最小替身（含 get_num.h） |
| [`get_num.c`](get_num.c) / [`.h`](get_num.h) | 官方 dist 原版（Listing 3-5/3-6） |
| [`signal_functions.c`](signal_functions.c) / [`.h`](signal_functions.h) | 跨章依赖（Listing 20-4，t_sigsuspend 用） |

## 编译分类（macOS 实测结果）

```bash
CC=clang   # cdev 环境的 clang 23.1.0
D='-D_DARWIN_C_SOURCE -I.'
# ✅ macOS 可编译
$CC $D -Wall -Wextra -o catch_rtsigs        catch_rtsigs.c        get_num.c signal_functions.c
$CC $D -Wall -Wextra -o t_sigsuspend        t_sigsuspend.c        get_num.c signal_functions.c
$CC $D -Wall -Wextra -o demo_SIGFPE         demo_SIGFPE.c         get_num.c signal_functions.c
$CC $D -Wall -Wextra -o sig_speed_sigsuspend sig_speed_sigsuspend.c get_num.c signal_functions.c
$CC $D -Wall -Wextra -c signal.c            # 空目标 = dummy，预期
$CC -Wall -Wextra -o sigsuspend_wait        sigsuspend_wait.c
# ❌ Linux 专有（macOS 编译不过，Pi5 编译）
$CC -D_GNU_SOURCE -Wall -Wextra -o t_sigqueue        t_sigqueue.c        get_num.c signal_functions.c
$CC -D_GNU_SOURCE -Wall -Wextra -o t_sigwaitinfo     t_sigwaitinfo.c     get_num.c signal_functions.c
$CC -D_GNU_SOURCE -Wall -Wextra -o signalfd_sigval   signalfd_sigval.c
$CC -D_GNU_SOURCE -Wall -Wextra -o sigqueue_rt       sigqueue_rt.c
$CC -D_GNU_SOURCE -Wall -Wextra -o sigwaitinfo_loop  sigwaitinfo_loop.c
```

| 程序 | macOS 结果 | 原因 |
|------|-----------|------|
| `t_sigqueue` / `sigqueue_rt` | 编译失败 | macOS **无 `sigqueue()`** |
| `t_sigwaitinfo` / `sigwaitinfo_loop` | 编译失败 | macOS **无 `sigwaitinfo()`**（只有 `sigwait`） |
| `signalfd_sigval` | 编译失败 | macOS **无 `sys/signalfd.h`** |
| 其余 5 个 | 零警告通过 | — |

## 实测输出（macOS 26.6.2 / arm64，原文钉死）

### ① Listing 22-5：sigsuspend 的原子性（22.9 的灵魂输出）

```text
Initial signal mask is:
        <empty signal set>
=== LOOP 1
Starting critical section, signal mask is:
        2 (Interrupt: 2)
        3 (Quit: 3)
Before sigsuspend() - pending signals:
        2 (Interrupt: 2)          ← 临界区（busy 4s）内到达的 SIGINT 变 pending
Caught signal 2 (Interrupt: 2)    ← 精确在 sigsuspend 点被处理，无竞态窗口
=== LOOP 2
...
=== Exited loop
Restored signal mask to:
        <empty signal set>
```

对照自编 `sigsuspend_wait`：

```text
pid=10949  waiting with sigsuspend — press Ctrl+C
got SIGINT safely                             ← exit=0，掩码复原
```

### ② Listing 22-3：SA_SIGINFO + 标准信号合并（macOS 无 RT 段）

```text
catch_rtsigs: PID is 8160
catch_rtsigs: signals blocked - sleeping 1 seconds
catch_rtsigs: sleep complete
caught signal 1                               ← SIGHUP
    si_signo=1, si_code=0 (other), si_value=0   ← macOS SI_USER=0x10001，0 打成 "other"
    si_pid=0, si_uid=0                          ← 阻塞期收到的信号 si_pid 未填充！
caught signal 30                              ← SIGUSR1=30（BSD 编号）
Caught 2 signals                              ← 发了 USR1×2+HUP×1，USR1 合并 → 共 2 次
```

**对照**：Ch21 `siginfo_demo` 运行期直接收到 USR1 时 si_pid=10683/uid=501 正常填充——
macOS 对「阻塞后延迟递送」的信号不回填发送者信息，Linux 会填。

### ③ 官方补充 demo_SIGFPE：⚠️ ARM64 整数除零不触发 SIGFPE

```text
Catching SIGFPE
About to generate SIGFPE
Shouldn't get here!                ← x = 1/y（y=0）直接算出结果继续跑
[exit=1]                           ← handler 根本没进！
```

ARM64 的 `sdiv` 除零不设陷阱（x86 `idiv` 才 trap）——**Apple Silicon 上 22.4 的
SIGFPE 演示必然落空**；书上的输出只能在 x86 Linux 复现。

### ④ 官方补充 sig_speed_sigsuspend：信号是廉价 IPC

```text
$ time ./sig_speed_sigsuspend 2000
real    0m0.040s                   ← 2000 次父子 sigsuspend 往返 ≈ 20µs/次
```

### ⑤ Listing 22-1 signal.c：编译产物为空 = 预期行为

它只在 `__sun`/`__sgi` 下提供实现，其他平台（含 macOS/Linux）是 dummy 源文件。

## Linux 专有语义（源码核验 + Pi5 复测清单）

| 项 | 预期（man-pages/内核 v6.6） | 节 |
|----|------------------------------|----|
| RT 信号排队：`sigqueue` 连发 4 个 SIGRTMIN，caught 4 次 | 标准信号合并 vs RT 排队的直接对照 | 22.8 |
| `t_sigqueue` 的 `si_value` 回读 | `sigqueue(pid, sig, val)` → handler `info->si_value.sival_int` | 22.8 |
| `t_sigwaitinfo`：handler 不执行，信号被同步取走 | 对比 sigsuspend：**不跑 handler** | 22.10 |
| `sigwaitinfo` 错误路径：等待期间收到非等待信号 → `EINTR` 后重入 | | 22.10 |
| `signalfd_sigval`：`read(sfd)` 得 `signalfd_siginfo`；`poll/epoll` 纳管 | buffer < `sizeof` 报 EINVAL | 22.11 |
| `SIGSYS`（seccomp TRAP）→ core dump（22.1） | `dump_seccomp_filter` 类实验 | 22.1 |
| `TASK_UNINTERRUPTIBLE`（D 状态）不吃信号 | `kill -9` 一个 D 状态进程观察 | 22.3 |
| `signal.c` 在 Linux 上编译为空（同 macOS） | | 22.7 |
| 自编三件套 RT 对照实验 | `sigwaitinfo_loop` / `sigqueue_rt` / `sigsuspend_wait` | 22.8–22.10 |

---
