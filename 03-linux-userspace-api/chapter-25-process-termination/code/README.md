# Ch25 代码索引 — Process Termination

本章 `code/` 下共 **16 个源文件** + 本文件（+ 由 `build_all.py` 生成的 `index.html`）：

| 类别 | 个数 | 文件 |
|------|------|------|
| 原书镜像件（官方 `procexec/`） | **2** | `exit_handlers.c` · `fork_stdio_buf.c` |
| 本仓库自写 | **8** | `probe25.c` · `c25_exit_three_steps.c` · `c25_fall_off_main.c` · `c25_exit_handler_no_return.c` · `c25_atexit_reentry.c` · `c25_exec_clears_handlers.c` · `c25_fork_stdio_three_fixes.c` · `ex25_1_exit_minus_one.c` |
| 公共库镜像（TLPI `lib/`） | **3** | `tlpi_hdr.h`（替身）· `get_num.c` · `get_num.h` |
| 旧 demo（早于补代码工程） | **3** | `atexit_order.c` · `exit_vs_exit.c` · `fork_atexit.c` |

> ⚠️ **自写件不是原书内容。** 按本仓库惯例，8 个自写件的**文件头注释**里都写明了
> 「⚠️ 延伸 demo —— **原书没有这个程序**」或「本仓库自写」，笔记正文里也逐条声明。
> **不伪造节号**：原书没有哪一节对应它们，所以文件名里不带 `c25_<节号>_` 这种伪节号。

---

## 一、原书镜像件（2 个，逐字取自 man7 官方 `procexec/`）

来源：`https://man7.org/tlpi/code/online/dist/procexec/<file>`（**不带 `.html` 后缀才是原始字节**；
带 `.html` 的是渲染页，会丢空行）。

| 文件 | 原书 Listing | 行数 | 说明 |
|------|-------------|------|------|
| `exit_handlers.c` | **Listing 25-1** | 64 | `atexit()` + `on_exit()` 交错注册；`on_exit` 那半被 `#ifdef __linux__` 包住（*"Few UNIX implementations have on_exit()"*）；`main` 最后 `exit(2)` |
| `fork_stdio_buf.c` | **Listing 25-2** | 29 | 三行主体：`printf` → `write` → `fork` → 父子都 `exit()` |

⚠️ **`fork_stdio_buf.c` 在 Ch24 的官方分发目录里也出现过**，但官方把它归 **Ch25 的 Listing 25-2**
（Ch24 README 里已点名）。本轮正式收进本章。

> ⚠️ 官方源码顶部**自带** `#define _BSD_SOURCE`（`exit_handlers.c:18`），
> **不要在命令行再加**。

---

## 二、本仓库自写（8 个，原书没有）

按「对应节」排序：

| 文件 | 对应节 | 演示什么 | 关键实测 |
|------|--------|---------|---------|
| `probe25.c` | 全章 | **探针**（每章固定动作）：`sysconf(_SC_ATEXIT_MAX)`、连续注册 20 万个 handler、stdout 是什么、`EXIT_SUCCESS/FAILURE`、rlimit、`PATH`/`argv[0]` | `_SC_ATEXIT_MAX = 2147483647`；20 万个全成功；`S_ISSOCK=1` / `isatty(1)=0` |
| `c25_exit_three_steps.c` | **25.1** | 用 `write(2)` vs `printf(3)` 把 `exit()` 三步的**顺序**做成可观察量 | `W4` 出现在 `M3` **之前** ⇒ flush 在所有 handler 之后 |
| `c25_fall_off_main.c` | **25.1** | 同一源文件编 `-std=c89` / `-std=c99` 两遍，看「掉出 `main` 末尾」的分水岭 | c89 ⇒ 退出码 **42**（+ `-Wreturn-type`）；c99 ⇒ **0**（无警告） |
| `c25_exit_handler_no_return.c` | **25.3** | handler 里 `_exit()` vs `exit()` 的**两种相反后果**并排跑（各放一个子进程，父进程收尸报数） | `_exit(7)` ⇒ 剩余 handler 与 flush 全取消；`exit(9)` ⇒ 照常 |
| `c25_atexit_reentry.c` | **25.3** | 验证原书那句 *"placed at the head of the list of exit handlers that remain to be called"* | 顺序 **C → B → D → A** ⇒ **插队首** |
| `c25_exec_clears_handlers.c` | **25.3** | 验证"`exec()` 清掉全部注册"。用 `/proc/self/exe`（沙箱里 `/bin` 与 `/usr/bin` **整个不存在**），不 `fork`、PID 不变 | `exec` 前注册的 `oldHandler` **没有执行**；`exec` 后新注册的跑了（对照组） |
| `c25_fork_stdio_three_fixes.c` | **25.4** | 一个源文件 **4 种模式**（`argv[1]`）：模式 0 = 原书原样（**病**）；1 = `fflush` 前置；2 = 子进程 `_exit`；3 = `setbuf(stdout, NULL)`（三种**药**） | 模式 3 是 CE 上**唯一**能间接展示「类终端顺序」的模式 |
| `ex25_1_exit_minus_one.c` | **25.6 习题 25-1** | 16 个边界值（`-1 / -2 / -256 / -1000 / 0 / 1 / 2 / 127 / 128 / 255 / 256 / 257 / 300 / 511 / 1000 / 65535`）+ 一次 `raise(SIGSEGV)` 对照 | **`exit(-1)` ⇒ `WEXITSTATUS()` = 255** |

