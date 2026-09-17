# Ch24 代码索引 — Process Creation

本目录有 **22 个源文件**（另有本 README 与 `index.html`），分五类：

| 类别 | 数量 | 说明 |
|------|------|------|
| **原书镜像件** | 7 | 逐行取自 man7 官方 Ch24 分发目录 `procexec/`，未做任何改写 |
| **本仓库自写** | 7 | 原书**没有**这些程序（§24.1 的 Figure 24-1 可运行版、掩码继承 demo、两个探针、习题 24-1/24-3/24-5 的解答） |
| **公共库镜像** | 4 | `get_num.{c,h}` + `curr_time.{c,h}` |
| **替身** | 1 | `tlpi_hdr.h`（代替原书 `lib/tlpi_hdr.h` + `lib/error_functions.c`） |
| **旧 demo** | 3 | 本仓库早期随手写的小例子，保留但**不是**原书内容 |

所有程序都在 Compiler Explorer 上实跑过（gcc 13.3.0 / x86-64 / Ubuntu 24.04，沙箱内核 7.0.0-1012-aws），
笔记里引用的输出与冻结日志 `tlpi-ch24-final.txt` 逐字一致（**17 个作业**）。

---

## 一、原书镜像件（7 个，逐字取自 man7 官方 `procexec/`）

| 文件 | 行数 | 出处 | 对应节 | 备注 |
|------|------|------|--------|------|
| `t_fork.c` | 46 | **Listing 24-1** | 24.2 | 父子各拿一份 `stack`/`data` 副本；含 `sleep(3)`（实测 `execTime = 3020`） |
| `fork_file_sharing.c` | 76 | **Listing 24-2** | 24.2.1 | 共享 open file description：先看共享 offset，再切 `O_APPEND` 对照 |
| `footprint.c` | 69 | **Listing 24-3** | 24.2.2 | 通过 `sbrk()` 读堆顶，观察子进程调 `func()` 不改父进程内存足迹；接受命令行参数 |
| `t_vfork.c` | 40 | **Listing 24-4** | 24.3 | vfork 两条语义（共享内存 + 父挂起）；⚠️ 里面的 `sleep(3)` 是**刻意的演示性违规** |
| `fork_whos_on_first.c` | 54 | **Listing 24-5** | 24.4 | 父子竞态；`[num-children]` 与 `--help` 两个分支 |
| `fork_sig_sync.c` | 91 | **Listing 24-6** | 24.5 | 信号同步样板；**`sigprocmask(SIG_BLOCK)` 在 `fork()` 之前** |
| `vfork_fd_test.c` | 43 | **习题 24-2 的官方解答** | 24.7 | 证明 vfork 之后 fd 表仍是独立的（父进程连关两次 stdout） |

> ⚠️ **同目录的 `procexec/fork_stdio_buf.c` 不属于本章** —— 官方把它归 **Ch25 的 Listing 25-2**。
> 本仓库 `code/fork_stdio_buf.c` 是旧 demo，**没有**在 Ch24 笔记里被引用。

> 📌 **两处「印刷版 vs 分发版」差异**（已在笔记里标注，不是本仓库改的）：
> - `t_vfork.c` / `vfork_fd_test.c` 的 `_BSD_SOURCE` 定义块，在当前分发版里把跨行注释
>   排在了 `#define` 之后（`#define _BSD_SOURCE     /* To get vfork() declaration from <unistd.h>` + 续行）。
>   ⚠️ 这个写法会让「把宏整行提升到文件顶部」的拼接工具出错 —— 本仓库的 `gen_ce_single.py`
>   就为此加了一个 `hoist_comment` 状态机（见第八节）。
> - `fork_whos_on_first.c` 的 `setbuf(stdout, NULL)` 注释文案在各版本间有过措辞调整。

官方分发地址形如 `https://man7.org/tlpi/code/online/dist/procexec/<name>`。

---

## 二、本仓库自写（7 个，原书没有）

这 7 个文件**都不是原书内容**，文件头里都有显式声明。

