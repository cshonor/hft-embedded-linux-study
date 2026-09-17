# Ch23 代码索引 — Timers and Sleeping

本目录有 **25 个源文件**（另有本 README 与 `index.html`），分五类：

| 类别 | 数量 | 说明 |
|------|------|------|
| **原书镜像件** | 14 | 逐行取自 man7 官方 Ch23 分发目录 `timers/`，未做任何改写 |
| **本仓库自写** | 4 | 原书**没有**这些程序（§23.3 的阻塞超时、§23.6.6 的 overrun、习题 23-1 / 23-4 的解答） |
| **公共库镜像** | 4 | `get_num.{c,h}` + `curr_time.{c,h}` |
| **替身** | 1 | `tlpi_hdr.h`（代替原书 `lib/tlpi_hdr.h` + `lib/error_functions.c`） |
| **旧 demo** | 2 | 本仓库早期随手写的小例子（`nanosleep_retry.c` / `posix_timer_thread.c`），保留但**不是**原书内容 |

所有程序都在 Compiler Explorer 上实跑过（gcc 13.3.0 / x86-64 / Ubuntu 24.04），笔记里引用的输出与冻结日志逐字一致。

---

## 一、原书镜像件（14 个）

| 文件 | 行数 | 出处 | 对应节 | 备注 |
|------|------|------|--------|------|
| `real_timer.c` | 132 | **Listing 23-1**，page 479–485 | 23.1 | `setitimer()` 最小演示：`ITIMER_REAL` + 每秒一次 `SIGALRM` |
| `timed_read.c` | 68 | **Listing 23-2**，page 487–488 | 23.3 | `alarm()` 给 `read()` 加超时；**刻意不定义 `_POSIX_C_SOURCE`** |
| `t_nanosleep.c` | 74 | **Listing 23-3**，page 490–491 | 23.4 | 带 `EINTR` 重试的 `nanosleep()`；输出用 `curr_time.c` 打时间戳 |
| `ptmr_sigev_signal.c` | 96 | **Listing 23-5**，page 500–502 | 23.6.3 | `timer_create()` + `SIGEV_SIGNAL`；`sival_ptr = &tidlist[j]` 回传 ID |
| `itimerspec_from_str.c` | 56 | **Listing 23-6**，page 502–503 | 23.6.4 | `"sec.usec[:int.usec]"` → `struct itimerspec` |
| `itimerspec_from_str.h` | 22 | 同上 | 23.6.4 | 只声明 `itimerspecFromStr()` |
| `ptmr_sigev_thread.c` | 124 | **Listing 23-7**，page 504–506 | 23.6.7 | `SIGEV_THREAD`；线程 + 互斥量 + 条件变量；需 `-pthread` |
| `demo_timerfd.c` | 78 | **Listing 23-8**，page 510–512 | 23.7 | `timerfd_create()` + `read()` 拿到期次数 |
| `ptmr_null_evp.c` | 66 | 习题 23-3 的解（未印刷） | 23.6.2 | `timer_create(..., NULL)` ⇒ 默认 `SIGEV_SIGNAL` + `SIGALRM` + `sival_int` = 定时器 ID |
| `t_clock_nanosleep.c` | 106 | 习题 23-2 的解（未印刷） | 23.5.4 | `clock_nanosleep()` 的相对 / 绝对两种模式对照 |
| `clock_times.c` | 70 | 未印刷 | 23.5.1 | 打各时钟的 `clock_gettime` 值与分辨率 |
| `cpu_burner.c` | 184 | 未印刷 | 23.9 附 A | 用「空转 + 睡眠」逼近给定 CPU 占用率；`cpulimit` 式工具 |
| `cpu_multi_burner.c` | 144 | 未印刷 | 23.9 附 A | 同上，但 fork 多个子进程 |
| `cpu_multithread_burner.c` | 154 | 未印刷 | 23.9 附 A | 同上，但用线程池（需 `-pthread`） |

> ⭐ **`Listing 23-4` 没有对应文件**。它是 §23.5.4 正文里的一段**内联示例**（*Using `clock_nanosleep()`*），官方分发目录里没有这个 `.c`。想在 man7 的 `timers/` 列表里找它只会徒劳 —— 详见 [`notes/23.9-exercises.md`](../notes/23.9-exercises.md) 附录 B。

