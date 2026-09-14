# TLPI 第 10 章 — Time

**优先级**：🔴 必读（HFT 打时间戳 / 延迟测量的直接基础）
**前置**：[Ch3](../chapter-03-system-programming-concepts/README.md)（syscall 与 `errno` 范式）· [Ch9](../chapter-09-process-credentials/README.md)（`CAP_*` 权限）
**后置**：[Ch23 Timers](../chapter-23-timers-sleeping/README.md)（`timer_create` / `timerfd_create` / `clock_nanosleep`）

---

## 小节目录

- [10.1 Calendar Time 日历时间](notes/10.1-calendar-time.md)
- [10.2 Time-Conversion Functions 时间转换函数](notes/10.2-time-conversion-functions.md)
- [10.3 Timezones 时区](notes/10.3-timezones.md)
- [10.4 Locales 本地化](notes/10.4-locales.md)
- [10.5 Updating the System Clock 修改系统时钟](notes/10.5-updating-the-system-clock.md)
- [10.6 The Software Clock (Jiffies) 软件时钟](notes/10.6-the-software-clock-jiffies.md)
- [10.7 Process Time 进程时间](notes/10.7-process-time.md)
- [10.8 Summary 本章总结](notes/10.8-summary.md)
- [10.9 Exercise 练习](notes/10.9-exercise.md)

> 九节的划分与 TLPI 原书一致（10.1–10.9）。原书本章**没有子编号小节**（不像 Ch3 那样有 3.5.1/3.6.1），所以一篇对应一节，不需要合并。
>
> **注意 10.9 的划分**：原书第 10 章的练习题部分与全书写法一致（章节正文后单列 Exercise）。TLPI Ch10 **只有一道习题**（10-1），本节另附 5 道自编练习。

---

## 章节目标

- **模型**：Linux 上的「时间」不是**一个**东西，而是**十一个语义完全不同的时钟**（`CLOCK_REALTIME` … `CLOCK_TAI`）
- **辨析**：墙上时间（wall）与 CPU 时间是**两套互不相干的账**；`wall` 照走而 CPU 不涨（睡眠），或两者同时涨（忙等）
- **工程**：选对时钟 = 选对语义。测延迟用 `MONOTONIC`，打时间戳用 `REALTIME`，算成本用 `*_CPUTIME_ID`
- **溯源**：本章 **6 处**「书上的数值/说法」被 v6.6 源码 + 实测修正（详见 [10.8 §4](notes/10.8-summary.md)）

### 本章的三条主线

| 维度 | 问题 | 涉及节 |
|------|------|--------|
| **读** | 内核给了哪些时钟？各是什么语义？怎么写成人看的字符串？ | 10.1 / 10.2 / 10.3 / 10.4 / 10.6 / 10.8 |
| **写** | 谁能改时钟？跳变还是平滑？ | 10.5 |
| **量** | 这个进程烧了多少 CPU？和「过了多久」有什么区别？ | 10.7 |

### 与 Ch23 边界（防混淆）

| 章 | 主题 | 内容 |
|----|------|------|
| **Ch10** | Time | **读时间 / 写时间 / 算 CPU 时间** —— 「现在几点、过了多久、花了多少」 |
| **Ch23** | Timers | **定时器** —— 「N 时间之后叫我」：`timer_create` / `timerfd_create` / `clock_nanosleep` |

Ch10 是 Ch23 的地基：Ch23 的所有接口都要先指定**用哪个时钟**（`clockid_t`）。

---

## 原书示例清单（man7 官方按章文件列表）

| Listing | 文件 | 页码 | 作用 |
|---------|------|------|------|
| **10-1** | `time/calendar_time.c` | p.191 | `time()` / `gettimeofday()` / `gmtime()` / `localtime()` / `asctime()` / `ctime()` / `mktime()` 一条链走完 |
| **10-2** | `time/curr_time.h` + `time/curr_time.c` | p.194 | `currTime()` 助手函数——**返回 `static` 缓冲区**（这是个陷阱） |
| **10-3** | `time/strtime.c` | p.197 | `strptime()` → `mktime()` → `strftime()` 的往返；那个「不给值的 `_XOPEN_SOURCE`」 |
| **10-4** | `time/show_time.c` | p.199 | `ctime()` / `asctime()` / `strftime()` 三连打同一个时间 |
| **10-5** | `time/process_time.c` | p.208 | `clock()` 与 `times()` 在四个阶段各打一次 |

