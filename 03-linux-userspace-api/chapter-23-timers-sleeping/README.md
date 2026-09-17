# TLPI 第 23 章 — Timers and Sleeping

**优先级**：🔴（超时、周期任务、与信号/`EINTR` 交互、Linux 定时器全家族）
**前置**：[Ch22 Signals: Advanced](../chapter-22-signals-advanced/README.md)（realtime 信号、`si_value`、`sigwaitinfo`） · [Ch21 Signal Handlers](../chapter-21-signal-handlers/README.md)（`SA_SIGINFO`、`SA_RESTART`、async-signal-safe） · [Ch10 Time](../chapter-10-time/README.md)（§10.6 软件时钟 / jiffy）
**后置**：[Ch24 Process Creation](../chapter-24-process-creation/README.md)（`fork`/`exec` 对定时器的继承规则） · [Ch63 Alternative I/O Models](../chapter-63-alternative-i-o-models/notes/63.4-the-epoll-api.md)（`timerfd` 的真正威力；⚠️ 该章目前**还没有 README**，所以这里链到它现存的一篇笔记）

---

有 9 节（与官方 TOC 一致，编号 23.1–23.9）。其中 **23.4 带 23.4.1/23.4.2、23.5 带 23.5.1~23.5.4、23.6 带 23.6.1~23.6.7**；按仓库约定，**同一节子节收在同一篇里用 `###`**，不拆成多文件。

> ⚠️ **`Listing 23-4` 没有对应的官方文件** —— 它是 §23.5.4 正文里的一段内联代码片段（*Using `clock_nanosleep()`*）。本章 8 个 Listing 里 **7 个有文件、只有 23-4 没有**。详见 [23.9 附录 B](notes/23.9-exercises.md)。
>
> ⚠️ **官方 Ch23 分发目录有 14 个文件（13 个 `.c` + `itimerspec_from_str.h`），书里只印了 8 个 Listing** —— 差额来自「2 个习题解答 + 4 个未印刷程序」。完整对账表见下表与 [23.8](notes/23.8-summary.md)。
>
> ⚠️ **`ptmr_sigev_signal.c` / `ptmr_sigev_thread.c` 不认 `--help`** —— 它们的用法分支只判 `argc < 2`，`--help` 会被 `itimerspecFromStr()` 当成合法字符串（`atoi("--help") == 0`）⇒ 创建一个 0 时长定时器后无限 `pause()`。要拿用法报文必须**真的不给参数**。

## 小节目录

- [23.1 Interval Timers 间隔定时器](notes/23.1-interval-timers.md)（`setitimer`/`alarm`、三种 `which`、返回值口径）
- [23.2 Scheduling and Accuracy of Timers 调度与精度](notes/23.2-scheduling-and-accuracy-of-timers.md)（到期 ≠ 被调度、不累积漂移、jiffy、HRT）
- [23.3 Setting Timeouts on Blocking Operations 给阻塞操作设超时](notes/23.3-setting-timeouts-on-blocking-operations.md)（五步法、`SA_RESTART` 的破坏性）
- [23.4 Suspending Execution for a Fixed Interval 睡固定时长](notes/23.4-suspending-execution-for-a-fixed-interva.md)（含 23.4.1 `sleep()` / 23.4.2 `nanosleep()`）
- [23.5 POSIX Clocks POSIX 时钟](notes/23.5-posix-clocks.md)（含 23.5.1~23.5.4：`clock_gettime`/`clock_getres`/`clock_settime`/`clock_nanosleep`）
- [23.6 POSIX Interval Timers POSIX 间隔定时器](notes/23.6-posix-interval-timers.md)（含 23.6.1~23.6.7：创建/武装/查询/删除/信号通知/overrun/线程通知）
- [23.7 Timers That Notify via File Descriptors: the timerfd API](notes/23.7-timers-that-notify-via-file-descriptors-.md)（把定时器变成 fd）
- [23.8 Summary 小结](notes/23.8-summary.md)
- [23.9 Exercises 习题](notes/23.9-exercises.md)（四道题 + 本仓库自写的 23-1/23-4 解答 + burner 附录）

---

## 章节目标