> 📌 **两处「印刷版 vs 分发版」差异**（已在笔记里逐条标注，不是本仓库改的）：
> - `itimerspec_from_str.c`：书上直接对**传入的字符串**动刀（`*cptr = '\0'`，会销毁 `argv[]`）；当前分发版先 `strdup()` 在副本上切分，最后 `free()`。**当前版更好**。
> - `demo_timerfd.c`：局部变量声明位置在印刷版与分发版之间有过调整。

官方分发地址形如 `https://man7.org/tlpi/code/online/dist/timers/<name>`。

---

## 二、本仓库自写（4 个，原书没有）

这 4 个文件**都不是原书内容**，文件头里都有显式声明。写它们的原因全部是「原书要演示的现象在无人值守 / 非交互环境里复现不出来」：

| 文件 | 行数 | 补的是哪一节 | 为什么必须自写 |
|------|------|-------------|---------------|
| `c23_alarm_pipe_timeout.c` | 124 | §23.3 | **Listing 23-2 的 `Read timed out` 分支在 CE 上走不到**。CE 的 stdin 是**已关闭的管道** ⇒ `read()` 立刻返回 0（EOF），压根不进阻塞态，`alarm(2)` 的两秒根本没等（实测 `execTime` 21 ms / 30 ms）。本程序用「**写端保持打开**的匿名管道」造出真阻塞，于是同一条 5 步法能跑出两种对照：<br>**对照 A** `sa_flags = 0` ⇒ `read()` 返回 **-1** / `errno = 4 (EINTR)`，handler 被调 1 次<br>**对照 B** `sa_flags = SA_RESTART` ⇒ `read()` 被内核自动重启，**永不返回**（跑到 20234 ms 被 SIGKILL，`exit 143`） |
| `c23_posix_timer_overrun.c` | 144 | §23.6.6 | **原书制造 overrun 靠手工 `Ctrl-Z` / `fg`**（SIGSTOP / SIGCONT 停住进程让定时器欠账），无人值守环境无法复现 ⇒ `ptmr_sigev_signal.c` 跑出来的 `timer_getoverrun()` 全是 0。本程序改用「主动 `SIG_BLOCK` 通知信号」**确定性**制造欠账：1 ms 周期、睡 1 s、`SIG_BLOCK` 住 `SIGRTMAX` ⇒ `si_overrun = 999`、`timer_getoverrun() = 999`；再验证「**收到即重置**」（解除武装后再读一次得 0） |
| `ex23_1_my_alarm.c` | 157 | 习题 23-1 | 用 `setitimer()` 实现 `alarm()`；顺带实测出 `alarm()` 的返回值语义 —— **内核是「向上取整」不是「截断」**（剩余 4.699919 s ⇒ `alarm(0)` 返回 **5**） |
| `ex23_4_ptmr_sigwaitinfo.c` | 149 | 习题 23-4 | 把 Listing 23-5 的**处理器**换成 `sigwaitinfo()`（官方没有解答文件）。额外加了 `-n <N>` 选项让它能退出 —— 原书的 `for (;;) pause();` 没有退出路径。**这个 `-n` 不是原书内容** |

### 编译要点（踩过的坑）

```bash
# 两个自写程序只依赖 libc，不需要 tlpi_hdr.h / get_num.c
gcc -O0 -Wall -Wextra -o c23_alarm_pipe c23_alarm_pipe_timeout.c
gcc -O0 -Wall -Wextra -o c23_overrun    c23_posix_timer_overrun.c

# 习题解答要带替身头
gcc -O0 -Wall -Wextra -I. -o ex23_1 ex23_1_my_alarm.c
gcc -O0 -Wall -Wextra -I. -o ex23_4 ex23_4_ptmr_sigwaitinfo.c
```

