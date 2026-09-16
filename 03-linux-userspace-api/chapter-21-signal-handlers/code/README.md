# Ch21 code — Signals: Signal Handlers

> 实测环境：**macOS 26.6.2 (arm64) + clang 23.1.0**（micromamba `cdev`）。
> 本章 3 个原书 Listing 镜像 + 4 个自编 demo + 1 个习题实现，其中可移植部分全部**真实编译运行**。

## 文件清单

### 原书 Listing 逐字镜像（man7 官方 dist `signals/`）

| 文件 | 镜像 | 覆盖 | 节 |
|------|------|------|----|
| [`nonreentrant.c`](nonreentrant.c) | **Listing 21-1** | handler 内调非可重入函数（`crypt()`）的后果 | 21.1.2 |
| [`sigmask_longjmp.c`](sigmask_longjmp.c) | **Listing 21-2** | 从 handler 里 `[sig]longjmp`（掩码语义） | 21.2.1 |
| [`t_sigaltstack.c`](t_sigaltstack.c) | **Listing 21-3** | `sigaltstack()` 备择栈上处理 SIGSEGV | 21.3 |
| [`nonatomic_uint64.c`](nonatomic_uint64.c) | 官方补充 | handler 撕裂写 u64（`sig_atomic_t` 不够宽） | 21.1.3 |

> `sigmask_longjmp.c` 的 `printSigMask()` 来自 **Listing 20-4** 的
> [`signal_functions.{c,h}`](signal_functions.h)（跨章依赖，随本目录镜像）。

### 自编 demo

| 文件 | 覆盖 | 节 |
|------|------|----|
| [`flag_handler.c`](flag_handler.c) | `volatile sig_atomic_t` 置旗范式 + handler 期 `sa_mask` 额外屏蔽 | 21.1 |
| [`eintr_read.c`](eintr_read.c) | 无 `SA_RESTART` 时阻塞 `read` 被 SIGINT 打断 → `EINTR` | 21.5 |
| [`siginfo_demo.c`](siginfo_demo.c) | `SA_SIGINFO` 打印发送者 pid/uid | 21.4 |
| [`sigchld_reap.c`](sigchld_reap.c) | SIGCHLD handler 内 `waitpid(WNOHANG)` 循环（**标准信号不排队**的工程后果） | 21.1 |

### 习题实现

| 文件 | 镜像 | 覆盖 | 节 |
|------|------|------|----|
| [`ex21_1_abort.c`](ex21_1_abort.c) | 习题 21-1 | 自实现 `abort()`：SUSv3 的「override 阻塞/忽略 + handler 返回则复位 SIG_DFL + flush stdio」 | 21.2.2 |

> 习题编号核验：`github.com/segarciat/TLPI` `ch21-Signals-Signal-Handlers/exercises/`（仅 21-1 abort）。

### 支撑文件

| 文件 | 说明 |
|------|------|
| [`tlpi_hdr.h`](tlpi_hdr.h) | 官方头的 macOS 最小替身（与 Ch18/19/20 同构，含 get_num.h） |
| [`get_num.c`](get_num.c) / [`.h`](get_num.h) | 官方 dist 原版（Listing 3-5/3-6） |

## 编译（macOS 实测命令）

```bash
CC=clang   # cdev 环境的 clang 23.1.0
D='-D_DARWIN_C_SOURCE -I.'
$CC $D -Wall -Wextra -o sigmask_longjmp  sigmask_longjmp.c  get_num.c signal_functions.c
$CC $D -Wall -Wextra -o t_sigaltstack    t_sigaltstack.c    get_num.c signal_functions.c
$CC $D -Wall -Wextra -o nonatomic_uint64 nonatomic_uint64.c get_num.c signal_functions.c
$CC -Wall -Wextra -o flag_handler   flag_handler.c
$CC -Wall -Wextra -o eintr_read     eintr_read.c
$CC -Wall -Wextra -o siginfo_demo   siginfo_demo.c
$CC -Wall -Wextra -o sigchld_reap   sigchld_reap.c
$CC -Wall -Wextra -o ex21_1_abort   ex21_1_abort.c
# nonreentrant.c：macOS 无 <crypt.h>，Linux 专有，Pi5 上编译
#   cc -D_GNU_SOURCE -I. -o nonreentrant nonreentrant.c get_num.c signal_functions.c -lcrypt
```

## 实测输出（macOS 26.6.2 / arm64，原文钉死）

### ① Listing 21-2：handler 里 longjmp（setjmp 版，掩码语义平台相关）