| 文件 | 行数 | 补的是哪一节 | 为什么必须自写 |
|------|------|-------------|---------------|
| `probe24.c` | 58 | 全章（探针） | **每章固定动作**：把「CE 沙箱事实」一次性问清楚，免得正文出现凭印象写的数字。本章实测：`PATH` **为空**、`/bin/*` 与 `/usr/bin/*` 全部 `access(X_OK) = -1`、唯一可执行绝对路径是 `/proc/self/exe`、`_SC_OPEN_MAX = 100`、`_SC_CHILD_MAX = 54956`、`RLIMIT_CORE = 0/0` |
| `probe24_sched.c` | 84 | 24.4 | **§24.4 原书给了一个 `/proc` 路径就当答案了**（*"can be changed by assigning a nonzero value to ... sched_child_runs_first"*）。这个探针把那个文件在实测内核上的**真实状态**问出来：**`ENOENT`**（同目录其它 4 个 `sched_*` 开关都在）；顺带拿到 `nice = 19`、`SCHED_OTHER`、`/proc/version = 7.0.0-1012-aws` |
| `c24_lifecycle.c` | 111 | 24.1 | **§24.1 原书一个 Listing 都没有**（只有 Figure 24-1 那张图）。本程序把 `fork → exit → wait → execve` 四个调用跑成可观测输出，并证明 **`execve` 换映像但不换 PID**（`pid=4` 执行前后都是 4） |
| `c24_sigmask_inherit.c` | 102 | 24.2（延伸） | **原书 Ch24 正文没有这条**（它在 Ch33 讲线程时才展开）。实测三行结论：**掩码继承**（父 1 / 子 1）、**pending 不继承**（父 1 / 子 0）、父进程自己的 pending 不受 fork 影响 |
| `ex24_1_fork_count.c` | 100 | 习题 24-1 | 原书**没有给这题的答案**。用一根**在任何 `fork()` 之前建好的管道**把 7 个新进程的 pid/ppid 收回来 —— 顺便观察到 **`ppid=1` 的孤儿收养**现象 |
| `ex24_3_core_dump.c` | 130 | 习题 24-3 | 原书**没有给这题的答案**。思路是 **`fork` 一个子进程让它崩**（子进程的地址空间是父进程那一刻的副本）⇒ 父进程 `wait()` 收尸后继续跑。顺带实测出「`WCOREDUMP = 128` 却**没有** core 文件」的完整因果链 |
| `ex24_5_fork_sig_sync2.c` | 122 | 习题 24-5 | 原书**没有给这题的答案**。把 Listing 24-6 扩成**双向握手**：`SIGUSR1`（子→父）+ `SIGUSR2`（父→子），**两个信号都在 `fork()` 之前屏蔽** |

### 编译要点

```bash
# 四个自包含（只依赖 libc）
gcc -O0 -Wall -Wextra -o probe24            probe24.c
gcc -O0 -Wall -Wextra -o probe24_sched      probe24_sched.c
gcc -O0 -Wall -Wextra -o c24_lifecycle      c24_lifecycle.c
gcc -O0 -Wall -Wextra -o c24_sigmask_inherit c24_sigmask_inherit.c
gcc -O0 -Wall -Wextra -o ex24_1_fork_count  ex24_1_fork_count.c
gcc -O0 -Wall -Wextra -o ex24_3_core_dump   ex24_3_core_dump.c

# 需要 curr_time.c（打时间戳）
gcc -O0 -Wall -Wextra -I. -o ex24_5_fork_sig_sync2 \
    ex24_5_fork_sig_sync2.c curr_time.c
```

> ⚠️ **`probe24.c` / `probe24_sched.c` / `c24_lifecycle.c` / `c24_sigmask_inherit.c` /
> `ex24_1_fork_count.c` / `ex24_3_core_dump.c` 都不 include `tlpi_hdr.h`** —— 它们只用 libc，
> 因此可以在**任何**环境里编（不依赖替身头）。
>
> ⚠️ **`c24_lifecycle.c` 演示 `exec` 时用的是 `execl("/proc/self/exe", ...)`**，不是
> `execlp("echo", ...)`。原因见第六节：CE 沙箱 `PATH` 为空、`/bin/echo` 不存在。

---

## 三、公共库镜像（4 个）与旧 demo（3 个）

### 公共库（4 个）