- **「到期」与「执行」是两件事** —— 定时器保证**到期时刻**（且**不累积漂移**），但**不保证**你的代码立刻跑。实测：`real_timer 1 0 1 0` 的 `ALARM:` 落在 `1.49 → 2.00 → 3.00`（按绝对时刻表），而 `cpu_multi_burner` 的输出里会出现 `delta: 0.32 / 0.67 / 0.78` 这类被抢占的证据
- **jiffy 与 HRT 可以同时存在** —— 实测同一台机器：`clock_getres(CLOCK_REALTIME)` = **1 ns**（HRT 已开），而 `clock_getres(CLOCK_MONOTONIC_COARSE)` = **1 ms**（**COARSE 的分辨率就是 jiffy** ⇒ `CONFIG_HZ=1000`）。两者差 6 个数量级
- **睡眠只会晚、不会早，超出量是几十微秒** —— 实测 `nanosleep(1s)` → `1.000071`（+71 µs）、`nanosleep(1.5s)` → `1.500096`（+96 µs）、`clock_nanosleep(1s)` → `1.000071` / `1.000051`
- **`SA_RESTART` 会彻底废掉「用定时器给阻塞调用加超时」这套办法** —— 实测：`sa_flags = 0` 时 `read()` 返回 `-1`/`EINTR`；装了 `SA_RESTART` 后 `read()` 被自动重启，**永远不返回**（跑到 20 秒被 SIGKILL）
- **`alarm()` 的返回值是向上取整，不是截断** —— 实测真实剩余 `4.699919` 秒 ⇒ `alarm(0)` 返回 **5**，而「裸 `setitimer` + 取 `tv_sec`」返回 **4**。这就是习题 23-1 的真正考点
- **`timer_t` 是不透明句柄，数值随通知机制而变** —— 实测 `SIGEV_SIGNAL` 路径下是 `(nil)` / `0x1` / `0x2`（内核小整数 ID），`SIGEV_THREAD` 路径下是 `0x8000000005c81238`（等间距 0x30 = 48 字节，glibc 内部结构地址）。**`Timer ID: 0` 是完全合法的第一个定时器**
- **overrun 可以被确定性制造出来** —— 实测：`SIG_BLOCK` 通知信号 + 1 ms 周期定时器 + 睡 1 秒 ⇒ `si_overrun = 999`、`timer_getoverrun() = 999`，收到信号后再读为 **0**（证明「收到即重置」）
- **`timerfd` 的 `read()` 读到的是「到期次数」，不是数据** —— 实测 1 µs 周期下一次 `read()` 读到 **47** 次到期；每轮 `read → clock_gettime → printf` 约 **6~7 µs** ⇒ 定时器能表达的精度 ≠ 代码能响应的精度
- **两套取整规则方向相反** —— `clock_settime()` **向下**（尽量接近），定时器类 **向上**（至少等这么久）；`alarm()` 的返回值也 **向上**（内核注释：*we'd better return too much than too little*）
- **`fork`/`exec` 对三类定时器的语义都不同** —— `setitimer`：不继承、**跨 `exec` 保留**；POSIX 定时器：不继承、**`exec` 时被清除**；`timerfd`：**fd 继承且 `exec` 后保留**（除 `TFD_CLOEXEC`）
- **诚实**：本章所有实测都跑在 **CE 公开 API（gcc 13.3.0 / x86-64 / Ubuntu 24.04）**，不是本机。笔记明确标注了 CE 环境的硬边界：**20 秒 SIGKILL**、**约 32 KB 输出采集上限**（超限插 `[Truncated]`）、**stdout 是 socket ⇒ glibc 8 KB 全缓冲**（被杀时未刷出的输出全丢）、**无法向被测进程发信号**、**`argv[0]` 恒为 `./output.s`**。凡受这些边界影响的数字，一律标注为「日志中可见的」而不是「程序产生的」
- **溯源**：本章 **9 篇**笔记里的每一行实测输出都能在 CE 冻结日志 `tlpi-ch23-final*.txt` 里定位（35 个作业）

### 一条主线：本章只有「一个演进线」和它每一步被逼出来的原因

| 阶段 | 接口 | 它解决了什么 | 它留下的问题 |
|------|------|-------------|-------------|
| 一 | `alarm()` / `setitimer()` | 最朴素的定时 | 每类只能一个、信号不可换、丢失 overrun、只到微秒 |
| 二 | `sleep()` / `nanosleep()` | 只要「睡一会儿」 | 只解决「睡」，`nanosleep` 的**相对**语义在信号密集时会累积漂移甚至永不醒 |
| 三 | POSIX 时钟 + `clock_nanosleep(TIMER_ABSTIME)` | **绝对时刻**睡眠 ⇒ 漂移不累积；可选择时钟（`MONOTONIC`/`TAI`/`RAW`） | 还是没有「多个定时器 + 同步处理」 |
| 四 | POSIX 定时器（`timer_create`） | 多实例、可选通知方式（信号/线程）、overrun 计数、纳秒 | 通知是**异步**的（信号处理器限制多）或**有线程开销**；overrun 必须「收到后立刻读」 |
| 五 | **`timerfd`** | 定时器变成 **fd** ⇒ 可 `epoll`、`read()` 直接拿计数、代码在**同步上下文** | Linux 专有（≥ 2.6.25）；`fork` 后父子共享同一对象 |

一句话：**本章没有新概念，只有「同一个需求有五个层次的实现，每层都比上一层少一个坑」这个事实。**

---

## 原书示例清单（man7 官方按章文件列表）

man7.org 在 Ch23 下分发 **14 个文件**（13 个 `.c` + `itimerspec_from_str.h`，全部在 `timers/` 目录），但书里只印了 **8 个 Listing**。差额的性质必须分清：