### 编译要点

```bash
# 八个自写件全部自包含（只依赖 libc），不需要替身头、不需要 -lrt/-lpthread
gcc -O0 -Wall -Wextra -o probe25 probe25.c
gcc -O0 -Wall -Wextra -o c25_exit_three_steps c25_exit_three_steps.c
gcc -O0 -Wall -Wextra -o c25_exit_handler_no_return c25_exit_handler_no_return.c
gcc -O0 -Wall -Wextra -o c25_atexit_reentry c25_atexit_reentry.c
gcc -O0 -Wall -Wextra -o c25_exec_clears_handlers c25_exec_clears_handlers.c
gcc -O0 -Wall -Wextra -o c25_fork_stdio_three_fixes c25_fork_stdio_three_fixes.c
gcc -O0 -Wall -Wextra -o ex25_1_exit_minus_one ex25_1_exit_minus_one.c

# c25_fall_off_main 要编两遍
gcc -O0 -Wall -Wextra -std=c89 -o c25_fall_off_c89 c25_fall_off_main.c
gcc -O0 -Wall -Wextra -std=c99 -o c25_fall_off_c99 c25_fall_off_main.c
```

⚠️ **`c25_fork_stdio_three_fixes` 的四种模式必须在「stdout 不是终端」时看** ——
也就是**重定向到文件或管道**时，例如 `./c25_fork_stdio_three_fixes 0 | cat`。
在真终端上 stdout 是行缓冲，四种模式看不出差别。

⚠️ **`c25_exit_handler_no_return.c` / `ex25_1_exit_minus_one.c` 里有 `fork()`**，
运行时会看到父/子两边的输出**交错**（取决于调度）。这是正常的，不是 bug。

---

## 三、公共库镜像（3 个）与旧 demo（3 个）

### 公共库（3 个）

| 文件 | 说明 |
|------|------|
| `get_num.c` + `get_num.h` | `getInt()` / `getLong()`（来自 TLPI `lib/`）。**本章两个官方件里只有 `exit_handlers.c` 需要它**（因为替身头没提供 `error_functions`，官方件靠它构成完整编译单元；实际上 8 个自写件都不用它） |
| `tlpi_hdr.h` | **替身**，见第四节 |

### 旧 demo（3 个，**不是**原书内容，早于本仓库"补代码"工程）

这三个是本章骨架阶段随手写的教学片段，**保留作历史对照**，不列入自写件清单：

| 文件 | 字节 | 说明 |
|------|------|------|
| `atexit_order.c` | 557 | 回调 LIFO 的最小演示 |
| `exit_vs_exit.c` | 594 | `exit` 刷缓冲 vs `_exit` 不刷（`argv[1]` 选） |
| `fork_atexit.c` | 1113 | fork 之后 `exit`/`_exit` 与 atexit 的关系 |

> 📌 它们的内容已被自写件**完全覆盖**（`c25_atexit_reentry` / `c25_exit_three_steps` /
> `c25_fork_stdio_three_fixes`），但**删除会破坏历史读者的链接**，所以保留。

