# Ch3 demos — 系统编程概念

本目录的 13 个 `.c` 与本章 8 篇笔记对应，**每个都在 Compiler Explorer（gcc 13.3.0 / x86-64 / Ubuntu 24.04）上真实编译 + 运行过**（`build code = 0`、`diagnostics = 0`，共 18 个作业），输出原样抄在对应笔记的实测块里。

## 13 个 demo

| 文件 | 对应节 | 演示什么 | 需要什么 |
|------|--------|----------|----------|
| `c3_1_syscall_path.c` | 3.1 | glibc 包装 / `syscall(2)` / 手写 `syscall` 指令三条路径写同一份数据；失败分支的 `-1`+`errno` vs 裸 `-9`；`SIGPIPE` 的 `SIG_DFL` / `SIG_IGN` 两种结局 | `fork` + `wait` + `pipe` |
| `c3_2_lib_vs_syscall.c` | 3.2 | 用 `/proc/self/io` 的增量证明「纯用户态库函数不进内核」；`fopen("w")` vs `open(O_WRONLY)` 的 flag 差异；stdio 缓冲的 `Δsyscw` | `/proc/self/io`、`/etc/passwd` |
| `c3_3_glibc.c` | 3.3 | `__GLIBC__` / `gnu_get_libc_version()` / `confstr` / `_POSIX_VERSION` / `__GNUC__` 各自身份 | 无 |
| `c3_4_errno.c` | 3.4 | `errno` 六条规则逐条实测：不清零、可赋值左值宏、TLS 地址对比、中间调用冲掉、库函数不一定设 | `-pthread` |
| `c3_5_errno_traps.c` | 3.4 | 六种 `errno` 误用逐个打脸，含 `strerror` 三次返回同一地址、`%m` 扩展 | `-pthread` |
| `c3_13_efault.c` | 3.4 | 六种坏指针喂给 `write(2)`：`NULL` / `0x1` / 越界值 / 只读映射页；`EFAULT` vs `SIGSEGV` | `mmap` + `mprotect` |
| `c3_14_raw_asm.S` | 3.1 | 无 libc 纯汇编：`_start` 直接 `mov x8,#64; svc #0`（aarch64）/ `mov $1,%eax; syscall`（x86_64）发 `write`+`exit`，证明「glibc 只是填寄存器的便利，不是内核的准入证」 | `-nostdlib -static`；aarch64 在 Pi 5 实测，x86_64 分支未实测 |
| `c3_6_cli_args.c` | 3.5.1 | `argc`/`argv` 的真实形状（`argv[argc] == NULL`）+ 人工构造 `argv` 演示 `getopt(3)` 三种结局 | 无 |
| `c3_7_get_num.c` | 3.5.2 | `atoi` 为什么分不清「0」与「出错」+ 复刻原书 `get_num.c` 的 `strtol`+`errno`+`endptr` 三态判定 | 无 |
| `c3_8_error_functions.c` | 3.5.2 | 六个「会自杀的」错误处理函数各关进子进程，抓回真实输出与退出码；`_exit()` 丢 stdio 缓冲的 MARKER 实验 | `fork` + `wait` + `pipe` |
| `c3_9_ftm.c` | 3.6.1 | 同一个源文件 5 组旗标 → `__USE_*` 开关矩阵与声明可见性全表（守护条件照抄 glibc 2.39） | 需 5 次编译 |
| `c3_10_types.c` | 3.6.2 | LP64 数据模型、系统类型宽度、`PRI*` 宏、`_FILE_OFFSET_BITS=64` 的实际作用 | 需 2 次编译 |
| `c3_11_portability.c` | 3.6.3 | 平台/编译器宏、字节序（宏 + 运行期双验证）、`char` 符号性、负数右移、结构体对齐填充 | 无 |
| `c3_12_syscall_speed.c` | 3.7 | 系统调用 / glibc 包装 / 普通函数 / 纯用户态库函数的单次耗时对照 | `CLOCK_MONOTONIC`，跑得较久 |

## 编译

一次编完（在 `code/` 目录下；`c3_4`/`c3_5` 需要 `-pthread`，所以单独编）：

```bash
for f in c3_*.c; do gcc -O0 -Wall -Wextra -o "${f%.c}" "$f" || echo "FAIL $f"; done
```

需要额外旗标的 8 个作业：

```bash
# 线程
gcc -O0 -Wall -Wextra -pthread -o c3_4 c3_4_errno.c
gcc -O0 -Wall -Wextra -pthread -o c3_5 c3_5_errno_traps.c

# c3_9：五组旗标
for f in "" "-D_POSIX_C_SOURCE=200809L" "-D_XOPEN_SOURCE=700" "-D_GNU_SOURCE" "-std=c99"; do
    gcc -O0 -Wall -Wextra $f -o c3_9 c3_9_ftm.c && ./c3_9
done

# c3_10：两组设定
gcc -O0 -Wall -Wextra                        -o c3_10 c3_10_types.c && ./c3_10
gcc -O0 -Wall -Wextra -D_FILE_OFFSET_BITS=64 -o c3_10 c3_10_types.c && ./c3_10
```