| 官方文件 | 原书 Listing | 本仓库位置 | 性质 |
|---------|-------------|-----------|------|
| `real_timer.c` | Listing 23-1 | `code/real_timer.c` | 正文程序（`setitimer`） |
| `timed_read.c` | Listing 23-2 | `code/timed_read.c` | 正文程序（给阻塞调用加超时） |
| `t_nanosleep.c` | Listing 23-3 | `code/t_nanosleep.c` | 正文程序（`nanosleep` + `remain` 续睡） |
| **（无文件）** | Listing 23-4 | —— | ⭐ **不是文件** —— §23.5.4 正文里的内联代码片段 |
| `ptmr_sigev_signal.c` | Listing 23-5 | `code/ptmr_sigev_signal.c` | 正文程序（信号通知） |
| `itimerspec_from_str.c` + `.h` | Listing 23-6 | `code/itimerspec_from_str.{c,h}` | 正文程序（参数解析，**2 个文件**） |
| `ptmr_sigev_thread.c` | Listing 23-7 | `code/ptmr_sigev_thread.c` | 正文程序（线程通知） |
| `demo_timerfd.c` | Listing 23-8 | `code/demo_timerfd.c` | 正文程序（`timerfd`） |
| `t_clock_nanosleep.c` | —— | `code/t_clock_nanosleep.c` | **习题 23-2 的官方解答** |
| `ptmr_null_evp.c` | —— | `code/ptmr_null_evp.c` | **习题 23-3 的官方解答** |
| `clock_times.c` | —— | `code/clock_times.c` | **未印刷**：显示 4 种时钟 + 分辨率（文件头自称 *for time namespaces*） |
| `cpu_burner.c` | —— | `code/cpu_burner.c` | **未印刷**：CPU 占用率测量（造调度竞争） |
| `cpu_multi_burner.c` | —— | `code/cpu_multi_burner.c` | **未印刷**：多进程版 |
| `cpu_multithread_burner.c` | —— | `code/cpu_multithread_burner.c` | **未印刷**：多线程版 |

> ⚠️ 这批文件用到 TLPI 的公共头 `lib/tlpi_hdr.h`（及其依赖 `get_num.c` / `curr_time.c` / `itimespec_from_str.h`）。本仓库用一个**替身** `code/tlpi_hdr.h` 代替，改动与差异逐条列在 [`code/README.md`](code/README.md) 的「替身差异」表里。
>
> ⚠️ **原书编译参数不是 `-Wall -Wextra`** —— 官方 `Makefile.inc` 是
> `-std=c99 -D_XOPEN_SOURCE=600 -D_DEFAULT_SOURCE -g -I${TLPI_INCL_DIR} -pedantic -Wall -W -Wmissing-prototypes -Wimplicit-fallthrough -Wno-unused-parameter`。
> 本仓库在 CE 上用 `-O0 -Wall -Wextra` 跑出的 `unused parameter` / `strdup` 隐式声明之类的警告**都不是原书代码的问题**，是编译参数差异 ⇒ **本仓库的笔记里不把它们当「发现」写**。
>
> ⚠️ **`usageErr()` / `errExit()` 的输出全在 stderr**。笔记里引用它们时，代码块的内容取自 **stderr**，用 `2>/dev/null` 什么都看不到。

---

## 易错清单