---

## 四、替身：`tlpi_hdr.h`（1 个）

原书结构是「`lib/tlpi_hdr.h` 声明 + `lib/error_functions.c` 实现」两层，本仓库都没有随附。
这里为**单文件可控**把实现做成 `static inline` 放在头里，**报文格式逐字对齐官方**。

> 📌 本文件与 [Ch24 的 `tlpi_hdr.h`](../../chapter-24-process-creation/code/tlpi_hdr.h)
> **是同一份**（sha256 相同：`1d1dedf4…`），因为它在 CE 上被拼进同一个翻译单元。

### 替身差异表

| # | 差异 | 本章影响 |
|---|------|---------|
| 1 | **`ename[]` 表缺失**（原书那张表由 `lib/build_ename.sh` 从 `errno.h` **生成**）。`outputError()` 里 `[%s %s]` 的第一个 `%s` 退化成 `?UNKNOWN?` | ⚠️ **本章不会撞到** —— 本章没有任何作业走到 `errExit` 的错误分支（`exit_handlers` 走的是 `fatal()`，而它只在注册失败时触发） |
| 2 | `usageErr` 的文案与官方逐字相同 | 本章无作业使用命令行参数校验 |
| 3 | `Boolean` 照抄官方（先 `#undef TRUE/FALSE` 再 `typedef enum`） | 无差异 |
| 4 | `outputError()` 里那条会触发 `-Wformat-truncation` 的 `snprintf`，**同样**用 `#pragma GCC diagnostic push/ignored/pop` 压掉 | 否则会凭空多出一条**官方代码里没有的**警告 |
| 5 | 提供 `errMsg` / `errExit` / `errExitEN` / `fatal` / `usageErr` / `cmdLineErr` / `terminate` 七个 | ⭐ **本章只用到 `fatal()`** —— 就是 `exit_handlers.c` 里那四处 `fatal("on_exit 1")` 等 |
| 6 | 官方头里那几段**平台兼容块**（`socklen_t` / `FASYNC→O_ASYNC` / `MAP_ANON→MAP_ANONYMOUS` / `O_FSYNC→O_SYNC` / `__FreeBSD__` 的 sigval 别名）按需省略 | 这些在 Linux/glibc 上全是**空操作** |
| 7 | **不带 `signal_functions.h`**（原书的 `printSigMask` 等） | 本章没有程序用 |
| 8 | `min` / `max` 宏保留（与官方头对齐），但**本章没有程序调用** | 无影响 |

> ⚠️ **与 Ch10–Ch22 的同名替身不通用，且报文格式不同**：那一批把 `errExit` 写成
> `msg: strerror` 直出（**没有** `ERROR` 前缀、**没有** `[...]` 段）；本 Ch24/25 份按官方格式
> 输出 `ERROR [...]`。各章笔记引用的报错文本因此不同，**别跨章对照**。

---

## 五、官方编译参数（必须照用，否则会误判「代码有问题」）

原书 `Makefile.inc` 的 `IMPL_CFLAGS`：

```bash
-std=c99 -D_XOPEN_SOURCE=600 -D_DEFAULT_SOURCE \
-pedantic -Wall -W -Wmissing-prototypes -Wimplicit-fallthrough -Wno-unused-parameter
```

本仓库在 CE 上用的是 `-O0 -Wall -Wextra`，所以**在 CE 上看到的下面这些警告都不是原书代码的问题**：

| CE 上会看到的警告 | 出现在 | 真实原因 |
|------------------|--------|---------|
| `#warning "_BSD_SOURCE and _SVID_SOURCE are deprecated, use _DEFAULT_SOURCE"` | `exit_handlers` | `features.h:196` 里的 `#warning`，由**官方源码顶部的 `#define _BSD_SOURCE`** 触发。**原书写作时这不是警告**（glibc 后来才弃用）。**功能不受影响**（报警后自动定义 `_DEFAULT_SOURCE`） |
| `warning: unused parameter 'argc'` / `'argv'` | `exit_handlers` · `fork_stdio_buf` | 官方 `IMPL_CFLAGS` 里有 `-Wno-unused-parameter`，CE 上没加 |
| `warning: control reaches end of non-void function [-Wreturn-type]` | `c25_fall_off_c89` | ⚠️ **这不是噪声，是本节要讲的现象本身** —— `main` 没有 `return`，正是 §25.1 那个 C89 未定义行为的触发器 |