| 文件 | 行数 | 来源 | 谁在用 |
|------|------|------|--------|
| `get_num.c` | 102 | 原书 `lib/get_num.c` | `getInt()` / `getLong()`；解析失败时调 `cmdLineErr()`（替身头里已提供） |
| `get_num.h` | 32 | 原书 `lib/get_num.h` | 声明 `getInt()` / `getLong()` / `GN_*` 常量 |
| `curr_time.c` | 42 | 原书 `time/curr_time.c` | `currTime()` 返回 `"HH:MM:SS"` |
| `curr_time.h` | 20 | 原书 `time/curr_time.h` | 声明 `currTime()` |

依赖边界（已逐个核对）：`footprint` / `fork_whos_on_first` / `fork_sig_sync` / `vfork_fd_test` 用 `getInt()`；
`fork_sig_sync` / `ex24_5_fork_sig_sync2` 额外用 `currTime()`。
`t_fork` / `fork_file_sharing` / `t_vfork` 其实**不调用** `get_num.c` 里的任何函数，但原书把它们放在同一个编译单元里，本仓库照做。

### 旧 demo（3 个，**不是**原书内容）

| 文件 | 行数 | 说明 |
|------|------|------|
| `fork_basic.c` | 38 | 返回值 / PID / COW 全局变量 —— 内容与 Listing 24-1 重叠 |
| `fork_stdio_buf.c` | 38 | 缓冲重复输出 / `fflush` —— ⚠️ **这个主题官方归 Ch25 的 Listing 25-2** |
| `fork_fd_offset.c` | 53 | 父子共享文件偏移 —— 内容与 Listing 24-2 重叠 |

> ⚠️ 这三个文件保留是为了不破坏旧链接，但**Ch24 笔记里一个新引用都没有**。
> 学 Ch24 请只看第一节那 7 个官方镜像件。

---

## 四、替身：`tlpi_hdr.h`（1 个）

原书结构是「`lib/tlpi_hdr.h` 声明 + `lib/error_functions.c` 实现」两层，本仓库都没有随附。
这里为**单文件可控**把实现做成 `static inline` 放在头里，**报文格式逐字对齐官方**。

> 📌 本文件与 [Ch23 的 `tlpi_hdr.h`](../../chapter-23-timers-sleeping/code/tlpi_hdr.h)
> **是同一份**（sha256 相同：`1d1dedf4…`），因为它在 CE 上被拼进同一个翻译单元。

### 替身差异表

| # | 差异 | 影响 |
|---|------|------|
| 1 | **`ename[]` 表缺失**（原书那张表由 `lib/build_ename.sh` 从 `errno.h` **生成**）。`outputError()` 里 `[%s %s]` 的第一个 `%s` 退化成 `?UNKNOWN?` | ⚠️ **本章实测能撞到**：`vfork_fd_test` 的 stderr 就是<br>原书：`ERROR [EBADF Bad file descriptor] close`<br>替身：`ERROR [?UNKNOWN? Bad file descriptor] close`<br>笔记里已按替身的实际输出引用 |
| 2 | **`usageErr` 的文案与官方逐字相同**（`"Usage: "`），且本章会真的触发 | 实测 `whos_first_help`：`Usage: ./output.s [num-children]`，`exit code = 1` |
| 3 | `Boolean` 照抄官方（先 `#undef TRUE/FALSE` 再 `typedef enum { FALSE, TRUE }`） | 无差异 |
| 4 | `outputError()` 里那条会被 gcc 报 `-Wformat-truncation` 的 `snprintf`，**同样**用 `#pragma GCC diagnostic push/ignored/pop` 压掉 | 否则会凭空多出一条**官方代码里没有的**警告 |
| 5 | 提供 `errMsg` / `errExit` / `errExitEN` / `fatal` / `usageErr` / `cmdLineErr` / `terminate` 七个 | 本章只用到 `errExit` / `errMsg` / `usageErr` |
| 6 | 官方头里那几段**平台兼容块**（`socklen_t` / `FASYNC→O_ASYNC` / `MAP_ANON→MAP_ANONYMOUS` / `O_FSYNC→O_SYNC` / `__FreeBSD__` 的 sigval 别名）按需省略 | 这些在 Linux/glibc 上全是**空操作**；本章无程序用到 |
| 7 | **不带 `signal_functions.h`**（原书的 `printSigMask` 等） | 本章没有程序用 |
| 8 | `min` / `max` 宏保留（与官方头对齐），但**本章没有程序调用** | 无影响 |