1. **以为「定时器到期 = 我的代码立刻执行」** —— 到期只是把信号标记 pending / 把 fd 变可读，真正跑起来要等调度，延迟**无上界**（§23.2）
2. **用「处理器被调用的时刻」当业务时间戳** —— 那时已经晚了不可知的一段时间
3. **以为周期定时器会漂移，于是每轮手动重算间隔** —— `setitimer`/POSIX 定时器的周期**按上次到期点**推算，不漂移；**自己 `nanosleep` 循环才漂移**
4. **用 `ITIMER_VIRTUAL`/`ITIMER_PROF` 做超时** —— 它们走 **CPU 时间**，进程被换下 CPU 就暂停 ⇒ **永远等不到超时**
5. **以为每类 `which` 可以有多个定时器** —— **每进程每类只有一个**，第二次调用是**覆盖**
6. **在一个进程里混用 `alarm()` / `sleep()` / `setitimer(ITIMER_REAL)`** —— 三者**共享同一个** `ITIMER_REAL`，互相覆盖
7. **以为 `it_interval` 置 0 是「撤销」** —— 撤销要靠把 **`it_value`** 置 0
8. **按直觉写 `{.it_value=..., .it_interval=...}`** —— `struct itimerval`/`itimerspec` 都是 **`it_interval` 在前**；用指定初始化器就不会错
9. **拿了 `setitimer()` 的返回秒数当「精确剩余」** —— 那是 `tv_sec` **截断**值；`alarm()` 才是**向上取整**（实测差 1）；要精确剩余调 `getitimer()`
10. **装了 `SA_RESTART` 还指望「定时器打断阻塞调用」生效** —— 它会让系统调用**自动重启**，超时形同不存在（实测：`read()` 永不返回）
11. **忘了第 4 步 `alarm(0)`** —— 残留定时器会在之后**某段无关代码**里打断别的系统调用
12. **在 `alarm(0)` 之前用 `errno`** —— `alarm()` 自己可能改 `errno`；必须先 `savedErrno = errno`
13. **把 `read()` 返回 0 当「超时」** —— 返回 0 是 **EOF**；超时是返回 `-1` + `EINTR`
14. **在非交互环境（CI / 重定向 / CE）期望看到 `Read timed out`** —— stdin 若已到 EOF，`read` 立刻返回 0，走不到超时分支
15. **以为 `sleep()` 不与 `alarm()` 冲突** —— Linux 上没事（`sleep()` 基于 `nanosleep()`），但 **SUSv3 未规定**，老 libc 上就是 `alarm()` + `SIGALRM`，必然互踩
16. **用 `nanosleep()` 的 `remain` 循环续睡** —— 每轮向上取整到 jiffy ⇒ 信号密时**永远睡不完**；用 `clock_nanosleep(TIMER_ABSTIME)`
17. **`request.tv_nsec` 写 ≥ 1000000000** —— 内核**不会**帮你进位，直接 `EINVAL`
18. **以为 `nanosleep()` 不会被打断** —— 会被信号打断，返回 `-1`/`EINTR`
19. **把 `clock_settime()` 和定时器的取整方向记成一样的** —— 前者**向下**、后者**向上**
20. **`clock_nanosleep()` 写 `if (s == -1)`** —— 它**返回正数错误码**，不是 `-1` + `errno`；写 `if (s != 0)`
21. **`TIMER_ABSTIME` 时还给 `remain` 传指针并读它** —— 绝对模式下 `remain` **完全无用**，传 `NULL`
22. **用 `CLOCK_REALTIME` 做「过了多久」的测量或周期任务** —— 一次 NTP 阶跃/闰秒就错位；用 `CLOCK_MONOTONIC`
23. **以为 `CLOCK_MONOTONIC` 完全不被调整** —— 它会被 NTP **slew**；要完全不被调的是 `CLOCK_MONOTONIC_RAW`
24. **用 `CLOCK_MONOTONIC` 存盘当时间戳** —— 重启归零，跨机不可比
25. **以为能 `clock_settime()` 改任意时钟** —— **只有 `CLOCK_REALTIME` 可设**，且需 `CAP_SYS_TIME`
26. **把 `clock_getres()` 当「精度承诺」** —— 它是**下界**；实测 `clock_nanosleep(1s)` 实际 +51~71 µs
27. **用 `CLOCK_*_COARSE` 做精确定时** —— 分辨率就是 **jiffy**（实测 1 ms）
28. **写 `-lrt`** —— glibc ≥ 2.17 已并入 libc；现代工具链不需要（老交叉工具链仍要）
29. **把 `timer_t` 当整数比较/打印/存盘** —— 它是**不透明句柄**（`SIGEV_SIGNAL` 下小整数、`SIGEV_THREAD` 下是 glibc 内部结构地址）
30. **以为 `timer_t == 0` 是「创建失败」** —— `Timer ID: 0` 是**合法**的第一个定时器；失败看 `timer_create()` 返回 `-1`
31. **以为 POSIX 定时器可以无限创建** —— 内核**每创建一个就预分配一个 realtime 信号结构** ⇒ 受信号队列上限约束
32. **`timer_gettime()` 的 `it_value` 当绝对时刻用** —— 它返回的是**距下次到期的剩余**，即使当初用 `TIMER_ABSTIME` 武装
33. **忘了 `SA_SIGINFO`** —— 处理器拿不到 `siginfo_t`，无法分辨是哪个定时器/读 `si_value`
34. **多个定时器共用一个信号，却用 `sival_int` 传 ID** —— 应该用 `sival_ptr = &tidlist[j]` 传**地址**（`sival_int` 装 ID 只在 `evp == NULL` 默认语义下成立）
35. **以为 realtime 信号能排队表达多次到期** —— POSIX.1b **明确不做**：*multiple instances of the signal are never queued, even if we use a realtime signal* ⇒ 必须读 overrun
36. **忽略 overrun** —— 定时器实际到期 1000 次，代码以为是 1 次（实测 `overrun = 999` 的正例）
37. **在信号处理器里调 `printf()`/`malloc()`** —— 不是 async-signal-safe（`timer_getoverrun()` **是**）；要同步处理改用 `sigwaitinfo()`
38. **高频定时器用 `SIGEV_THREAD`** —— glibc 会给每个定时器建辅助线程 ⇒ 开销远超信号
39. **用 `atoi()`/`sscanf()` 解析参数却不校验** —— `--help` 被当 0；本页实测 `ptmr_*` 就因此挂住
40. **靠 `fork()` 后子进程里的 POSIX 定时器** —— **不继承**，子进程一个都没有
41. **靠 `exec()` 后 POSIX 定时器还在** —— `exec` 时**被解除武装并删除**（对比 `setitimer` 会保留）
42. **忘了 `-pthread` 编 `SIGEV_THREAD` 程序** —— 链接可能过但运行时崩
43. **只 `close(timerfd)` 就以为定时器没了** —— fd 是引用计数的；`fork`/`dup` 后要**所有**引用都关掉才释放
44. **给 `read(timerfd)` 的缓冲小于 8 字节** —— 必须能装下 `uint64_t`
45. **以为 `read(timerfd)` 返回「距下次到期还有多久」** —— 返回的是**上次读之后的到期次数**
46. **`timerfd` 的绝对模式写成 `TIMER_ABSTIME`** —— 宏名是 **`TFD_TIMER_ABSTIME`**
47. **`timerfd_create()` 填 `CLOCK_PROCESS_CPUTIME_ID`** —— 不支持，只有 `CLOCK_REALTIME` / `CLOCK_MONOTONIC`
48. **假设 `timerfd` 的 `read()` 每次都是 1** —— 计数要**累加**（实测一次读到 **47**）
49. **`fork` 后父子都去 `read()` 同一个 `timerfd`** —— 谁读谁清零 ⇒ **明确只有一个读者**
50. **`exec` 后忘了新程序会继承 `timerfd`** —— 不想要就加 `TFD_CLOEXEC`
51. **期望 `timerfd` 跨平台** —— Linux 专有（≥ 2.6.25）
52. **以为「定时器 1 µs 一次」响应就是 1 µs** —— 实测每轮 `read → clock_gettime → printf` 要 6~7 µs