> ⚠️ **`c23_alarm_pipe_timeout.c` 刻意不定义 `_POSIX_C_SOURCE`**，与 Listing 23-2（`timed_read.c`）的写法保持一致。原因实测踩过：一旦显式定义 `_POSIX_C_SOURCE 199309`，glibc 就**不再隐式打开 `_DEFAULT_SOURCE`**，`SA_RESTART` 随之变成
> ```
> <source>:96:23: error: 'SA_RESTART' undeclared (first use in this function); did you mean 'ERESTART'?
> ```
> 同一原因还会让 `usleep()` 不声明 ⇒ `c23_posix_timer_overrun.c` 改用 `nanosleep()`。
> 原书 Makefile 之所以能编过 `ptmr_*.c` 那批带 `_POSIX_C_SOURCE` 的文件，是因为 `IMPL_CFLAGS` 里还有 `-D_DEFAULT_SOURCE`。

> ⚠️ **`c23_alarm_pipe_timeout.c` 里调了 `setvbuf(stdout, NULL, _IONBF, 0)`**。那不是程序逻辑的一部分 —— 对照 B 会一直阻塞到被执行器杀掉，而 CE 的 stdout 是 **socket** ⇒ glibc 用 8 KB **全缓冲** ⇒ 有缓冲的话前面已打印的内容会跟着进程一起消失。这是实测环境的事实，不是程序行为。

---

## 三、公共库镜像与旧 demo

### 公共库（4 个）

| 文件 | 行数 | 来源 | 谁在用 |
|------|------|------|--------|
| `get_num.c` | 102 | 原书 `lib/get_num.c` | 解析失败时调 `cmdLineErr()`（替身头里已提供） |
| `get_num.h` | 32 | 原书 `lib/get_num.h` | 声明 `getInt()` / `getLong()` / `GN_*` 常量 |
| `curr_time.c` | 42 | 原书 `time/curr_time.c` | `currTime()` 返回 `"HH:MM:SS"`，`t_nanosleep.c` / `ptmr_*.c` 用 |
| `curr_time.h` | 20 | 原书 `time/curr_time.h` | 声明 `currTime()` |

依赖边界（已逐个 grep 确认）：`real_timer` / `t_nanosleep` / `t_clock_nanosleep` 用 `getLong()`；`timed_read` / `demo_timerfd` 用 `getInt()`。

### 旧 demo（2 个，**不是**原书内容）

| 文件 | 行数 | 说明 |
|------|------|------|
| `nanosleep_retry.c` | 53 | `nanosleep` + `EINTR` 安全重试 —— 内容与 Listing 23-3 重叠，属早期随手写的玩具 |
| `posix_timer_thread.c` | 59 | `timer_create` + `CLOCK_MONOTONIC` + `SIGEV_THREAD` 的 20 行版 —— 是 Listing 23-7 的**简化重复** |

> ⚠️ 这两个文件保留是为了不破坏旧链接，但**笔记里一个新引用都没有**。学 Ch23 请只看 `real_timer.c` / `t_nanosleep.c` / `ptmr_sigev_thread.c` 三个原书版本。

---

## 四、替身：`tlpi_hdr.h`（1 个）

原书结构是「`lib/tlpi_hdr.h` 声明 + `lib/error_functions.c` 实现」两层，本仓库都没有随附。这里为**单文件可控**把实现做成 `static inline` 放在头里，**报文格式逐字对齐官方**。

### 替身差异表（写在这里免得读者对不上输出）

