# Ch10 `code/` 目录说明

TLPI 第 10 章（Time）的可编译代码。分三类：

| 类别 | 数量 | 命名 |
|------|------|------|
| 自编 demo | **14** | `c10_<节>_<名字>.c`（11 个）+ `ex10_<n>_<名字>.c`（3 个） |
| 原书镜像 | **6** | 与 man7 官方文件名一致，**逐字保真** |
| 框架替身 | **1** | `tlpi_hdr.h` |

**全部 18 个作业**（14 自编 + 4 个可独立运行的原书程序）都在 Compiler Explorer（gcc 13.3.0 / x86-64 / Ubuntu 24.04）上跑过：`build code = 0`、`didExecute = True`、`diagnostics = 0`。

---

## 文件表

### 自编 demo（14）

| 文件 | 对应节 | 演示什么 | 需要什么 |
|------|--------|----------|---------|
| `c10_1_calendar_probe.c` | 10.1 | `time()` / `gettimeofday()` / `clock_gettime()` 三者同秒；Epoch 原点与负数时间；`time_t` 真实宽度与 2038 边界 | — |
| `c10_2_strftime.c` | 10.2 | `struct tm` 各字段的真实取值；38 个 `strftime` 格式符全表；两条分解路径；`strftime`→`strptime`→`mktime` 往返 | — |
| `c10_2_reentrant.c` | 10.2 | `gmtime` / `localtime` / `asctime` / `ctime` 共用静态缓冲区（打地址 + 打内容验证覆盖）；`_r` 版对照 | — |
| `c10_2_curr_time_demo.c` | 10.2 | 原书 `currTime()` 助手函数的**陷阱**：返回同一 `static` 缓冲 | `curr_time.h` + `curr_time.c` |
| `c10_3_timezone.c` | 10.3 | 系统时区来源；`tzset()` 与三个全局；九种 `TZ` 写法（含两个陷阱）；DST 双季节；`tm_gmtoff` 与 `mktime−timegm` 互证 | — |
| `c10_4_locale.c` | 10.4 | `setlocale` 的两种调用语义；本机装了哪些 locale；`LC_TIME` 影响哪些格式符；`nl_langinfo` / `localeconv`；`LC_NUMERIC` 与 `printf`；**`strftime` 返回 0 的坑** | — |
| `c10_5_setclock.c` | 10.5 | 读 `CapEff` 判 `CAP_SYS_TIME`；四个写接口逐个试（全 `EPERM`）；两个只读接口拿 `struct timex` 全字段；`gettimeofday` 的 `tz` 参数 | — |
| `c10_6_clockres.c` | 10.6 | 三个「每秒几个 tick」；`clock_getres` 全景；**实测最小正增量**；`USER_HZ ≠ CONFIG_HZ` 的完整撞坑 | —（跑几秒） |
| `c10_6_coarse_cost.c` | 10.6 | 六种读法的单次开销对照（`nop_call` / `time` / `gettimeofday` / 四种 `clock_gettime` / `rdtsc`） | **必须 `-O0`** |
| `c10_7_process_time.c` | 10.7 | `wall` vs `cpu` 四阶段快照；进程/线程 CPU 口径；`times()` 刻度太粗的实证；子进程记账与 `getrusage(RUSAGE_CHILDREN)` | —（会 fork） |
| `c10_8_clockids.c` | 10.8 | 11 个 `CLOCK_*` 的 id / 分辨率 / 当前读数全表；四个差值对账；ALARM 时钟 | — |
| `ex10_1_clock_cycle.c` | 10.9 | 原书习题 10-1：`clock_t` 回绕周期（32 位假设 vs 本机 64 位）；`times()` 返回值真相 | — |
| `ex10_2_utc_offset.c` | 10.9 | ISO 8601 带偏移（含半刻钟时区）；硬编码 `+0800` 的错误；**证明 `%:z` 在 glibc 2.39 不存在** | — |
| `ex10_3_sleep_precision.c` | 10.9 | `nanosleep` 亚 jiffy 能力；`sleep(1)` 误差；`EINTR` 与剩余量；原地重启演示 | —（跑几秒） |

### 原书镜像（6，逐字保真）

| 文件 | Listing | 页码 | 说明 |
|------|---------|------|------|
| `calendar_time.c` | **10-1** | p.191 | `time` → `gettimeofday` → `gmtime`/`localtime` → `asctime`/`ctime` → `mktime` |
| `curr_time.h` | **10-2** | p.194 | `currTime()` 声明 |
| `curr_time.c` | **10-2** | p.194 | `currTime()` 实现（返回 `static` 缓冲） |
| `strtime.c` | **10-3** | p.197 | `strptime` → `mktime` → `strftime` 往返；`_XOPEN_SOURCE` 不给值 |
| `show_time.c` | **10-4** | p.199 | `ctime` / `asctime` / `strftime` 三连打 |
| `process_time.c` | **10-5** | p.208 | `clock()` 与 `times()` 的阶段对照 |