> 出处：[man7 — List of source code files, by chapter](https://man7.org/tlpi/code/online/all_files_by_chapter.html)（6 个文件 = 5 个 `Listing` + 1 个头文件）。
> 本仓库的 `code/` 下**逐字镜像**了这 6 个文件（保真到注释与空行），并额外提供 `tlpi_hdr.h` 的**最小替身**让它们能单文件编译（原书的 `tlpi_hdr.h` 依赖 `lib/` 目录）。

三个容易记错的点：

| 易错 | 正解 |
|------|------|
| 以为 Listing 10-2 只有一个文件 | 是 **`.h` + `.c` 两个**（`curr_time.h` / `curr_time.c`） |
| 以为 `currTime()` 可以直接存起来 | 它返回**静态缓冲区**，第二次调用就覆盖（实测同一地址 `0x404060`） |
| 以为 `strtime.c` 的 `_XOPEN_SOURCE` 是笔误 | 原书**故意不写值**（只写 `#define _XOPEN_SOURCE`），靠 glibc 的默认值兜住 —— 详见 [10.2 §7](notes/10.2-time-conversion-functions.md) |

---

## 易错清单

1. **`sysconf(_SC_CLK_TCK)` 恒为 100，不是 `CONFIG_HZ`** —— 它是内核写死的 `USER_HZ`（`include/asm-generic/param.h:9`）。本容器真实 `CONFIG_HZ = 1000`，两者差 10 倍。
2. **`CLOCKS_PER_SEC` 同名不同值** —— 内核侧 = `USER_HZ` = 100（`param.h:10`）；glibc 侧 = 1000000。两条换算链完全独立。
3. **`times()` 的返回值是「开机以来的 tick 数」** —— 不是进程时间！进程时间在 `struct tms` 出参里（`kernel/sys.c:1033`）。
4. **`clock()` 在 Linux 上不调 `times()`** —— glibc 的 `sysdeps/unix/sysv/linux/clock.c` 走的是 `CLOCK_PROCESS_CPUTIME_ID`，粒度 ns 而非 10 ms。
5. **`clock_getres(CLOCK_REALTIME)` 报 1 ns ≠ 真的能分辨 1 ns** —— 实测连续两次读的最小正增量是 **17 ns**（读一次时钟的成本下限）。
6. **`CLOCK_*_COARSE` 的值不对齐 jiffy 边界** —— 实测最小正增量是 `999979 ns` 而不是 `1000000`。别用 `% 1000000` 判断来源。
7. **`gmtime` / `localtime` / `asctime` / `ctime` / `strerror` / `getpwnam` 共用静态缓冲区** —— 实测两次调用返回**同一地址**。要同时持有两个就上 `_r` 版。
8. **`struct tm` 的字段没有一个是直觉值** —— `tm_year` = 年 − 1900；`tm_mon` = 0..11；`tm_yday` = 0..365；`gmtime` 的 `tm_isdst` 恒为 0。
9. **`strftime` 失败只返回 0** —— 不告诉你需要多大，而且**缓冲区内容未定义**（实测：预填 `#` 后失败，内容仍是 `#`）。这是 `zh_CN` 下最容易被咬的地方。
10. **`%:z` 在 glibc 2.39 上不存在** —— 原样输出三个字符 `%:z`（`ret=3`）。`date +%:z` 能用是因为 coreutils 自己解析格式串。
11. **POSIX `TZ` 的符号是反的** —— `TZ=XXX-8` 表示**东八区**（offset `+0800`）；所以 `TZ=UTC+8` 得到的是 `-0800`。
12. **`gettimeofday` / `settimeofday` 的 `tz` 参数永远传 `NULL`** —— Linux 内核没有时区信息（实测 glibc 只填 0）。
13. **`adjtime()` 每秒最多补 500 µs** —— `MAX_TICKADJ`（`kernel/time/ntp.c:43`）。拨 1 ms 要 ~2 秒，拨 1 秒要 ~33 分钟。
14. **`clock_settime()` 会跳变** —— 所有 `TIMER_ABSTIME` 定时器行为不可预测；HFT 里这是事故信号。
15. **`uid=0` ≠ 有 `CAP_SYS_TIME`** —— 容器默认裁掉这个 capability，四条写接口全部 `EPERM`（实测）。
16. **CPU 时间不跨进程传递** —— 子进程只进父进程的 `tms_cutime` / `RUSAGE_CHILDREN`，`CLOCK_PROCESS_CPUTIME_ID` 不变。
17. **`times()` 的 CPU 时间粒度是 10 ms** —— 小循环读到 `0 tick` 不代表没耗 CPU。要测小量级必须用 `CLOCK_PROCESS_CPUTIME_ID`。
18. **测间隔绝不用 `CLOCK_REALTIME`** —— 而且**小时级**的测量也别用 `MONOTONIC_RAW`（不被 NTP 校正，100 ppm × 4 h = 1.44 s 累积误差）。
19. **`nanosleep` 被信号打断要按剩余量补睡** —— `while (nanosleep(&req, &req) == -1 && errno == EINTR);`。掐头重睡会睡过头。
20. **`REALTIME − MONOTONIC` 的绝对值无意义** —— 两个起点不同。但它的**变化率**有意义：跳变 = 有人动了时钟。
21. **容器里 `CLOCK_BOOTTIME == CLOCK_MONOTONIC`** —— 容器的 suspend 和宿主机的 suspend 是两回事。
22. **小于 100 µs 的 `nanosleep` 没有精度可言** —— 实测「超出量」恒为 **56–65 µs**（固定调度往返开销），请求 1 µs 时相对误差 6236%。

---

## 章节链路

```text
Ch3  syscall 模型 + errno 范式
  → Ch9  CAP_* 能力（CAP_SYS_TIME / CAP_WAKE_ALARM 的前提）
  → Ch10 时间三件事：
           读 → time / gettimeofday / clock_gettime（11 个 clockid）
           写 → settimeofday / clock_settime / adjtime / clock_adjtime
           量 → times / clock / CLOCK_PROCESS_CPUTIME_ID / getrusage
  → Ch20-22 Signals（EINTR、CPU-time 定时器）
  → Ch23 Timers（把 clockid 接上定时器：timer_create / timerfd_create）
  → Ch30 pthread_cond_timedwait（超时用哪个时钟）
  → Ch63 epoll_wait / ppoll 的超时
```

---

## 双线提示

| 路线 | |
|------|--|
| **HFT** | 打时间戳用 `CLOCK_MONOTONIC`（19 ns）→ 量大换 `COARSE`（4.9 ns）→ 极致用 `rdtsc`（10.5 ns）；**测延迟绝不用 `REALTIME`**；校时走 `adjtime` 不跳变；用 `REALTIME − MONOTONIC` 差值监控跳变；`ru_nivcsw` / `ru_minflt` 做抖动诊断 |
| **嵌入式** | 32 位平台 `times()` 1.36 年就回绕；老内核无 hrtimer 时本章一半现代结论要退回原书版本；无 RTC 的板子上电 `REALTIME` 是垃圾；ARM 用 `CNTVCT_EL0` 等价 TSC 但 `CNTFRQ_EL0` 常是假值；容器/无 `CAP_SYS_TIME` 环境改不了时钟 |

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | Linux 有 **11 个 `CLOCK_*`**：4 个 POSIX 标准 + 7 个 Linux 扩展 |
| 2 | **wall 照走、CPU 不涨**（睡眠）；**两者同涨**（忙等）。测延迟用 wall，测成本用 CPU |
| 3 | `CLOCK_REALTIME` 会跳变；`CLOCK_MONOTONIC` 被 NTP slewing；`CLOCK_MONOTONIC_RAW` 完全不校正 |
| 4 | 测间隔：短期（<1 s）可 `RAW`，长期必须 `MONOTONIC` |
| 5 | `sysconf(_SC_CLK_TCK) = 100`（`USER_HZ`），与 `CONFIG_HZ` 无关 |
| 6 | `CLOCKS_PER_SEC`：内核侧 100 / glibc 侧 1000000 |
| 7 | `times()` 返回值 = 开机 tick；出参 `struct tms` 才是进程时间 |
| 8 | `clock()` 在 Linux = `CLOCK_PROCESS_CPUTIME_ID` 重新标度，不是 `times()` 的简化 |
| 9 | `clock_getres` 报的是**声明值**；实测粒度看「连续两次读的最小正增量」 |
| 10 | hrtimer 让 `CLOCK_REALTIME` / `MONOTONIC` 报 1 ns；只有 `*_COARSE` 还是 1/HZ |
| 11 | `REALTIME − MONOTONIC` 的**变化率**才是时钟跳变探测器 |
| 12 | `TAI − REALTIME = 37 s`（闰秒）；`BOOTTIME − MONOTONIC` = 挂起时长 |
| 13 | 改时钟四个接口全要 `CAP_SYS_TIME`；容器默认没有 |
| 14 | `adjtime` 每秒 ≤ 500 µs（平滑）；`clock_settime` 跳变 |
| 15 | 时间戳偏移从 `tm_gmtoff` 取；`%:z` glibc 不支持，要冒号自己拼 |
| 16 | `strftime` 失败只返回 0，缓冲区内容未定义 |

---

## 参考

- Kerrisk, *The Linux Programming Interface*, **Chapter 10 — Time**
- [man7 官方源码清单（按章）](https://man7.org/tlpi/code/online/all_files_by_chapter.html) · [OUTLINE](../OUTLINE.md) · [Ch23 Timers](../chapter-23-timers-sleeping/README.md)
- 内核源码（v6.6）：`kernel/time/`（`time.c` / `hrtimer.c` / `posix-timers.c` / `posix-cpu-timers.c` / `ntp.c` / `timekeeping.c`）、`include/uapi/linux/time.h` · `timex.h` · `times.h`、`include/vdso/jiffies.h` · `ktime.h`、`include/linux/hrtimer_defs.h`、`kernel/Kconfig.hz`

---

## 代码示例

本章 `code/` 下有：

- **14 个自编 demo**（`c10_*.c` 11 个 + `ex10_*.c` 3 个）
- **6 个原书文件**逐字镜像（`calendar_time.c` / `curr_time.[hc]` / `strtime.c` / `show_time.c` / `process_time.c`）
- **1 个框架替身** `tlpi_hdr.h`（原书 `lib/tlpi_hdr.h` 的最小可用子集，让 5 个原书程序能单文件编译）

全部在 Compiler Explorer（gcc 13.3.0 / x86-64 / Ubuntu 24.04）上真实编译 + 运行过，共 **18 个作业**，全部 `build code = 0` / `didExecute = True` / `diagnostics = 0`。输出原样抄在对应笔记的实测块里。完整索引见 [`code/README.md`](code/README.md)。

| 文件 | 对应节 | 演示什么 | 需要什么 |
|------|--------|----------|---------|
| `c10_1_calendar_probe.c` | 10.1 | 三种读法同秒 / Epoch 原点 / `time_t` 真实宽度 / 2038 边界 | `-` |
| `c10_2_strftime.c` | 10.2 | `struct tm` 字段 + 38 个格式符全表 + `strftime`→`strptime`→`mktime` 往返 | `-` |
| `c10_2_reentrant.c` | 10.2 | 四个函数共用静态缓冲区（地址实测）+ `_r` 版对照 | `-` |
| `c10_2_curr_time_demo.c` | 10.2 | 原书 `currTime()` 的陷阱（返回同一 `static` 缓冲） | `curr_time.h` + `curr_time.c` |
| `c10_3_timezone.c` | 10.3 | 九种 `TZ` 写法 + DST 双季节 + `tm_gmtoff` 与 `mktime−timegm` 互证 | `-` |
| `c10_4_locale.c` | 10.4 | 装了哪些 locale + `LC_TIME` 影响面 + `nl_langinfo` + `strftime` 返回 0 的坑 | `-` |
| `c10_5_setclock.c` | 10.5 | 四路改时钟全 `EPERM` + 只读拿 `struct timex` 全字段 | `-`（**故意在非特权下跑**） |
| `c10_6_clockres.c` | 10.6 | `clock_getres` 全景 + 实测最小正增量 + `USER_HZ ≠ CONFIG_HZ` 现场撞坑 | `-` |
| `c10_6_coarse_cost.c` | 10.6 | 六种读法的单次开销（vDSO / COARSE / `rdtsc`） | `-`（**必须 `-O0`**） |
| `c10_7_process_time.c` | 10.7 | wall vs cpu 四阶段 + 线程/进程口径 + 子进程账 + `getrusage` | `-` |
| `c10_8_clockids.c` | 10.8 | 11 个 `CLOCK_*` 全景 + 四个差值对账 | `-` |
| `ex10_1_clock_cycle.c` | 10.9 | 原书习题 10-1：`clock_t` 回绕周期 + 本机 64 位对照 | `-` |
| `ex10_2_utc_offset.c` | 10.9 | ISO 8601 带偏移（半刻钟时区）+ 证明 `%:z` 不存在 | `-` |
| `ex10_3_sleep_precision.c` | 10.9 | `nanosleep` 亚 jiffy 能力 + `sleep(1)` 误差 + `EINTR` 补睡 | `-` |
| `calendar_time.c` | 10.1 | **原书 Listing 10-1**（p.191） | `tlpi_hdr.h` |
| `curr_time.c` / `curr_time.h` | 10.2 | **原书 Listing 10-2**（p.194） | `tlpi_hdr.h` |
| `strtime.c` | 10.2 | **原书 Listing 10-3**（p.197） | `tlpi_hdr.h` + `-D_XOPEN_SOURCE=700` |
| `show_time.c` | 10.2 | **原书 Listing 10-4**（p.199） | `tlpi_hdr.h` |
| `process_time.c` | 10.7 | **原书 Listing 10-5**（p.208） | `tlpi_hdr.h` |

一次编完全部自编 demo（在 `code/` 目录下）：

```bash
# 13 个单文件 demo（c10_2_curr_time_demo 需要额外源文件，单独编）
for f in c10_1_*.c c10_2_strftime.c c10_2_reentrant.c c10_3_*.c c10_4_*.c \
         c10_5_*.c c10_6_*.c c10_7_*.c c10_8_*.c ex10_*.c; do
    gcc -O0 -Wall -Wextra -o "${f%.c}" "$f" || echo "FAIL $f"
done
```

需要额外文件的：

```bash
# 原书 Listing 10-1 / 10-3 / 10-4 / 10-5 都依赖 tlpi_hdr.h 替身
gcc -O0 -Wall -Wextra -Wno-unused-parameter -o calendar_time tlpi_hdr.h calendar_time.c
gcc -O0 -Wall -Wextra -Wno-unused-parameter -o show_time     tlpi_hdr.h show_time.c
gcc -O0 -Wall -Wextra                      -o process_time  tlpi_hdr.h process_time.c
gcc -O0 -Wall -Wextra -D_XOPEN_SOURCE=700  -o strtime       tlpi_hdr.h strtime.c

# 原书 Listing 10-2 的助手函数 + 演示它陷阱的自编 demo
gcc -O0 -Wall -Wextra -o c10_2_curr_time_demo curr_time.h curr_time.c c10_2_curr_time_demo.c
```

带参数的（原书程序从命令行取输入）：

```bash
./strtime '2023-11-14 22:13:20|%Y-%m-%d %H:%M:%S'
./process_time 1000000
```

**几条实测结论**（都是本仓库跑出来的，不是书上抄的）：

- **`sysconf(_SC_CLK_TCK)` 恒为 100**，而 `CLOCK_MONOTONIC_COARSE` 的实测分辨率是 `1000000 ns` → 真 `CONFIG_HZ = 1000`。用 `_SC_CLK_TCK` 推会**差 10 倍**（CE 容器绝对值；可迁移的是「两者不是一个数」这个事实）
- **`clock_getres(CLOCK_REALTIME) = 1 ns` 但实测最小正增量是 17 ns**（三个精确时钟都是 17 ns）；`*_COARSE` 的最小正增量是 `999979 ns`，且 200 万次读只有 4~9 次前进（这组「单次极值」每次跑都有小幅漂移）
- **读一次时间全是几十 ns**：`time(NULL)` 2.9 / `gettimeofday()` 19.0 / `clock_gettime(MONOTONIC)` 18.9 / `*_COARSE` 4.9 / `rdtsc` 10.5 ns（基线 `nop_call()` 2.3 ns）
- **`nanosleep(300ms)` 后 wall 涨 0.300 s，cpu 涨 0.000 s**（`ticks=134/50/0/0` 一字未变）—— wall 与 CPU 是两套账的直接证据
- **`clock()` 与 `CLOCK_PROCESS_CPUTIME_ID` 数值完全一致**（`1.852 s`），而 `times()` 的 `utime+stime` 是 `1.84 s` —— 证明 Linux 上 `clock()` 走的是前者
- **`clock_adjtime` 只读不需要特权**：实测拿到 `status=0x2000`（`STA_NANO=1`）、`tai=37`、`tick=10000`（`USER_HZ` 下 10 ms），而四条写接口全部 `EPERM`
- **`TAI − REALTIME = +37.000000134 s`**（闰秒）；**`RAW − MONO = −32.586189 ms`**（NTP 累计 slew）；**`BOOTTIME − MONO = +68 ns`**（容器没挂起过）
- **glibc 2.39 不认 `%:z`**：`ret=3`、输出就是字面 `%:z`；`%Ez` / `%Oz` 正常（`ret=5`）
- **`strftime` 失败只返回 0**：`zh_CN.UTF-8` 下 `%c` 需要 43 字节，只给 20 字节 → `ret=0`，缓冲区内容**未定义**（预填的 `#` 一个没变）
- **`nanosleep` 的固定开销 ≈ 60 µs**：请求 1 µs / 10 µs / 100 µs / 1 ms / 10 ms / 100 ms，「超出量」分别是 +62.4 / +56.6 / +56.9 / +56.8 / +60.1 / +64.8 µs —— **几乎恒定的固定开销，不是分辨率问题**
- **`times()` 在 32 位平台上 1.36 年回绕**（`2³²/100`），64 位平台约 29.2 亿年；本机 `sizeof(clock_t) = 8`