| # | 差异 | 影响 |
|---|------|------|
| 1 | **`ename[]` 表缺失**（原书那张表由 `lib/build_ename.sh` 从 `errno.h` **生成**）。`outputError()` 里 `[%s %s]` 的第一个 `%s` 退化成 `?UNKNOWN?` | ⚠️ 与 Ch14 不同，**本章正常运行路径看不到**：`errExit` / `errExitEN` 只在「`clock_gettime` / `timer_*` / `sigaction` 失败」这类不该发生的分支才走。<br>原书：`ERROR [ENOENT No such file or directory] clock_gettime`<br>替身：`ERROR [?UNKNOWN? No such file or directory] clock_gettime` |
| 2 | **`usageErr` / `cmdLineErr` 的文案与官方逐字相同**（`"Usage: "` / `"Command-line usage error: "`），且本章会真的触发 | `real_timer` / `t_nanosleep` / `t_clock_nanosleep` / `ptmr_*` / `demo_timerfd` 参数不对时都调 `usageErr` —— 已核对一致 |
| 3 | `Boolean` 照抄官方（先 `#undef TRUE/FALSE` 再 `typedef enum { FALSE, TRUE }`） | 无差异 |
| 4 | `outputError()` 里那条会被 gcc 报 `-Wformat-truncation` 的 `snprintf`，**同样**用 `#pragma GCC diagnostic push/ignored/pop` 压掉 | 否则会凭空多出一条**官方代码里没有的**警告 |
| 5 | 提供 `errMsg` / `errExit` / `errExitEN` / `fatal` / `usageErr` / `cmdLineErr` / `terminate` 七个；原书的 `err_exit` 与不带 errno 的 `terminate` **未提供** | 本章无程序用 |
| 6 | 官方头里那几段**平台兼容块**（`socklen_t` / `FASYNC→O_ASYNC` / `MAP_ANON→MAP_ANONYMOUS` / `O_FSYNC→O_SYNC` / `__FreeBSD__` 的 sigval 别名）按需省略 | 这些在 Linux/glibc 上全是**空操作**（已逐条比对官方原件），本章无程序用到 |
| 7 | **不带 `signal_functions.h`**（原书的 `printSigMask` 等） | 本章没有程序用 |
| 8 | `min` / `max` 宏保留（与官方头对齐），但**本章没有程序调用**（`demo_timerfd.c` 用的是变量名 `maxExp`，不是宏） | 无影响 |

> ⚠️ **与 Ch10–Ch22 的同名替身不通用，且报文格式不同**：那一批把 `errExit` 写成 `msg: strerror` 直出（**没有** `ERROR` 前缀、**没有** `[...]` 段）；本 Ch23 份按官方格式输出 `ERROR [...]`。各章笔记引用的报错文本因此不同，**别跨章对照**。

---

## 五、官方编译参数（必须照用，否则会误判「代码有问题」）

原书 `Makefile.inc` 的 `IMPL_CFLAGS`：

```bash
-std=c99 -D_XOPEN_SOURCE=600 -D_DEFAULT_SOURCE \
-pedantic -Wall -W -Wmissing-prototypes -Wimplicit-fallthrough -Wno-unused-parameter
```

本仓库在 CE 上跑的时候用的是等价的 `-O0 -Wall -Wextra -I.`，所以**在 CE 上看到的下面这些警告都不是原书代码的问题**：

| CE 上会看到的警告 | 真实原因 |
|------------------|---------|
| `warning: unused parameter 'argc'` / `'argv'` | 官方 `IMPL_CFLAGS` 里有 `-Wno-unused-parameter`，CE 上没加 |
| `warning: implicit declaration of function 'strdup'` | 官方有 `-D_DEFAULT_SOURCE`，CE 上没加 |

### 链接参数

| 事项 | 结论 |
|------|------|
| `-lrt` | ⭐ **不需要**。glibc **2.34**（2021-08）起 `timer_create` / `timer_settime` / `timerfd_*` / `clock_gettime` / `clock_nanosleep` 全部从 `librt` / `libpthread` 迁进了 `libc`。原书 2010 年出版时的 `-lrt` 在今天的 Ubuntu 24.04 上**纯属多余**（`librt.so` 只剩一个空壳），但也不报错 |
| `-pthread` | ⭐ **需要**，且只有 `ptmr_sigev_thread.c` / `cpu_multithread_burner.c` 需要（用 `SIGEV_THREAD` 与线程） |
| `-I.` | 原书镜像件都要（找替身的 `tlpi_hdr.h`）；两个自写程序不需要 |

> 📌 本仓库笔记里所有 CE 编译诊断的行号（如 `<source>:96:23`）都是按上面这套参数跑出来的，读者按同样参数编译就能对上。

---

## 六、沙箱（Compiler Explorer 容器）的硬限制

本目录的 demo 全部在 CE 上跑过。容器里**做不到**的事，笔记里都做了诚实标注，不要把它们当成「代码写错了」：