```text
Signal mask at startup:
        <empty signal set>
Calling setjmp()
Received signal 2 (Interrupt: 2), signal mask is:
        2 (Interrupt: 2)          ← handler 执行期 SIGINT 自动入掩码
After jump from handler, signal mask is:
        <empty signal set>        ← 本平台 longjmp 后掩码恢复为空（macOS 行为；
                                     POSIX 说此值在 setjmp 版未定义，Linux 上常保持阻塞！）
```

**这正是 21.2.1 的核心考点**：要用 `sigsetjmp(env, 1)` 显式保存掩码，别赌平台行为。

### ② Listing 21-3：备择栈上处理 SIGSEGV

```text
Top of standard stack is near 0x16b163c14
Alternate stack is at         0x150018000-0x104ea3fff
Call    1 - top of stack near 0x16b14b52c
...（递归 83 层撑爆 8MB 主线程栈）
Call   83 - top of stack near 0x16a97816c
Caught signal 11 (Segmentation fault: 11)
Top of handler stack near     0x150037b58    ← handler 明确跑在 sigaltstack 的区间！
```

### ③ 自编 `eintr_read`：无 SA_RESTART 的 EINTR（FIFO 喂 stdin 自动化）

```text
blocking read(STDIN)... press Ctrl+C
read interrupted: EINTR (got_flag=1)     ← read 返回 -1/EINTR，handler 的旗也置上了
```

### ④ 自编 `flag_handler`：sig_atomic_t 范式

```text
pid=10686  press Ctrl+C (exits after 3 deliveries)
main: saw SIGINT #1
main: saw SIGINT #2
main: saw SIGINT #3                        ← handler 只置旗，main 循环消费，exit=0
```

### ⑤ 自编 `sigchld_reap`：SIGCHLD 不排队 → handler 内 while 循环

```text
spawned 3 children; waiting for reaps...
all children reaped (no zombies)           ← while(waitpid(-1,...,WNOHANG))>0 是唯一正确写法
```

### ⑥ 自编 `siginfo_demo` + 习题 21-1

```text
SIGUSR1 from pid=10683 uid=501             ← macOS SA_SIGINFO 的 si_pid/uid 正常填充
                                              （对照：Ch20 catch_rtsigs 阻塞期收到的信号 si_pid=0）

$ ex21_1_abort        → [exit=134]  ⓪ 默认 SIGABRT 终止
$ ex21_1_abort b      → [exit=134]  ① 阻塞也拦不住（SUSv3 override），且终止前 flush 了 stdio
$ ex21_1_abort h      → [exit=134]  ② handler 捕获返回 → 复位 SIG_DFL → 第二次 raise 必死
```

## macOS ↔ Linux 差异与未实测项

| 项 | 状态 |
|----|------|
| `nonreentrant`（21-1） | macOS 无 `<crypt.h>` 编译不过；**Linux 专有**，Pi5 复测（预期：第二次进 handler 时 crypt 的静态缓冲已被上次调用覆盖） |
| `nonatomic_uint64` | 编译零警告；实测被沙箱拦（fork 子进程紧密 kill ~#1018 次报 EPERM，见 Ch20 code/README 预演）。撕裂读未复现，Pi5 复测 |
| `sigmask_longjmp` 掩码语义 | macOS longjmp 后掩码为空；**Linux 上 setjmp 版常表现为掩码保持阻塞**——同一份代码两个结果，Pi5 对照 |
| core dump（21-2.2 / 22.1） | macOS 默认不落盘（`/cores` 且 sysctl 限制）；Linux 上 `ulimit -c` 控制，Pi5 验证 |

## Pi5 复测清单

| 项 | 节 | 验证方法 |
|----|----|---------|
| `nonreentrant`：crypt 静态缓冲被覆盖的乱码 | 21.1.2 | 跑两次连发信号看第二段密文 |
| `sigmask_longjmp`：Linux 的掩码表现 vs macOS 空集 | 21.2.1 | setjmp 版与 `-DUSE_SIGSETJMP` 版各跑一次 |
| `nonatomic_uint64`：撕裂读 `Unexpected: xxxxxxxx` | 21.1.3 | 无沙箱直跑 3 秒 |
| `eintr_read`：终端 Ctrl+C 与 FIFO 情形一致 | 21.5 | 前台直跑 |
| `abort()` 真核文件（`ulimit -c unlimited` 后 `Aborted (core dumped)`） | 21.2.2 | `ex21_1_abort ⓪` + 查 `/proc/sys/kernel/core_pattern` |
| `sigaltstack` 递归层数与栈大小关系（8MB vs `ulimit -s`） | 21.3 | 改小栈限额再跑 |
| SIGCHLD 多子同亡只投递一次的合并窗口 | 21.1 | sigchld_reap 加 sleep 错峰对照 |

---