**诊断条数对照**（本仓库刻意**不修**官方件的 warning，以保持逐字镜像）：

| 作业 | `diagnostics` | 内容 |
|------|--------------|------|
| `exit_handlers` | **12** | `-Wcpp` 1 条 + `-Wunused-parameter` 2 条 + 位置引用行 |
| `fork_stdio_buf` | **7** | `-Wunused-parameter` 2 条 + 位置引用行 |
| `c25_fall_off_c89` | **4** | `-Wreturn-type` 1 条 + 位置引用行 |
| 其余 11 个作业 | **0** | —— |

### 链接参数

| 事项 | 结论 |
|------|------|
| `-lrt` / `-lpthread` | **都不需要**。本章只用 `fork` / `exit` / `atexit` / `on_exit` / `write` / `waitpid`，全在 `libc` |
| `-I.` | **只有两个官方镜像件需要**（找替身的 `tlpi_hdr.h`）；8 个自写件都不需要 |
| `-D_BSD_SOURCE` | ⚠️ **由源码自己定义**（`exit_handlers.c:18`），**不要在命令行再加**；新代码建议改用 `-D_DEFAULT_SOURCE` |

> 📌 本仓库笔记里所有 CE 编译诊断的行号都是按上面这套参数跑出来的。

---

## 六、沙箱（Compiler Explorer 容器）的硬限制

本章 14 个作业全部在 CE 上跑过。容器里**做不到**的事，笔记里都做了诚实标注，
不要把它们当成「代码写错了」。⭐ 打星号的是**本章新增**的发现。

| 限制 | 观察到的事实 | 受影响的 demo |
|------|-------------|--------------|
| ⭐ **stdout 是 `SOCKET` 不是 pipe** | `probe25` 实测 `S_ISREG=0 / S_ISCHR=0 / S_ISFIFO=0 / S_ISSOCK=1`，`isatty(1)=0` ⇒ glibc 对 stdout 用 **8 KB 全缓冲**（`BUFSIZ=8192`） | ⭐ **这一条决定了 §25.4 只能观察到一半** —— 官方 `fork_stdio_buf` 在 CE 上**直接就是原书"重定向到文件"那一半**；"终端行缓冲"那一半**物理上不可观察**。本仓库用 `c25_fork_stdio_three_fixes` 模式 3 间接补上，并标注那是**无缓冲**≠**行缓冲** |
| ⭐ **`PATH` 为空** + **`/bin` 与 `/usr/bin` 整个不存在** | `getenv("PATH")` 是空串；`argv[0]` 恒为 `./output.s` | `c25_exec_clears_handlers.c`（只能用 `/proc/self/exe` 做 exec） |
| ⭐ **`RLIMIT_CORE = 0 / 0`** | `probe25` 实测。所以"异常终止 ⇒ 产生 core"这条在本环境**拿不到 core 文件**（Ch24 已实测过 `WCOREDUMP=128` 但磁盘上没有 core） | `ex25_1_exit_minus_one.c` 的 `raise(SIGSEGV)` 那段（`WCOREDUMP=1` 但无 core） |
| ⭐ **`RLIMIT_NOFILE` / `_SC_OPEN_MAX = 100`** | §25.2 第 1 条"关 fd"的作用范围上限 | `probe25.c` |
| ⭐ **`_SC_CHILD_MAX` 会漂** | 本轮 **27191**；Ch24 那轮同一探针读到 **54956**。与 cgroup 配额 / `pid_max` 有关 | `probe25.c`（引用必须带日志出处） |
| **容器 PID namespace** | pid 从 **2** 开始（1 是 init） | 全部 demo |
| **每个作业最长 20 秒** | 超时被 SIGKILL（`exit 143`）。⚠️ 本章**没有**作业触发它（最长的 `ex25_1` 是 179 ms） | 无 |
| **单次执行的 stdout 采集上限约 32 KB** | 本章最长的输出 `ex25_1` 约 2.4 KB，**没有** `[Truncated]` | 无 |
| **CE 每个作业是独立容器，宿主可能不同** | 同章两次跑可能落在不同宿主上。⚠️ 本轮**重跑后逐字对比过**：`probe25` 与 `ex25_1` 的 stdout **一字未变**，`_SC_CHILD_MAX` 仍是 27191 | 全部 demo |
| **`diagnostics` 计数含"位置引用行"** | 所以 `= 4` 不一定是 4 条 warning（`c25_exec_clears_handlers` 的 4 条就是空内容） | `c25_exec_clears_handlers` |