| 限制 | 观察到的事实 | 受影响的 demo |
|------|-------------|--------------|
| **每个作业最长 20 秒** | 超时被 SIGKILL，`exit 143`。`ptmr_*` 的 `for (;;) pause();` 没有退出路径 ⇒ **必然**跑到被杀 | `ptmr_sigev_signal.c`、`ptmr_sigev_thread.c`、`ptmr_null_evp.c`、`c23_alarm_pipe_timeout.c` 对照 B、`ex23_4_*` 不带 `-n` |
| **单次执行的 stdout 采集上限约 32 KB** | 超限时 CE 插入 `[Truncated]` 并**停止采集**。证据：`ptmr_null_run` 与 `cpu_burner_cg` 采集到的字节数**完全相同（32782）**，且都在 20 秒**之前**就停了（18.495 s / 13.444 s）；而 `ptmr_thr_run`（20482 B）与 `cpu_burner_run`（20482 B）**没有** `[Truncated]`，是被 20 秒掐断的 | 全部计数类输出 |
| **stdout 是 `SOCKET` 不是 pipe** | `isatty() == 0` 且 `S_ISSOCK` 为真 ⇒ glibc 用 **8 KB 全缓冲**（不是行缓冲）。进程被强杀时**未刷出的内容全丢** ⇒ 无限循环程序如果不把输出频率压高，会看到「一行都没有」 | `ptmr_*`、`cpu_*_burner.c`、`c23_alarm_pipe_timeout.c` |
| **无法向进程发信号** | 不能 `Ctrl-C` / `Ctrl-Z` / `kill -CONT`。原书靠手工 `Ctrl-Z` 造 overrun 的办法失效 | `c23_posix_timer_overrun.c` 就是为了绕开这条 |
| **stdin 是已关闭的管道** | `read(0, ...)` 立刻返回 **0（EOF）**，不进阻塞态；`alarm(2)` 的两秒根本没等 | `timed_read.c`（`Read timed out` 分支走不到） |
| **`argv[0]` 恒为 `./output.s`** | CE 给产物起的名字。`usageErr` 打的是 `argv[0]` ⇒ 用法行显示 `./output.s` | 全部调 `usageErr` 的程序 |
| **CE 每个作业是独立容器，宿主可能不同** | `totalram` 会在 16.5 GB / 8.15 GB 之间跳（`uptime` / `loads` / `procs` 一起变） | 全部 demo：**只在同一次运行内比较** |
| **时钟分辨率随宿主 CPU 变** | 本次冻结日志里：`CLOCK_REALTIME` / `MONOTONIC` / `BOOTTIME` / `TAI` / `MONOTONIC_RAW` / `CPUTIME` = **1 ns**（HRT）；`*_COARSE` 两个 = **1 ms**（`CONFIG_HZ=1000`）。换宿主可能不同 | `clock_times.c` |
| **单次极值类测量会漂** | `min` 正增量、命中次数、`clock_adjtime` 的 NTP 字段，重跑一次就变；而「N 百万次平均」类**一字不变** | 笔记里凡标「本次」的数字 |

> ⚠️ 因此：所有「行数 / 通知次数」必须读作「**日志中可见的**」而不是「程序产生的」。本章笔记已在 §23.6 显式声明这一点。

---

## 七、文件校验（sha256）