> ⚠️ **与 Ch10–Ch22 的同名替身不通用，且报文格式不同**：那一批把 `errExit` 写成
> `msg: strerror` 直出（**没有** `ERROR` 前缀、**没有** `[...]` 段）；本 Ch24 份按官方格式
> 输出 `ERROR [...]`。各章笔记引用的报错文本因此不同，**别跨章对照**。

---

## 五、官方编译参数（必须照用，否则会误判「代码有问题」）

原书 `Makefile.inc` 的 `IMPL_CFLAGS`：

```bash
-std=c99 -D_XOPEN_SOURCE=600 -D_DEFAULT_SOURCE \
-pedantic -Wall -W -Wmissing-prototypes -Wimplicit-fallthrough -Wno-unused-parameter
```

本仓库在 CE 上跑的时候用的是 `-O0 -Wall -Wextra`，所以**在 CE 上看到的下面这些警告都不是原书代码的问题**：

| CE 上会看到的警告 | 真实原因 |
|------------------|---------|
| `warning: unused parameter 'argc'` / `'argv'` | 官方 `IMPL_CFLAGS` 里有 `-Wno-unused-parameter`，CE 上没加 |
| `expected '=', ',', ';', 'asm' or '__attribute__' before '>=' token`（**编译错误**，不是警告） | ⚠️ **不是原书代码的问题** —— 是本仓库的**拼接工具**踩到的坑，见第八节 |

### 链接参数

| 事项 | 结论 |
|------|------|
| `-lrt` / `-lpthread` | **都不需要**。本章只用 `fork`/`vfork`/`wait`/`kill`/`sigprocmask`/`sigsuspend`，全在 `libc` |
| `-I.` | 原书镜像件都要（找替身的 `tlpi_hdr.h`）；7 个自写件里只有 `ex24_5_fork_sig_sync2.c` 需要（它 include `curr_time.h`） |
| `-D_BSD_SOURCE` | ⚠️ **由源码自己定义**（`t_vfork.c` / `vfork_fd_test.c` 顶部），**不要在命令行再加** |

> 📌 本仓库笔记里所有 CE 编译诊断的行号都是按上面这套参数跑出来的。

---

## 六、沙箱（Compiler Explorer 容器）的硬限制

本章 demo 全部在 CE 上跑过。容器里**做不到**的事，笔记里都做了诚实标注，
不要把它们当成「代码写错了」。⭐ 打星号的几条是**本章新增**的发现。

