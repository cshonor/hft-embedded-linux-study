# Ch20 code — Signals: Fundamental Concepts

> 实测环境：**macOS 26.6.2 (arm64) + clang 23.1.0**（micromamba `cdev`）。
> 信号是 POSIX 可移植面最宽的机制之一，本章 6 个程序全部在本机**真实编译 + 运行实测**；
> 跨平台差异（信号编号、`si_code` 语义）见下方差异表，Pi5 复测清单见文末。

## 文件清单

### 自编 demo

| 文件 | 覆盖 | 节 |
|------|------|----|
| [`kill_probe.c`](kill_probe.c) | `kill(pid, 0)` 探测进程存在性（存在/EPERM/ESRCH 三态） | 20.6 |
| [`block_pending.c`](block_pending.c) | 阻塞 SIGINT → sigpending 查 pending → 解除递送 | 20.10/20.11 |

### 原书 Listing 逐字镜像（man7 官方 dist `signals/`）

| 文件 | 镜像 | 覆盖 | 节 |
|------|------|------|----|
| [`ouch.c`](ouch.c) | **Listing 20-1** | `signal()` 最简 handler | 20.3 |
| [`intquit.c`](intquit.c) | **Listing 20-2** | `sigaction()` 捕 INT/QUIT | 20.13 |
| [`t_kill.c`](t_kill.c) | **Listing 20-3** | kill 命令行包装（发任意信号） | 20.5 |
| [`signal_functions.c`](signal_functions.c) / [`.h`](signal_functions.h) | **Listing 20-4** | printSigMask / printPendingSigs / printSigset / sigEmptySet | 20.9 |
| [`sig_sender.c`](sig_sender.c) | **Listing 20-6** | 连发 num-sigs 个信号（演示不排队） | 20.12 |
| [`sig_receiver.c`](sig_receiver.c) | **Listing 20-7** | 屏蔽期收信号 → pending → 解除 → 计数 | 20.11/20.12 |

> Listing 20-5 是 `raise()` 的介绍性文字，无对应文件。

### 官方习题解答镜像

| 文件 | 镜像 | 覆盖 | 节 |
|------|------|------|----|
| [`ignore_pending_sig.c`](ignore_pending_sig.c) | **习题 20-2 官方解** | pending 信号被 `SIG_IGN` 抹掉，永远看不到 | 20.11 |
| [`siginterrupt.c`](siginterrupt.c) | **习题 20-4 官方解** | 用 sigaction 实现 `siginterrupt()`（SA_RESTART 开关） | 20.13 |

> 习题编号核验：`github.com/segarciat/TLPI` `ch20-Signals-Fundamental-Concepts/exercises/`（共 4 题：
> 20-1 用 sigaction 改写 Listing 20-7、20-2 SIG_IGN 吞 pending、20-3 SA_RESETHAND/SA_NODEFER、20-4 siginterrupt()）。

### 支撑文件

| 文件 | 说明 |
|------|------|
| [`tlpi_hdr.h`](tlpi_hdr.h) | 官方 `lib/tlpi_hdr.h` 的 macOS 最小替身（errExit/fatal/usageErr/cmdLineErr/errMsg，含 get_num.h） |
| [`get_num.c`](get_num.c) / [`.h`](get_num.h) | **官方 dist 原版**（Listing 3-5/3-6），自含 gnFail，不依赖替身头 |