> 镜像来源：`https://man7.org/tlpi/code/online/dist/time/<file>`（返回**原始源文件**，含 GPL 头与原作者注释）。
> 之所以取 `.c` 原始文件而不是网页 HTML 版：man7 的 HTML 渲染把源码切成多个 `<pre>` 块，会丢空行与缩进——**保真度不够**。

### 框架替身（1）

| 文件 | 说明 |
|------|------|
| `tlpi_hdr.h` | 原书 `lib/tlpi_hdr.h` 的**最小可用子集**（常用系统头 + `Boolean` + 退出常量 + `errExit` 等宏）。原书版依赖整个 `lib/` 目录，无法单文件编译 |

---

## 编译

一次编完 13 个单文件自编 demo（在 `code/` 目录下）：

```bash
for f in c10_1_*.c c10_2_strftime.c c10_2_reentrant.c c10_3_*.c c10_4_*.c \
         c10_5_*.c c10_6_*.c c10_7_*.c c10_8_*.c ex10_*.c; do
    gcc -O0 -Wall -Wextra -o "${f%.c}" "$f" || echo "FAIL $f"
done
```

原书程序（都需要 `tlpi_hdr.h`）：

```bash
gcc -O0 -Wall -Wextra -Wno-unused-parameter -o calendar_time tlpi_hdr.h calendar_time.c
gcc -O0 -Wall -Wextra -Wno-unused-parameter -o show_time     tlpi_hdr.h show_time.c
gcc -O0 -Wall -Wextra                      -o process_time  tlpi_hdr.h process_time.c
gcc -O0 -Wall -Wextra -D_XOPEN_SOURCE=700  -o strtime       tlpi_hdr.h strtime.c
```

`c10_2_curr_time_demo` 需要 `curr_time.c`：

```bash
gcc -O0 -Wall -Wextra -o c10_2_curr_time_demo curr_time.h curr_time.c c10_2_curr_time_demo.c
```

### 为什么一律 `-O0`

| 原因 | 说明 |
|------|------|
| `c10_6_coarse_cost.c` | `-O2` 会把 `nop_call()` 和好几处读时钟**内联或消除**，测出来全是 0 |
| 其他 demo | `-O0` 让 gcc 的行号与源码对齐，便于读者按行号对照 |

`-Wall -Wextra` 下 **0 告警**（`-Wno-unused-parameter` 只给原书程序加，因为原书很多 `main` 收了参数没用）。

### 为什么不需要 `-lrt`

`clock_gettime` / `clock_settime` 在 glibc 2.17 之后已经从 `librt` 并入 `libc`。本机 glibc 2.39，**不需要** `-lrt`。（如果为兼容老平台，加 `-lrt` 也无害。）

---

## 运行

| 命令 | 说明 | 耗时 |
|------|------|------|
| `./c10_1_calendar_probe` | 无需参数 | < 1 s |
| `./c10_2_strftime` | 无需参数 | < 1 s |
| `./c10_2_reentrant` | 无需参数 | < 1 s |
| `./c10_2_curr_time_demo` | 无需参数 | < 1 s |
| `./c10_3_timezone` | 无需参数（内部 `setenv("TZ", ...)`） | < 1 s |
| `./c10_4_locale` | 无需参数（依赖容器装了哪些 locale） | < 1 s |
| `./c10_5_setclock` | 无需参数；**非特权下跑才有意义**（写操作应全部 `EPERM`） | < 1 s |
| `./c10_6_clockres` | 无需参数；会跑 1000 万次读时钟 | ~0.4 s |
| `./c10_6_coarse_cost` | 无需参数；每项 200 万次调用 | ~0.2 s |
| `./c10_7_process_time` | 无需参数；会 fork 子进程烧 CPU | ~4 s |
| `./c10_8_clockids` | 无需参数 | < 1 s |
| `./ex10_1_clock_cycle` | 无需参数 | < 1 s |
| `./ex10_2_utc_offset` | 无需参数 | < 1 s |
| `./ex10_3_sleep_precision` | 无需参数；含 `sleep(1)` | ~1.4 s |
| `./calendar_time` | 无需参数 | < 1 s |
| `./show_time` | 无需参数 | < 1 s |
| `./strtime '2023-11-14 22:13:20\|%Y-%m-%d %H:%M:%S'` | **需要参数**（`时间串\|格式串`） | < 1 s |
| `./process_time 1000000` | **需要参数**（`getppid()` 循环次数） | ~0.2 s |