| 限制 | 观察到的事实 | 受影响的 demo |
|------|-------------|--------------|
| ⭐ **`PATH` 为空** | `getenv("PATH")` 返回 NULL / 空串 ⇒ **任何 `execlp` / `execvp` 都会失败**。实测 `c24_lifecycle` 最初写 `execlp("echo", ...)` 直接 `ERROR: execlp(echo): No such file or directory` | `c24_lifecycle.c`（改用 `/proc/self/exe`） |
| ⭐ **`/bin/*` 与 `/usr/bin/*` 全部不存在** | `access("/bin/echo", X_OK) = -1` … 五个候选路径全 −1；**唯一可执行绝对路径是 `/proc/self/exe`** | 任何要 `exec` 的演示 |
| ⭐ **`nice = 19`**（最低优先级）+ **2 个 CPU** + 共享宿主 | 这解释了 §24.4 实测为什么是 **91.6% / 8.4%** 而不是接近 100%。⇒ **「本机实测 xx%」必须连同调度环境一起说** | `fork_whos_on_first.c`（1000 次） |
| ⭐ **容器 PID namespace** | pid 从 **2** 开始（1 是 init）；`ppid=1` 就是「被 init 收养」 | 全部 demo |
| ⭐ **`RLIMIT_CORE = 0 / 0`**，且 hard limit **提不动** | `setrlimit(RLIMIT_CORE, INFINITY/INFINITY)` → `EPERM`；把 soft 抬到 hard（=0）后**还是 0** ⇒ **写不出 core 文件** | `ex24_3_core_dump.c` |
| ⭐ **`core_pattern` 是管道** | `\|/usr/share/apport/apport -p%p -s%s -c%c ...`。管道模式下内核**根本不看 `RLIMIT_CORE`**（`fs/coredump.c:613` 直接设成 `RLIM_INFINITY`）⇒ `WCOREDUMP` 位被置上，但文件不落地 | `ex24_3_core_dump.c` |
| **每个作业最长 20 秒** | 超时被 SIGKILL（`exit 143`）。⚠️ 本章**没有**作业触发它 —— 最长的 `t_vfork` 是 3027 ms（`sleep(3)`） | 无 |
| **单次执行的 stdout 采集上限约 32 KB** | 超限时 CE 插入 `[Truncated]` 并停止采集。本章最长的 `whos_first_1000` 是 2000 行 ≈ **16 KB**，**没有** `[Truncated]` | `fork_whos_on_first.c`（1000 次） |
| **stdout 是 `SOCKET` 不是 pipe** | glibc 用 **8 KB 全缓冲**（不是行缓冲） | ⚠️ 本章**所有**要观察顺序的程序都显式关缓冲（`setbuf(stdout, NULL)` 或 `setvbuf(..., _IONBF, 0)`） |
| **stdin 是已关闭的管道** | `read(0, ...)` 立刻返回 **0（EOF）** | 本章无程序读 stdin |
| **`argv[0]` 恒为 `./output.s`** | CE 给产物起的名字。`usageErr` 打的是 `argv[0]` ⇒ 用法行显示 `./output.s` | `whos_first_help` |
| **CE 每个作业是独立容器，宿主可能不同** | 同章两次跑可能落在不同宿主上 | 全部 demo：**只在同一次运行内比较** |
| **单次极值 / 计数类测量会漂** | §24.4 的 child-先次数与位置、各作业 `execTime` | 笔记里凡标「本次」的数字 |

> ⚠️ 因此：所有「行数 / 次数 / 百分比」必须读作「**日志中可见的**」而不是「程序保证的」。
> 本章笔记已在 §24.4 显式声明这一点。

---

## 七、文件校验（sha256）

```text
57da2099c0f0c2aca0ed4b1c4aecce4ddb184804ffe1f83fc30b1d6d058a669c  c24_lifecycle.c
cb100ed14e156a086dc9a10c16d30e46b987a32575bf5b6a1a18b17f5a1dfd38  c24_sigmask_inherit.c
d06bb3fd8e439e33c386c8c67e434e67927c2ce021b5b3b50fc26c0ef645bbcf  curr_time.c
941c78e6894cb15b7f5d937dfad76644103b099e31f710cf16719c12fbdeb3cf  curr_time.h
49e87c018ca12223d4868878ad6dcbc06b4960eeffbdebd748b45d3514b2e9ae  ex24_1_fork_count.c
524ba62618b64fb7cbb80dbc436b1a4c834073ff5d710acc121032f811f58b9f  ex24_3_core_dump.c
85046b0d077f4cc6fd06506ae8640f20e869c79e4f2cad1bd86f249924da1439  ex24_5_fork_sig_sync2.c
810470258a6ca40b3e4f36fcc0f445e74046d78616598c1132af3258eb083d60  footprint.c
de1b45b61dbd67d5c7fefb477e305ee93718ba6c1a4795c80a9884f8aed058d6  fork_basic.c
2233102e992a693c45e1bab9c23407d0bed36312b50198e153bc6ed14c1867ea  fork_fd_offset.c
bb00f64bc3d5ca724bdad55d79eca27e12275a0e07f5445a36c789da48b1e62b  fork_file_sharing.c
8a87860a5fb158d62c0c04f29436555667f1d8893927ea4ea3c01405d0188c4e  fork_sig_sync.c
ad0da71d621e6f317dfef532f7e8e70dd15f10f14568b211b5181cafa78bb43a  fork_stdio_buf.c
ce5fc3cf2940dfa1cd18b86c8ca09f216aeac5f4c50b2682b422432c6404dbe1  fork_whos_on_first.c
b6318f4d9d7d0416a574602d5fe86fa6c7af4871ec97a1900c24e5cdc4ca6724  get_num.c
e7b28952848dbd611edd8d7ab4440c18b49538ecb033704083905edfc69e8c08  get_num.h
2366149f3f0c5d21e27913bdcb99e354547f579e50ec9d0b18715ad5adfa6ff7  probe24.c
2302d30dc1beba7343d4988495a280b93fbf82810f6957c90a0c2207f6249da3  probe24_sched.c
a05636e96dacef35a97f67627ed0e51f2878ec0caccdf25e5b070a51ae15c00b  t_fork.c
9d44c6251921f9ac4b9679532083cb820fbb78694599ed7828a95dbc3b62686a  t_vfork.c
1d1dedf473a0d91e60b03b7583e0fb52dbf82dcf6ed274bef354992e5a186327  tlpi_hdr.h
053e60dfa14ffed48ef05156a555be29465ebe4fb3a45de032c7d79d0a9625de  vfork_fd_test.c
```