> 全部用 `-O0`：本批 demo 里有好几处依赖「函数调用真的发生」（如 `c3_12` 测 `nop_call()` 的开销），`-O2` 会把它们内联掉，测出来是 0。

## 运行

| 命令 | 说明 |
|------|------|
| `./c3_1_syscall_path` | 三条路径 + 两组失败分支（`SIGPIPE` 在子进程里演示，不会杀掉自己） |
| `./c3_2_lib_vs_syscall` | 底噪 → 库函数 → NSS → stdio 缓冲 → `malloc`，一节一节量 |
| `./c3_3_glibc` | 打印编译期/运行期的库与标准版本 |
| `./c3_4_errno` / `./c3_5_errno_traps` | 六条规则 / 六种误用 |
| `./c3_13_efault` | 六种指针的 `write` 结果对照 |
| `./c3_14_raw_asm` | 输出 `raw syscall: no libc`，`echo $?` 得 42（aarch64 上：`gcc -nostdlib -static -o c3_14_raw_asm c3_14_raw_asm.S`） |
| `./c3_6_cli_args` | `argv` 形状 + `getopt` 三种结局 |
| `./c3_7_get_num` | `atoi` 对照 + 11 组三态判定 |
| `./c3_8_error_functions` | 六个函数输出/退出码表 + MARKER 实验 + 管道关闭时机 |
| `./c3_9` | 需换旗标重编才看得到差异（见上） |
| `./c3_10` | 需换 `_FILE_OFFSET_BITS` 重编才看得到差异 |
| `./c3_11_portability` | 平台宏 / 字节序 / 符号性 / 右移 / 对齐 |
| `./c3_12_syscall_speed` | 四种调用的耗时对照（跑 2000000 次/组，稍慢） |

## 6 个必须知道的坑

1. **`SIGPIPE` 会把进程直接杀掉，不是返回错误码**：`c3_1` 第一版在主进程里写了一个只读 fd，结果进程被信号 13 带走、`exit code = 141`，后面几段一行都没跑到。修法是把这段关进 `fork` 出来的子进程，并在子进程里**显式** `signal(SIGPIPE, how)`——因为 `SIG_IGN` 会被 `fork` 继承。
2. **抓子进程输出必须 `read` 到 EOF 再 `close`**：`c3_8` 第一版「只 `read` 一次就 `close`」，七个函数里六个变成「被信号 13 终止」，整张表失去意义。正确姿势是 `read()` 返回 `0` 才说明子进程那一端关干净了。
3. **`fopen("w")` 会失败而 `open(O_WRONLY)` 会成功**：`fopen` 隐含 `O_CREAT`，而某些环境（如本批 CE 容器）拒绝对 `/dev/null` 带 `O_CREAT` 打开。`c3_2` 因此改用 `open` + `fdopen`，并把这个差异做成了活教材。
4. **`getpwuid(getuid())` 可能返回 `NULL` 且 `errno = 0`**：容器里 `uid 0` 根本不在 `/etc/passwd` 里。`c3_2` 改成先从 `/etc/passwd` 读出一个**真实存在**的用户名再 `getpwnam()`。注意「查无此人 ≠ 出错」。
5. **`setvbuf` 改已经用过的流是 UB**：`c3_8` 的 MARKER 实验第一版想用 `setvbuf` 切换 stdout 缓冲模式，glibc 直接忽略，两边都打印 MARKER、证明不了任何事。改成**全局不设 `_IONBF`**，让「非 tty → 全缓冲」自然成立。
6. **`__USE_POSIX2008` 这个宏不存在**：`c3_9` 第一版打印了它（凭印象写的）。核对 glibc 2.39 `features.h` 后确认真实只有 25 个 `__USE_*`，POSIX.1-2008 对应的是 `__USE_XOPEN2K8`。**写宏名/行号之前必须核源码。**

## 关于日志里的 NUL 字节

`c3_13` 第一版把一段**以 `0x00` 开头**的内存当 `write` 的源，日志里混进 16 个 NUL，整个 `.txt` 变成「二进制文件」。修法是把只读映射页改成 `mmap(PROT_READ|PROT_WRITE)` → `memcpy("RO-PAGE\n")` → `mprotect(PROT_READ)`：**先写内容再收权限**。以后写 demo 时，凡是往 stdout 写实验数据，都要确认里面没有 `0x00`。