```text
894efcda8b484d04c0b8be690fef1eee103e6c54bdc34066d9e6af07ab69b555  c23_alarm_pipe_timeout.c
e9a4e423f2083b2bbc6bfc4bd927327f953720bd0c7b12ee1d24d3d3dc1beb70  c23_posix_timer_overrun.c
ddedf099d6444ef72f5dde1467fa9481be1e20d251586387c7ebc6d054811100  clock_times.c
856e95237d1ff4ac54b49954779e95636ffc478f9c2edb54d9578046e1243c3a  cpu_burner.c
2692e661f135de79bd8d562eac05d0aa04ede3dcc712d17bd4743c01360dbce0  cpu_multi_burner.c
5f6d7740ab88945a2fb16fe3dbc2267f0270d9db3d24c64014e3fcc4cb23e508  cpu_multithread_burner.c
d06bb3fd8e439e33c386c8c67e434e67927c2ce021b5b3b50fc26c0ef645bbcf  curr_time.c
941c78e6894cb15b7f5d937dfad76644103b099e31f710cf16719c12fbdeb3cf  curr_time.h
378609228a2ab1dd9f55a1028d42f0f961a06e01a3c1654307d5c0295c288696  demo_timerfd.c
8473e7ff3bd6312148e6665025f8f2a77b16f48289bc947f7b3cfae91041b822  ex23_1_my_alarm.c
9bd5bd57fd192149307eb44e149af99c2ced84411be76e4cb3ae07b4deb7b43f  ex23_4_ptmr_sigwaitinfo.c
b6318f4d9d7d0416a574602d5fe86fa6c7af4871ec97a1900c24e5cdc4ca6724  get_num.c
e7b28952848dbd611edd8d7ab4440c18b49538ecb033704083905edfc69e8c08  get_num.h
e845da584f000e9432fc47f0278e03d13d9f505bfed1170546acdc9c16e1c259  itimerspec_from_str.c
a55ec406e5d0c24611bc9b7a01b469999aab4844485ee30c0c6cb8ebe54fc6f5  itimerspec_from_str.h
87abb3141122f81ae22a6571b5984358a6965369bb45f169f5cfca665a4d50f9  nanosleep_retry.c
2c8dafb8184206c78e03b12937ae89dce85757db4e8cd627c47d6bf5e9d6508e  posix_timer_thread.c
2351f972c4cc8c23705044d362b2c4b53a62bd8ff513a321570eabe80b82833b  ptmr_null_evp.c
201e232f85bcc5a464e34c0cc87670ee4f08470d3edd11277ff69875ba195233  ptmr_sigev_signal.c
4074fb60995b137a94dcc7857abdc1c79616645c3deec5066e8648bbb637595b  ptmr_sigev_thread.c
23ef86c5a183a11787bf8de4ad0f5cf1f9be7fa1b3b0ba53c70cc8cb70d31c63  real_timer.c
bacb2fe6a6c96b8b3fe13549a6e85ce9dbc5bdbd1a616bc21bf238dd40792672  t_clock_nanosleep.c
4b8f6ce14d921354e70544d6b6b3edc4e7a884ac3fcb42c8c7d6b5085ef4df57  t_nanosleep.c
4486cf8a8ecfecfc7b2c024b525dbc72c88e1f2a3303c792f44e04ed7f303807  timed_read.c
1d1dedf473a0d91e60b03b7583e0fb52dbf82dcf6ed274bef354992e5a186327  tlpi_hdr.h
```

> 笔记里的内联代码与本目录源码**逐行一致**（由 `cmp_note_code.py` 校验），所以笔记里引用的 `gcc` 行号（如 `<source>:96:23`）读者能对上。

---

## 八、怎么用这套代码

```bash
# 1. 原书镜像件（都要 -I. 找替身头；ptmr_sigev_thread 与 cpu_multithread_burner 要 -pthread）
for f in real_timer.c timed_read.c t_nanosleep.c t_clock_nanosleep.c \
         ptmr_sigev_signal.c ptmr_null_evp.c ptmr_sigev_thread.c \
         demo_timerfd.c clock_times.c cpu_burner.c cpu_multi_burner.c \
         cpu_multithread_burner.c; do
    gcc -O0 -Wall -Wextra -I. -o "${f%.c}" "$f" get_num.c curr_time.c \
        $(case "$f" in *thread*|*sigev_thread*) echo -pthread;; esac) \
        || echo "FAILED: $f"
done

# 2. 自写程序（前两个不需要替身头）
gcc -O0 -Wall -Wextra -o c23_alarm_pipe c23_alarm_pipe_timeout.c
gcc -O0 -Wall -Wextra -o c23_overrun    c23_posix_timer_overrun.c
gcc -O0 -Wall -Wextra -I. -o ex23_1 ex23_1_my_alarm.c
gcc -O0 -Wall -Wextra -I. -o ex23_4 ex23_4_ptmr_sigwaitinfo.c

# 3. 按 23.1 → 23.7 的顺序跑
./real_timer 3 1 1              # 3 秒后开始，每秒一次（跑到第 6 次自己退出）
./clock_times                   # 先看清「这台机器有哪些时钟、分辨率多少」
./timed_read                    # ⚠️ 在 CE 上必然是 Successful read (0 bytes)（stdin 立即 EOF）
./c23_alarm_pipe                # 这才是「真阻塞 + 两种 sa_flags 对照」的版本
./t_nanosleep 3 0               # 3 秒，不带时间戳；加 -t 显示时间戳
./t_clock_nanosleep r 3         # 相对模式；a 是绝对模式
./ptmr_sigev_signal 1.5:2.5     2     # 两个定时器（会跑到被 SIGKILL）
./ptmr_sigev_thread 1.5:2.5     2
./ptmr_null_evp 2:0             2    # 习题 23-3：默认 SIGALRM + sival_int
./demo_timerfd 1:0.1            5    # 5 次 100 ms
./c23_overrun                   # 习题 23-6.6 的确定性 overrun 版本：overrun = 999
./ex23_1 5                      # 习题 23-1
./ex23_4 -n 4 1:0.5             2    # 习题 23-4
```