> 笔记里的内联代码与本目录源码**逐行一致**（由 `cmp_note_code.py` 校验，本章 **12 处**全过），
> 所以笔记里引用的 `gcc` 行号读者能对上。

---

## 八、怎么用这套代码

### 1. 本地编译（用 `-I.` 找替身头）

```bash
# 原书镜像件（都要 -I.）
for f in t_fork.c fork_file_sharing.c footprint.c t_vfork.c \
         fork_whos_on_first.c fork_sig_sync.c vfork_fd_test.c; do
    gcc -O0 -Wall -Wextra -I. -o "${f%.c}" "$f" get_num.c curr_time.c \
        || echo "FAILED: $f"
done

# 自写件（前六个不需要替身头）
gcc -O0 -Wall -Wextra -o probe24 probe24.c
gcc -O0 -Wall -Wextra -o probe24_sched probe24_sched.c
gcc -O0 -Wall -Wextra -o c24_lifecycle c24_lifecycle.c
gcc -O0 -Wall -Wextra -o c24_sigmask_inherit c24_sigmask_inherit.c
gcc -O0 -Wall -Wextra -o ex24_1_fork_count ex24_1_fork_count.c
gcc -O0 -Wall -Wextra -o ex24_3_core_dump ex24_3_core_dump.c
gcc -O0 -Wall -Wextra -I. -o ex24_5_fork_sig_sync2 ex24_5_fork_sig_sync2.c curr_time.c
```

### 2. 按 24.1 → 24.5 的顺序跑

```bash
./probe24                 # 先问清沙箱事实（PATH 空、可执行路径、rlimit）
./c24_lifecycle           # §24.1：fork/exit/wait/execve 一圈；注意「exec 不换 PID」
./t_fork                  # §24.2：父子各一份数据（要等 3 秒）
./fork_file_sharing       # §24.2.1：共享 offset / O_APPEND
./footprint               # §24.2.2：内存足迹
./c24_sigmask_inherit     # §24.2 延伸：掩码继承 / pending 不继承
./t_vfork                 # §24.3：父进程被挂起 3 秒；istack=666
./vfork_fd_test           # §24.7 习题 24-2（⚠️ 输出在 stderr）
./fork_whos_on_first 1000  # §24.4：2000 行，看 91.6/8.4 的分布
./probe24_sched           # §24.4：那个开关在不在（本机可能还在，看内核版本）
./fork_sig_sync           # §24.5：信号同步（约 2 秒）
./ex24_1_fork_count       # §24.7 习题 24-1
./ex24_3_core_dump        # §24.7 习题 24-3
./ex24_5_fork_sig_sync2   # §24.7 习题 24-5（约 3 秒）
```

**建议的顺序**：先 `probe24` 建立环境画像（没有它，后面所有「找不到程序」「core 不见了」
都会被误判成代码 bug），再 `c24_lifecycle` 把四个调用串一遍，然后 `t_fork` /
`fork_file_sharing` / `footprint` / `c24_sigmask_inherit` 看 fork 的三个层面，
接着 `t_vfork` + `vfork_fd_test` 看 vfork 的对照，最后 `fork_whos_on_first` 看竞态、
`fork_sig_sync` 看解法。习题放最后。