> ⚠️ 因此：所有环境相关的数字（`_SC_CHILD_MAX` / `execTime` / pid）都必须读作
> 「**这一次日志里的**」而不是「程序保证的」。本章笔记已在 §25.2 与 §25.5 显式声明。

---

## 七、文件校验（sha256）

```text
adc1199263dd7b9ea3b6baecdd255af596a222917fcddf9a02f15cd5bc39b640  exit_handlers.c
8e621758dd20d245c48096bb6dd32a35973b5e374a5b504f3cec8de0ad0d8b6c  fork_stdio_buf.c
714de46d171d5f22de9a42a228c3ede5999dd7ca48fab9d4f0f9acd66594f3da  probe25.c
0d69a00a601a81d3fddb60bf95049c8ec54330b5b06ac06c3f72754f44734eea  c25_exit_three_steps.c
0762dea98b164933795c243e08a6d4d56236435055d1799ff9cfdcdd85c200ef  c25_fall_off_main.c
cde8a424a078691a593402e788923206c11897b505aac144ed18a2fcd9c3c8b7  c25_exit_handler_no_return.c
1e13bf74c8da451aed8b945f2fcd504a45e3b575018971bc12cdcb3304cbaefb  c25_atexit_reentry.c
9e546f11b5cf8e4e667c58cf2c1f20033e3ecf20bbe825525960f37a7e16e889  c25_exec_clears_handlers.c
a122803ae79eb4c67e31429528136b780b0eb15f6925fbe0c9983d12cd554aa5  c25_fork_stdio_three_fixes.c
0942757dd3a307b9ebaf640bbee1e6700b3ef50c58caf7867b77eac2540822e3  ex25_1_exit_minus_one.c
1d1dedf473a0d91e60b03b7583e0fb52dbf82dcf6ed274bef354992e5a186327  tlpi_hdr.h
b6318f4d9d7d0416a574602d5fe86fa6c7af4871ec97a1900c24e5cdc4ca6724  get_num.c
e7b28952848dbd611edd8d7ab4440c18b49538ecb033704083905edfc69e8c08  get_num.h
583c28ebafd7bc9f10f26adb4f81a841a1d4a442df009ae1f1f938ac8f180edf  atexit_order.c
6bd589583107c1e096a73b5db96f05b5209f74d9e94a1a7180b4fda56cc929a0  exit_vs_exit.c
7f15a69adf3dd8dd6ca2a252f5ffd620b5fe99130d110c7800f4ade2cd1cfd64  fork_atexit.c
```

> `tlpi_hdr.h` 那份 `1d1dedf4…` 与 Ch23 / Ch24 的同名替身**sha256 相同**（同一份文件）。
> `verify_html_ch25.py` 会把这 16 个值逐一与实际文件比对（**核到「值」不只「名单」**）。

---

## 八、怎么用这套代码

### 1. 本地编译

```bash
cd code

# 原书镜像件（需要 -I. 找替身头）
gcc -O0 -Wall -Wextra -I. -o exit_handlers exit_handlers.c get_num.c
gcc -O0 -Wall -Wextra -I. -o fork_stdio_buf fork_stdio_buf.c get_num.c

# 自写件（不需要 -I.）
gcc -O0 -Wall -Wextra -o probe25 probe25.c
gcc -O0 -Wall -Wextra -o c25_exit_three_steps c25_exit_three_steps.c
gcc -O0 -Wall -Wextra -o c25_exit_handler_no_return c25_exit_handler_no_return.c
gcc -O0 -Wall -Wextra -o c25_atexit_reentry c25_atexit_reentry.c
gcc -O0 -Wall -Wextra -o c25_exec_clears_handlers c25_exec_clears_handlers.c
gcc -O0 -Wall -Wextra -o c25_fork_stdio_three_fixes c25_fork_stdio_three_fixes.c
gcc -O0 -Wall -Wextra -o ex25_1_exit_minus_one ex25_1_exit_minus_one.c
```