---

## 章节链路

```text
                    需求：到点做事
                          │
        ┌─────────────────┴─────────────────┐
        │                                   │
   「睡一会儿」                        「定时通知我」
        │                                   │
        ▼                                   ▼
  ┌───────────────┐            ┌────────────────────────────┐
  │ sleep()  23.4 │            │ alarm()/setitimer()  23.1  │
  │ nanosleep 23.4│            │  每类只能一个 / 信号不可换  │
  └───────┬───────┘            │  丢 overrun / 只到微秒      │
          │                    └────────────┬───────────────┘
          │  信号密集 ⇒ 累积漂移              │
          ▼                                 ▼
  ┌───────────────────────────┐   ┌────────────────────────────┐
  │ clock_nanosleep 23.5.4    │   │ POSIX 定时器（23.6）        │
  │  TIMER_ABSTIME ⇒ 不漂移   │   │  timer_create/settime/delete│
  │  可选 clockid（23.5）     │   │  SIGEV_NONE/SIGNAL/THREAD/  │
  └───────────────────────────┘   │  THREAD_ID                  │
                                  │  overrun 计数（23.6.6）     │
                                  └────────────┬───────────────┘
                                               │ 通知是异步的 / 有线程开销
                                               ▼
                                  ┌────────────────────────────┐
                                  │ timerfd（23.7）             │
                                  │  定时器 = 一个可读 fd       │
                                  │  read() 直接返回到期次数    │
                                  │  可进 select/poll/epoll     │
                                  └────────────────────────────┘

  给阻塞调用加超时：23.3（alarm + SIGALRM 五步法）
  时间精度从哪来：23.2（jiffy vs HRT / clock_getres 判据）
```

---

## 双线提示