### 3. ⚠️ 在 CE 上跑时必须用「拼接」而不是多文件编译

CE 的单次请求只能有一个翻译单元，本仓库的做法是把
`get_num.h + tlpi_hdr.h + get_num.c + <主程序>` 用一个脚本拼成**一个 `.c`** 再上传
（脚本 `D:\.kernel-ref\gen_ce_single.py`，驱动脚本 `run_ch24.py`）。

拼接时踩过两个坑，都记在这里：

| 坑 | 现象 | 修法 |
|----|------|------|
| ⭐ **跨行注释被「宏提升」切断** | `#define _BSD_SOURCE     /* To get ... `<br>`in case _XOPEN_SOURCE >= 600 */`<br>把宏**整行**提到文件顶部后，注释的 `/*` 没闭合，**吞掉了紧随其后的 `tlpi_hdr.h`**，原地留下裸文本 ⇒ 编译报 `expected '=', ',', ';', 'asm' or '__attribute__' before '>=' token`（看着像语法错误，其实是这行没被注释掉）。**受影响的作业：`footprint` / `footprint_7` / `t_vfork` / `vfork_fd_test`** | 提升宏时**只取宏名与其值**，把跨行注释整体吃掉（加一个 `hoist_comment` 状态机） |
| 局部 `#include "..."` | 拼接后这些行会重复/失效 | 拼接时直接**剔除**所有 `#include "本地头"` 行 |

> 📌 这两个坑**都不是原书代码的问题** —— 是本仓库的拼接工具的问题。原书用 `make`
> 多文件编译时不会遇到。写下这一段，是为了让「照原书参数编译却编不过」的读者不至于怀疑源码。

### 4. 三个「看着像 bug」其实是环境事实的现象

| 现象 | 真相 |
|------|------|
| `c24_lifecycle` 里 exec 用的是 `/proc/self/exe` 而不是 `echo` | CE 沙箱 `PATH` 为空、`/bin/echo` 不存在（`probe24` 实测） |
| `vfork_fd_test` 打出 `ERROR [?UNKNOWN? Bad file descriptor] close` | ① `[?UNKNOWN?]` 是替身缺 `ename[]`（第四节）；② **只打一行是正确结果** —— 父进程第一次 `close(1)` 成功、第二次才失败，正好证明 fd 表独立 |
| `ex24_3_core_dump` 说 `WCOREDUMP = 128` 却找不到 core 文件 | `RLIMIT_CORE = 0/0`（且提不动）+ `core_pattern` 是管道（第六节） |

---

## 九、与笔记的对应

| 节 | 笔记 | 程序 |
|----|------|------|
| 24.1 | [`notes/24.1-overview-of-fork-exit-wait-and-execve.md`](../notes/24.1-overview-of-fork-exit-wait-and-execve.md) | `c24_lifecycle.c`（**原书无 Listing**）+ `probe24.c` |
| 24.2 | [`notes/24.2-creating-a-new-process-fork.md`](../notes/24.2-creating-a-new-process-fork.md) | `t_fork.c`（L24-1）+ `fork_file_sharing.c`（L24-2）+ `footprint.c`（L24-3）+ `c24_sigmask_inherit.c` |
| 24.3 | [`notes/24.3-the-vfork-system-call.md`](../notes/24.3-the-vfork-system-call.md) | `t_vfork.c`（L24-4） |
| 24.4 | [`notes/24.4-race-conditions-after-fork.md`](../notes/24.4-race-conditions-after-fork.md) | `fork_whos_on_first.c`（L24-5）+ `probe24_sched.c` |
| 24.5 | [`notes/24.5-avoiding-race-conditions-by-synchronizin.md`](../notes/24.5-avoiding-race-conditions-by-synchronizin.md) | `fork_sig_sync.c`（L24-6）+ `ex24_5_fork_sig_sync2.c` |
| 24.6 | [`notes/24.6-summary.md`](../notes/24.6-summary.md) | 总账（无专属程序） |
| 24.7 | [`notes/24.7-exercises.md`](../notes/24.7-exercises.md) | `vfork_fd_test.c`（官方解 24-2）+ `ex24_1_fork_count.c` + `ex24_3_core_dump.c` + `ex24_5_fork_sig_sync2.c` |