> 所有 demo 都**不读文件、不连网络、不要求特权**，唯一"环境依赖"是时区库（`/usr/share/zoneinfo`）与已安装的 locale。容器里缺 locale 时 `c10_4_locale` 会如实打印 `NULL（这台机器没装）`，不会崩。

---

## 几个必须知道的坑

1. **`c10_6_coarse_cost` 必须 `-O0`** —— `-O2` 下 `nop_call()` 会被内联成空，测出来 `0.0 ns/call`，整个基线失效。

2. **`c10_2_reentrant` 的输出里 `asctime()` 带换行** —— `asctime` / `ctime` 返回的字符串**末尾有 `\n`**，所以打印时会多空一行。这不是 bug。

3. **`c10_5_setclock` 在特权环境下的结果会不同** —— 它假设「没有 `CAP_SYS_TIME`」，四条写接口应该全部 `EPERM`。如果你给了容器 `--cap-add=SYS_TIME`，它会**真的把时钟改成 1970 年**（代码里 `clock_settime(CLOCK_REALTIME, 1970-01-01)`）。**不要在有 `CAP_SYS_TIME` 的机器上跑它**。

4. **`c10_7_process_time` 的时间数字不可迁移** —— `wall=3.388 s`、`cpu=1.852 s` 这些是 CE 容器跑出来的，换机器就变。**可迁移的是「wall 涨而 cpu 不涨」这个关系**。

5. **`ex10_2_utc_offset` 里的「错误写法」是故意的** —— 它硬编码 `+0800`，输出看着像 bug 但**就是要展示那个 bug**。别去"修"它。

6. **`c10_4_locale` 的结果依赖容器装了什么 locale** —— 本机有 `zh_CN.UTF-8` / `ja_JP.UTF-8` / `de_DE.UTF-8` / `lt_LT.UTF-8`，**没有** `fr_FR.UTF-8`。换环境输出会变，但 `setlocale` 返回 `NULL` 的分支会如实打印。

7. **`curr_time.c` 里的 `currTime()` 返回 `static` 缓冲** —— 这是**原书的设计**，不是镜像时引入的缺陷。它的教学价值就在于"第二次调用覆盖第一次"（见 `c10_2_curr_time_demo.c` 的输出）。

8. **`strtime.c` 只定义了 `_XOPEN_SOURCE` 但没给值** —— 这是原书原文。glibc 的默认值会把它补成 `700`，所以本仓库编译时额外显式加了 `-D_XOPEN_SOURCE=700`（命令行 `-D` 不会与源文件里的无值 `#define` 冲突，但**必须放在包含 `<features.h>` 之前**，命令行 `-D` 天然满足）。详见 [10.2 §7](../notes/10.2-time-conversion-functions.md)。

9. **`c10_8_clockids` 里的两个 ALARM 时钟「能读」** —— 读不需要特权，但用 `timer_create()` 挂定时器需要 `CAP_WAKE_ALARM`。demo 只读，所以不涉及。

10. **`ex10_3_sleep_precision` 的 `sleep(1)` 会让整个程序慢 1 秒** —— 如果你在批量跑，它在 `run` 表里标了 `~1.4 s`。

---

## 关于「日志里的绝对时间」

笔记里引用的实测输出中，`CLOCK_REALTIME` 的读数（如 `1789371108.831752453`）、`CLOCK_MONOTONIC` 的读数（如 `4038.158695981`）、`RAW − MONO = −32586189 ns` 这类**绝对数值**都是 CE 容器在那一次运行时的快照。

**它们不可迁移**，但在笔记里的作用是：

| 用法 | 是否可迁移 |
|------|-----------|
| 「`TAI − REALTIME ≈ 37 s`」 | ✅ 可迁移（闰秒差是全局事实） |
| 「`COARSE` 的分辨率是 `1000000 ns`」 | ✅ 可迁移（因为它 = `1/CONFIG_HZ`，而多数发行版内核是 1000） |
| 「`sysconf(_SC_CLK_TCK) = 100`」 | ✅ 可迁移（写死的 `USER_HZ`） |
| 「`CLOCK_REALTIME = 1789371108.83...`」 | ❌ 不可迁移（就是个时间戳） |
| 「`wall=3.388 s`、`cpu=1.852 s`」 | ❌ 不可迁移（依赖机器速度） |
| 「`nanosleep` 超出量 ≈ 60 µs」 | ⚠️ 量级可迁移，具体值不可 |

判断标准：**看这个数字是「定义」还是「测量」。定义类可迁移，测量类只说明量级。**