| 线 | 本章拿什么去用 |
|----|--------------|
| **HFT** | ① 定时器**只用 `timerfd(CLOCK_MONOTONIC)` + `epoll`** —— 行情/控制/定时全是 fd，主循环只有一个 `epoll_wait`，没有异步信号上下文，也没有线程开销；② 要在**绝对时刻**动手就用 `TFD_TIMER_ABSTIME`（或 `clock_nanosleep(TIMER_ABSTIME)`），免疫抖动累积；③ 用 `read()` 返回的**计数 > 1** 当免费的「主循环被卡住」告警；④ 量耗时用 `CLOCK_MONOTONIC`/`CLOCK_MONOTONIC_RAW`，跨机对齐用 PTP 校准的 `CLOCK_TAI`（**无闰秒跳变**），**只有 `CLOCK_REALTIME` 可以被设置**；⑤ 别用信号做超时：处理器里连 `printf` 都不能安全调，更别说下单/撤单这类要审计的操作 |
| **嵌入式** | ① 检查目标板有没有 **HRT**（`clock_getres`）：没有时所有睡眠精度掉回 jiffy，HZ=100 就是 **10 ms** 粒度，超时设计要按最坏一整个 jiffy 留余量；② 最小 rootfs 上的 `sleep()` 可能是 `alarm()` 实现 ⇒ 混用必踩，跨平台代码不要混 `sleep()`/`alarm()`/`setitimer()`；③ 单线程事件循环固件首选 `timerfd`（一个 `poll()` 管住 UART/GPIO/网络/定时），但要求内核 ≥ 2.6.25；④ `SIGEV_THREAD` 在 RAM 紧张的板子上优先排除（每定时器一个线程 = 一个栈）；⑤ `exec` 后旧定时器被清除（POSIX）/保留（`timerfd`、`setitimer`）——「重启后旧看门狗还在打主人」这类问题的根因就在这个差别上 |
| **两条线共同的坑** | 定时器只保证**到期**，不保证**执行**；所有实测数字（µs 级超出量、jiffy 值、overrun 计数）都是**环境相关**，只能在同一进程的同一次运行内比较；CE 的 20 秒 SIGKILL / 32 KB 输出上限 / 8 KB 全缓冲会**截断**计数类数据 |

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | `ITIMER_REAL` 走墙钟、`VIRTUAL` 走用户 CPU、`PROF` 走用户+内核 CPU |
| 2 | 每类 `which` 每进程**只有一个**；`alarm`/`sleep`/`setitimer` 共享 `ITIMER_REAL` |
| 3 | 周期定时器**不漂移**（按上次到期点重装）；`nanosleep` 循环**会**漂移 |
| 4 | 到期 ≠ 被调度；处理延迟**无上界** |
| 5 | `alarm()` 返回值**向上取整**；裸 `setitimer` 取 `tv_sec` 是**截断** |
| 6 | `SA_RESTART` 会废掉「定时器打断阻塞调用」；`errno` 要在 `alarm(0)` **之前**存 |
| 7 | `it_interval` 在后（`itimerval`）/在前（`itimerspec`）—— 用指定初始化器 |
| 8 | `clock_settime` **向下**取整；定时器类 **向上**取整 |
| 9 | 判 HRT：`clock_getres(CLOCK_REALTIME)` 是 ns 就有；ms 就没有 |
| 10 | `CLOCK_*_COARSE` 的分辨率**就是 jiffy** |
| 11 | 只有 `CLOCK_REALTIME` 可设；要 `CAP_SYS_TIME` |
| 12 | `CLOCK_TAI` 无闰秒；`CLOCK_BOOTTIME` = `MONOTONIC` + suspend |
| 13 | `CLOCK_MONOTONIC_RAW` 不受 NTP 调整；`MONOTONIC` 会被 slew |
| 14 | `timer_t` 是**不透明句柄**；`Timer ID: 0` 合法 |
| 15 | POSIX 定时器数量受 **realtime 信号队列**上限约束 |
| 16 | `evp = NULL` ≡ `SIGEV_SIGNAL` + `SIGALRM` + `sival_int = 定时器 ID` |
| 17 | `it_value` 全 0 = **解除武装**；`it_interval` 全 0 = 只响一次 |
| 18 | `timer_gettime` 返回的是**剩余**，即使 `TIMER_ABSTIME` 武装 |
| 19 | realtime 信号**不排队**表达多次到期 ⇒ 必须读 **overrun** |
| 20 | `overrun = 到期次数 − 1`；**收到信号即重置** |
| 21 | `timer_getoverrun()` 是 async-signal-safe；`printf` **不是** |
| 22 | `clock_nanosleep` 返回**正数错误码**，不是 `-1` |
| 23 | `TIMER_ABSTIME` ⇒ `remain` 无用；重启用**同一个** `request`（不累积漂移） |
| 24 | POSIX 定时器：不跨 `fork`、`exec` 时**被清除** |
| 25 | `timerfd`：fd 跨 `fork` 继承、`exec` **保留**（除 `TFD_CLOEXEC`） |
| 26 | `read(timerfd)` 必须给 **8 字节**，读到的是**到期次数** |
| 27 | `timerfd` 的绝对模式是 **`TFD_TIMER_ABSTIME`** |
| 28 | `timerfd` 的 `clockid` 只有 `CLOCK_REALTIME` / `CLOCK_MONOTONIC` |
| 29 | `close()` 删 `timerfd` 定时器；引用计数归零才真释放 |
| 30 | 要「同时等 I/O 与定时」就上 **`timerfd`** —— 这是本章最该记住的一条 |

---

## 参考

- Kerrisk · TLPI Ch23（专有名词：interval timer / POSIX timer / `sigevent` / overrun / `timerfd`）
- man-pages：`man 2 getitimer` · `man 2 alarm` · `man 2 nanosleep` · `man 2 clock_nanosleep` · `man 2 clock_gettime` · `man 2 clock_getres` · `man 2 clock_settime` · `man 2 timer_create` · `man 2 timer_settime` · `man 2 timer_gettime` · `man 2 timer_delete` · `man 2 timer_getoverrun` · `man 2 timerfd_create` · `man 2 timerfd_settime` · `man 3 sleep` · `man 7 signal`（`SA_RESTART` 的重启表、`RLIMIT_SIGPENDING`）
- Linux v6.6 源码：`kernel/time/itimer.c`（`alarm_setitimer()` —— **向上取整在这里**） · `kernel/time/hrtimer.c` · `kernel/time/posix-timers.c` · `kernel/signal.c`（`sigwaitinfo` → `do_sigtimedwait()`） · `fs/timerfd.c` · `include/uapi/linux/timerfd.h`
- glibc 2.39：`sysdeps/unix/sysv/linux/alarm.c` · `sysdeps/unix/sysv/linux/timer_create.c`（**`timer_t` 的两种形态在这里**） · `nptl/`（`SIGEV_THREAD` 的辅助线程）
- **SUSv3 各函数的 RATIONALE**（原书 §23.8 点名推荐） —— 解释「为什么 `sleep()` 与 `alarm()` 的交互 unspecified」、「为什么定时器不用 realtime 信号排队」
- [Gallmeister, 1995] *POSIX.4: Programming for the Real World*（原书 §23.8 推荐的 POSIX.1b 专著）
- 实测环境：Compiler Explorer 公开 API（gcc 13.3.0 / x86-64 / Ubuntu 24.04），冻结日志 `tlpi-ch23-final*.txt`（35 个作业）