**建议的顺序**：`clock_times` → `real_timer` → `timed_read` → `c23_alarm_pipe` → `t_nanosleep` → `t_clock_nanosleep` → `ptmr_sigev_signal` → `ptmr_null_evp` → `ptmr_sigev_thread` → `c23_overrun` → `demo_timerfd` → `ex23_1` → `ex23_4` → `cpu_*_burner`。

理由：`clock_times` 先建立「这台机器的时间源清单」（1 ns vs 1 ms 两条线，决定了后面所有精度的上限）；`real_timer` → `timed_read`/`c23_alarm_pipe` → `t_nanosleep` 走完「`setitimer` → 阻塞超时 → 固定间隔」的旧 API 链条；`t_clock_nanosleep` 带出 POSIX 时钟；`ptmr_*` 三兄弟覆盖 `SIGEV_SIGNAL`（处理器 / `sigwaitinfo`）/ 默认语义 / `SIGEV_THREAD` 三条通知路径；`c23_overrun` 补上原书靠手工 `Ctrl-Z` 才能演示的欠账问题；`demo_timerfd` 是最现代的一条路（把定时器变成 fd）；习题最后做。

---

## 九、与笔记的对应

| 节 | 笔记 | 程序 |
|----|------|------|
| 23.1 / 23.2 | [`notes/23.1-interval-timers.md`](../notes/23.1-interval-timers.md)、[`notes/23.2-scheduling-and-accuracy-of-timers.md`](../notes/23.2-scheduling-and-accuracy-of-timers.md) | `real_timer.c` + `clock_times.c` |
| 23.3 | [`notes/23.3-setting-timeouts-on-blocking-operations.md`](../notes/23.3-setting-timeouts-on-blocking-operations.md) | `timed_read.c` + `c23_alarm_pipe_timeout.c` |
| 23.4 | [`notes/23.4-suspending-execution-for-a-fixed-interva.md`](../notes/23.4-suspending-execution-for-a-fixed-interva.md) | `t_nanosleep.c` + `curr_time.{c,h}` |
| 23.5 | [`notes/23.5-posix-clocks.md`](../notes/23.5-posix-clocks.md) | `clock_times.c` + `t_clock_nanosleep.c` |
| 23.6 | [`notes/23.6-posix-interval-timers.md`](../notes/23.6-posix-interval-timers.md) | `ptmr_sigev_signal.c` + `ptmr_null_evp.c` + `itimerspec_from_str.{c,h}` + `ptmr_sigev_thread.c` + `c23_posix_timer_overrun.c` |
| 23.7 | [`notes/23.7-timers-that-notify-via-file-descriptors-.md`](../notes/23.7-timers-that-notify-via-file-descriptors-.md) | `demo_timerfd.c` |
| 23.8 | [`notes/23.8-summary.md`](../notes/23.8-summary.md) | 总账（无专属程序） |
| 23.9 | [`notes/23.9-exercises.md`](../notes/23.9-exercises.md) | `ex23_1_my_alarm.c` + `ex23_4_ptmr_sigwaitinfo.c`；附录 A 用 `cpu_*.c`，附录 B 说明 `Listing 23-4` 无文件 |