### 2. 按 25.1 → 25.6 的顺序跑

```bash
./probe25                                   # 先问清环境事实
./c25_exit_three_steps                      # ① exit() 三步的顺序
./c25_fall_off_c89; echo "c89 exit=$?"      # ② 掉出 main 末尾（C89）
./c25_fall_off_c99; echo "c99 exit=$?"      # ② 掉出 main 末尾（C99）
./exit_handlers; echo "exit=$?"             # ③ 官方 Listing 25-1（预期 exit=2）
./c25_atexit_reentry                        # ③ 插队首
./c25_exit_handler_no_return                # ③ 不返回的两种后果
./c25_exec_clears_handlers                  # ③ exec 清掉注册
./fork_stdio_buf | cat                      # ④ 官方 Listing 25-2（要重定向）
for m in 0 1 2 3; do ./c25_fork_stdio_three_fixes $m; done   # ④ 病 + 三种药
./ex25_1_exit_minus_one                     # ⑥ 习题 25-1
```

### 3. ⚠️ 在 CE 上跑时必须用「拼接」而不是多文件编译

CE 公开 API 一次只接受**一个源文件的拼接结果**。本仓库的 `run_ch25.py` 把
`get_num.h + tlpi_hdr.h + get_num.c + exit_handlers.c` 按这个顺序拼成一个翻译单元
（**头文件在前**，否则 `#include "tlpi_hdr.h"` 找不到）。
自写件是自包含的，直接单文件提交即可。

### 4. 四个「看着像 bug」其实是环境事实的现象

| 现象 | 真相 |
|------|------|
| `exit_handlers` 退出码是 **2** | 官方源码最后一行就是 `exit(2)`，**预期**（不是崩溃） |
| `exit_handlers` / `fork_stdio_buf` 有 warning | 官方源码自己的（`_BSD_SOURCE` 弃用 + `unused parameter`），本仓库**刻意不修** |
| `fork_stdio_buf` 在 CE 上没有"终端"那半 | CE 的 stdout 是 socket ⇒ 全缓冲。见第六节 |
| `c25_fall_off_c89` 退出码 **42** 而 c99 是 **0** | 这是 §25.1 要讲的**现象本身**，不是环境错误 |

---

## 九、与笔记的对应

| 笔记 | 用到的源文件 |
|------|-------------|
| [25.1 Terminating a Process: `_exit()` and `exit()`](../notes/25.1-terminating-a-process-exit-and-exit.md) | `c25_exit_three_steps.c` · `c25_fall_off_main.c`（+ `ex25_1` 的边界表） |
| [25.2 Details of Process Termination](../notes/25.2-details-of-process-termination.md) | `probe25.c` |
| [25.3 Exit Handlers](../notes/25.3-exit-handlers.md) | `exit_handlers.c` · `c25_atexit_reentry.c` · `c25_exit_handler_no_return.c` · `c25_exec_clears_handlers.c` |
| [25.4 Interactions Between fork(), stdio Buffers, and _exit()](../notes/25.4-interactions-between-fork-stdio-buffers-.md) | `fork_stdio_buf.c` · `c25_fork_stdio_three_fixes.c` |
| [25.5 Summary](../notes/25.5-summary.md) | （汇总全章 14 个作业） |
| [25.6 Exercise](../notes/25.6-exercise.md) | `ex25_1_exit_minus_one.c` |

> ⚠️ 笔记里的 ```c 块是从这些源文件**整文件内联**的（`make_ch25_notes.py` 的 `@@INLINE:@@` 宏展开），
> 与磁盘上这 16 个文件**逐行一致**（`cmp_note_code.py` 校验，本章 **10/10** 通过）。
> 笔记里的 ```text 块里的实测输出是从冻结日志 `tlpi-ch25-final.txt`
> 整段抄出来的（`@@OUT:@@` 宏展开），**115 行抽检 0 未命中**（`verify_note_outputs.py`）。