---

## 代码示例

本章 `code/` 下有 **25 个源文件**（**14 个原书镜像 + 4 个本仓库自写 + 4 个公共库镜像 + 1 个替身头文件**，另有 2 个旧 demo）：

### 原书镜像件（14 个，逐字取自 man7 官方 `timers/`）

| 文件 | 出处 | 备注 |
|------|------|------|
| `code/real_timer.c` | **Listing 23-1, page 479** | `setitimer` 演示 |
| `code/timed_read.c` | **Listing 23-2, page 487** | 阻塞调用加超时 |
| `code/t_nanosleep.c` | **Listing 23-3, page 490** | `nanosleep` + `remain` 续睡 |
| `code/ptmr_sigev_signal.c` | **Listing 23-5, page 500** | POSIX 定时器 + 信号通知 |
| `code/ptmr_sigev_thread.c` | **Listing 23-7, page 504** | POSIX 定时器 + 线程通知 |
| `code/demo_timerfd.c` | **Listing 23-8, page 510** | `timerfd` 演示 |
| `code/itimerspec_from_str.c` + `code/itimerspec_from_str.h` | **Listing 23-6, page 502** | 参数解析（2 个文件） |
| `code/t_clock_nanosleep.c` | 习题 23-2 的官方解答 | `TIMER_ABSTIME` 对照 |
| `code/ptmr_null_evp.c` | 习题 23-3 的官方解答 | `evp = NULL` 的默认语义 |
| `code/clock_times.c` | 未印刷 | 显示 4 种时钟 + 分辨率 |
| `code/cpu_burner.c` | 未印刷 | CPU 占用率测量（`-c` 打 cgroup） |
| `code/cpu_multi_burner.c` | 未印刷 | 多进程版 |
| `code/cpu_multithread_burner.c` | 未印刷 | 多线程版 |

### 本仓库自写（4 个，**原书没有**）

| 文件 | 对应节 | 演示什么 |
|------|--------|---------|
| `code/c23_alarm_pipe_timeout.c` | 23.3 | 用「写端不关的管道」造真阻塞 ⇒ 把 `Read timed out` 分支跑出来；`sa_flags = 0` vs `SA_RESTART` 对照 |
| `code/c23_posix_timer_overrun.c` | 23.6.6 | `SIG_BLOCK` 通知信号 + 1 ms 周期 ⇒ 确定性地拿到 `overrun = 999`，并验证「收到即重置」 |
| `code/ex23_1_my_alarm.c` | 23.9 习题 23-1 | 用 `setitimer()` 实现 `alarm()`；把「向上取整 vs 截断」实测出来 |
| `code/ex23_4_ptmr_sigwaitinfo.c` | 23.9 习题 23-4 | 把 Listing 23-5 的处理器换成 `sigwaitinfo()`（同步上下文）；加 `-n <N>` 退出扩展 |

### 公共库镜像（4 个，来自 TLPI `lib/` 与 `time/`）

| 文件 | 说明 |
|------|------|
| `code/get_num.c` + `code/get_num.h` | `getInt()` / `getLong()` |
| `code/curr_time.c` + `code/curr_time.h` | `currTime()`（`ptmr_*` 三个用） |

### 替身（1 个）

| 文件 | 说明 |
|------|------|
| `code/tlpi_hdr.h` | 代替原书的 `lib/tlpi_hdr.h`（含 `error_functions` 的 `static inline` 最小实现）。**唯一行为偏差**：`ename[]` 表缺失 ⇒ 错误信息显示 `ERROR [?UNKNOWN? ...]` 而不是 `ERROR [ENOENT ...]`。差异见 [`code/README.md`](code/README.md) |

### 旧 demo（2 个，Ch23 早期版本遗留）

| 文件 | 说明 |
|------|------|
| `code/nanosleep_retry.c` | `nanosleep` + `EINTR` 安全重试 |
| `code/posix_timer_thread.c` | `timer_create` + `CLOCK_MONOTONIC` + `SIGEV_THREAD` |

编译与运行（在 `code/` 目录下）：