> ⚠️ dist 的 `signals/` 目录混装了 Ch20/21/22 三章的程序（如 `t_sigsuspend.c`=Listing 22-5、
> `catch_rtsigs.c`=Listing 22-3、`sigmask_longjmp.c`=Listing 21-2）。为保持「每章镜像自己的程序」，
> **非 Ch20 的 12 个文件不放在本章 code/**，等升级 Ch21/22 时再各自镜像；其中几个在本机顺带实测的
> 结论先钉在下方「Ch21/22 预演」一节，不丢。

## 编译（macOS 实测命令，零错误通过）

```bash
CC=clang   # cdev 环境的 clang 23.1.0
D='-D_DARWIN_C_SOURCE -I.'
$CC $D -Wall -Wextra -o ouch       ouch.c            get_num.c signal_functions.c
$CC $D -Wall -Wextra -o intquit    intquit.c         get_num.c signal_functions.c
$CC $D -Wall -Wextra -o t_kill     t_kill.c          get_num.c signal_functions.c
$CC $D -Wall -Wextra -o sig_sender sig_sender.c      get_num.c signal_functions.c
$CC $D -Wall -Wextra -o sig_receiver sig_receiver.c  get_num.c signal_functions.c
$CC $D -Wall -Wextra -o ignore_pending_sig ignore_pending_sig.c
$CC $D -Wall -Wextra -c siginterrupt.c   # 库文件（无 main），习题 20-4
$CC -Wall -Wextra -o kill_probe    kill_probe.c
$CC -Wall -Wextra -o block_pending block_pending.c
```

## 实测输出（macOS 26.6.2 / arm64，原文钉死）

### ① Listing 20-1/20-2/20-3：handler 注册与投递（ouch 经 pty 运行防缓冲丢失）

```text
$ t_kill <ouch的pid> 2 ; t_kill <pid> 2 ; kill -TERM <pid>
Ouch!
1
Ouch!
2
[exit=143]          ← SIGTERM 终止（默认动作），handler 对 TERM 无感

$ intquit：kill -INT; kill -QUIT
Caught SIGINT (1)
Caught SIGQUIT - that's all folks!    ← QUIT handler 里 exit()，第二次 INT 发不进（进程已亡）
```

### ② Listing 20-6 + 20-7：**标准信号不排队**（20.12 的直接证据）

睡眠期（全屏蔽）连发 3×信号10 + 1×信号12，解除后：

```text
sig_receiver: pending signals are:
        10 (Bus error: 10)      ← macOS 上 10=SIGBUS！strsignal 按本机表翻译
        12 (Bad system call: 12)
sig_receiver: signal 10 caught 1 time    ← 发了 3 次，只递送 1 次
sig_receiver: signal 12 caught 1 time
```

**发了 3 次、caught 1 time** —— pending 位图只有一个比特，第 2、3 次连发全部合并进第 1 次。
20.12「信号不排队」不用背，看这行输出就会了。

### ③ 习题 20-2 官方解：SIG_IGN 抹掉 pending（变更处置 → 丢弃）

```text
Setting up handler for SIGINT
BLOCKING SIGINT for 5 seconds
PENDING signals are:
        2 (Interrupt: 2)        ← 阻塞期发的 SIGINT 在 pending
Ignoring SIGINT                 ← 处置改成 SIG_IGN 的瞬间……
PENDING signals are:
        <empty signal set>      ← pending 被抹掉，永远不会再递送
Reestablishing handler for SIGINT
UNBLOCKING SIGINT               ← 之后解除阻塞，什么也不会发生
```

### ④ `kill_probe`（自编）：kill(pid, 0) 三态

```text
./kill_probe $$     → 进程存在（kill 返回 0）
./kill_probe 1      → EPERM（存在但无权——macOS 上对 launchd）
./kill_probe 999999 → ESRCH（不存在）
```

## macOS ↔ Linux 实测差异（本章实测抓到的）

| 项 | Linux（书/man-pages） | macOS 26.6.2 实测 | 影响 |
|----|----------------------|-------------------|------|
| 信号编号 | `SIGUSR1=10, SIGUSR2=12`（System V 系） | `SIGUSR1=30, SIGUSR2=31`（**BSD 系**）；`SIGBUS=10`、`SIGSYS=12` | **跨平台别对信号编号做任何假设**，一律用 `SIGxxx` 宏 |
| `NSIG` | 65（含 RT 段 34–64） | 32，**无实时信号段** | macOS 没有队列化 RT 信号可用 |
| `si_code` for kill() | `SI_USER == 0` | `SI_USER == 0x10001`（`sys/signal.h:324`），kill 发的信号 `si_code=0` 打成 "other" | 判信号来源要判显式宏或负值/特殊值，别拿 Linux 的裸值当万能 |
| `si_pid/si_uid`（SA_SIGINFO） | kill() 发送时填发送者 pid/uid | 实测填 **0** | macOS 上不能靠 si_pid 追发送者 |
| `strsignal(10)` | "User defined signal 1" | "Bus error: 10" | 输出文案随编号表漂移，只可读不可解析 |

## Ch21/22 程序预演实测（文件归各自章，结论先钉在这）

> dist `signals/` 目录里 Ch21/22 的程序在本机顺带编译运行过，结论先记录，文件到对应章再镜像。

| 程序 | 镜像 | macOS 实测结论 |
|------|------|---------------|
| `t_sigsuspend` | Listing 22-5 | **sigsuspend 原子性完美复现**：临界区（INT/QUIT 全屏蔽）内到达的 SIGINT 变 pending，sigsuspend 一解除就递送，`Caught signal 2` 精确落在 sigsuspend 点；SIGQUIT 退出循环后掩码复原 |
| `sigmask_longjmp` | Listing 21-2 | setjmp/longjmp 版：handler 内掩码 `2 (Interrupt)`，跳回后打印空集——非 sigsetjmp 语义（未保存掩码），平台相关，Pi5 上应再对照 |
| `t_sigaltstack` | Listing 21-3 | 递归 83 层撑爆栈 → SIGSEGV，handler 明确跑在备择栈（`0x150037b58` vs 主栈 `0x16a9...`）[exit=1] |
| `demo_SIGFPE` | Ch22 补充 | ⚠️ **ARM64 macOS 整数除零不触发 SIGFPE**：`x = 1/y`（y=0）直接算出结果继续跑，打印 "Shouldn't get here!" 后 [exit=1]，handler 根本没进——ARM64 `sdiv` 除零不设陷阱（x86 除零才 trap）。**书上的 SIGFPE 演示在 Apple Silicon 上必然落空** |
| `sig_speed_sigsuspend` | Ch22 补充 | `time ... 2000`：**0.04s 完成 2000 次父子 sigsuspend 往返**（~20µs/往返，仍属「信号是廉价 IPC」量级） |
| `nonatomic_uint64` | Ch21 补充 | 编译零警告；**实测被沙箱拦**：子进程紧密 `kill(getppid())` 到 ~第 1018 次报 `EPERM`（单发正常；父进程 1017 次成功 kill 只实收 4 次——合并语义反而被间接验证）。撕裂读未复现，Pi5 复测 |
| `catch_rtsigs` | Listing 22-3 | SA_SIGINFO 正常；`si_code=0`→"other"、`si_pid/uid=0`（见差异表） |
| `signal.c` | Listing 22-1 | 编译产物为空——它只对 Solaris/SGI 提供实现，其他平台是 dummy 源文件，**属预期** |
| `nonreentrant`(21-1) / `t_sigqueue`(22-2) / `t_sigwaitinfo`(22-6) / `signalfd_sigval`(22-7) | — | **macOS 编译不过**：缺 `crypt.h` / `sigqueue()` / `sigwaitinfo()` / `sys/signalfd.h`——全是 Linux 专有，属预期 |

## Pi5 复测清单（Linux 侧待验证）

| 项 | 节 | 验证方法 |
|----|----|---------|
| `sig_receiver` 3 连发 caught 1 time（合并复现） | 20.12 | `sig_sender $$ 3 10 12` + `sig_receiver 2` |
| RT 信号 `SIGRTMIN` 4 连发 caught 4 次（**排队**对照） | 20.12 | 需要 sigqueue 版程序（Ch22 镜像后再测） |
| `kill_probe` 对 init(1) 报 EPERM 而非 ESRCH | 20.6 | `./kill_probe 1` |
| `ignore_pending_sig`：pending 被 SIG_IGN 抹掉 | 20.11 | 运行官方解，阻塞期 Ctrl+C |
| `siginterrupt(0/1)` 对 blocking syscall 的重启/打断 | 20.13 | 配合 Ch21 的 read 慢速管道实验 |
| `strsignal` 文案与 `sys_siglist` 对照 | 20.8 | 逐信号打印 |
| `signal()` 与 `sigaction()` 的 SA_RESTART 默认差异 | 20.3 | ouch 风格程序对比 read 被打断与否 |
| `block_pending`：Ctrl+C 后 pending 位图与解除递送时序 | 20.10/20.11 | 运行自编 demo |
| `t_kill 0 SIG`（pid=0 进程组广播） | 20.5 | 会发给同组全部进程，注意范围 |
| `sigmask_longjmp` 掩码语义对照（Linux 是值不确定） | Ch21 预演 | 运行后比对打印 |
| `nonatomic_uint64` 撕裂读复现 | Ch21 预演 | 无沙箱环境下跑 3 秒看 Unexpected 行 |

---