```bash
# 单文件程序（只依赖 libc）
gcc -O0 -Wall -Wextra -o clock_times clock_times.c && ./clock_times showres
gcc -O0 -Wall -Wextra -o c23_alarm_pipe c23_alarm_pipe_timeout.c && ./c23_alarm_pipe
gcc -O0 -Wall -Wextra -o c23_overrun c23_posix_timer_overrun.c && ./c23_overrun
gcc -O0 -Wall -Wextra -o ex23_1_my_alarm ex23_1_my_alarm.c && ./ex23_1_my_alarm

# 需要 get_num.c 的
gcc -O0 -Wall -Wextra -o real_timer real_timer.c get_num.c && ./real_timer 1 0 1 0
gcc -O0 -Wall -Wextra -o t_nanosleep t_nanosleep.c get_num.c && ./t_nanosleep 1 0
gcc -O0 -Wall -Wextra -o t_clock_nanosleep t_clock_nanosleep.c get_num.c && ./t_clock_nanosleep 1 0 a
gcc -O0 -Wall -Wextra -o timed_read timed_read.c get_num.c && echo hi | ./timed_read 2

# 需要 itimerspec_from_str.c 的
gcc -O0 -Wall -Wextra -o demo_timerfd demo_timerfd.c itimerspec_from_str.c get_num.c
./demo_timerfd 0/500000000:0/500000000 4

# 需要 curr_time.c 的
gcc -O0 -Wall -Wextra -o ptmr_sigev_signal ptmr_sigev_signal.c itimerspec_from_str.c curr_time.c get_num.c
gcc -O0 -Wall -Wextra -pthread -o ptmr_sigev_thread ptmr_sigev_thread.c itimerspec_from_str.c curr_time.c get_num.c
gcc -O0 -Wall -Wextra -o ptmr_null_evp ptmr_null_evp.c curr_time.c get_num.c
gcc -O0 -Wall -Wextra -o ex23_4_ptmr_sigwaitinfo ex23_4_ptmr_sigwaitinfo.c itimerspec_from_str.c curr_time.c get_num.c
```

> ⚠️ **`SIGEV_THREAD` 程序要加 `-pthread`**。**不需要 `-lrt`**（glibc ≥ 2.17 已并入 libc）。
>
> ⚠️ **`c23_alarm_pipe_timeout.c` 刻意不定义 `_POSIX_C_SOURCE`**（与 `timed_read.c` 一致）：一旦显式定义它，glibc 就不再隐式打开 `_DEFAULT_SOURCE`，`SA_RESTART` 会变成未声明。

### ⚠️ 会漂 / 会被截断的量（不要写进断言）

| 量 | 观察到的变化 |
|----|-------------|
| `CLOCK_REALTIME` / `CLOCK_TAI` 的绝对秒数 | 每次运行不同 |
| `CLOCK_MONOTONIC` / `CLOCK_BOOTTIME` | **跨 CE 会话会落在不同宿主机上**（实测 `15308.600` vs `16451.861`）⇒ **只在同一日志内比较** |
| `CLOCK_MONOTONIC_RAW − CLOCK_MONOTONIC` | 本机 0.289 秒（NTP slew 累积量），随宿主与时长变化 |
| `timer_t` 的具体数值 | `SIGEV_THREAD` 下是地址；每次都不同 |
| 睡眠/定时的超出量（+51~96 µs） | 不同宿主、不同负载 |
| `cpu_burner` 的 `%CPU` | 首行偏低（启动抖动），稳态 46~50 |
| `cpu_multi_burner` 的 `delta` | 偶发 0.32 / 0.67 / 0.78（被抢占） |
| 无退出路径程序的行数 | 受 CE **约 32 KB 输出采集上限**与 **20 秒 SIGKILL** 影响 ⇒ 只是「采集到的」 |

### 相对稳定（可以引用）

| 量 | 值 |
|----|----|
| `clock_getres(CLOCK_REALTIME)` | `0.000000001`（HRT 开启时） |
| `clock_getres(CLOCK_*_COARSE)` | `0.001000000`（= jiffy，本机 `CONFIG_HZ=1000`） |
| `sizeof(timer_t)` / `sizeof(clockid_t)` | `8` / `4`（x86-64 / glibc 2.39） |
| `CLOCK_TAI − CLOCK_REALTIME` | `37.000` 秒 |
| `SIGRTMAX` | `64` |
| `SIGALRM` | `14` |
| `EINTR` | `4` |
| `overrun` 与「到期次数」的关系 | `到期次数 = overrun + 1` |
| `SA_RESTART` 的效果 | 让可重启的系统调用**永不返回**（被信号打断后自动重启） |

---

## 与前后章

| | 章 | 关系 |
|--|----|------|
| ← 前置 | [Ch22 Signals: Advanced](../chapter-22-signals-advanced/README.md) | realtime 信号、`si_value`、`sigwaitinfo()` —— §23.6 的信号通知全靠它 |
| ← 前置 | [Ch21 Signal Handlers](../chapter-21-signal-handlers/README.md) | `SA_SIGINFO`、`SA_RESTART`、async-signal-safe 函数表 |
| ← 前置 | [Ch10 Time](../chapter-10-time/README.md) | §10.6 软件时钟 / jiffy —— §23.2 精度讨论的前提 |
| → 后置 | [Ch24 Process Creation](../chapter-24-process-creation/README.md) | `fork`/`exec` 对定时器的继承规则（本章出现三次：`setitimer` 保留、POSIX 定时器清除、`timerfd` 保留） |
| → 后置 | [Ch29 Threads: Introduction](../chapter-29-threads-intro/README.md) | `SIGEV_THREAD` 的通知函数在线程里跑；`nanosleep` 睡的是**线程** |
| → 后置 | [Ch63 Alternative I/O Models](../chapter-63-alternative-i-o-models/notes/63.4-the-epoll-api.md) | `select`/`poll`/`epoll` —— `timerfd` 的真正威力在那里才看全（⚠️ 该章还没有 README，故链到现存笔记） |
