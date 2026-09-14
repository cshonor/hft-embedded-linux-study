# TLPI 第 03 章 — System Programming Concepts

**优先级**：🔴 必读
**前置**：[Ch2 Fundamental Concepts](../chapter-02-basic-concepts/README.md)
**后置**：[Ch4 Universal I/O](../chapter-04-file-io-universal/README.md)（第一个实战 syscall 集）

---

## 小节目录

- [3.1 System Calls 系统调用](notes/3.1-system-calls.md)
- [3.2 Library Functions 库函数](notes/3.2-library-functions.md)
- [3.3 The Standard C Library; The GNU C Library (glibc)](notes/3.3-glibc.md)
- [3.4 Handling Errors from System Calls and Library Functions 错误处理](notes/3.4-error-handling.md)
- [3.5 Notes on the Example Programs in This Book 本书示例程序说明](notes/3.5-example-programs.md)
- [3.6 Portability Issues 可移植性](notes/3.6-portability.md)
- [3.7 Summary 本章小结](notes/3.7-summary.md)
- [3.8 Exercises 练习](notes/3.8-exercise.md)

> 八节的划分与 TLPI 原书一致（3.1–3.8）。原书 3.5 内部的 **3.5.1 Command-Line Options and Arguments** / **3.5.2 Common Functions and Header Files**，以及 3.6 内部的 **3.6.1 Feature Test Macros** / **3.6.2 System Data Types** / **3.6.3 Miscellaneous Portability Issues**，都收在对应那一篇里用 `###` 分节，**不拆文件**。

---

## 章节目标

- **模型**：用户态 ↔ 内核交互只有一条正规入口（系统调用），glibc 在其上做封装
- **错误**：`errno` 的六条硬规则 + 三种「可恢复 vs 真失败」错误码辨析
- **工程**：功能测试宏（API 可见性）+ 系统数据类型（类型宽度）+ 杂项可移植性（语义细节）

### 与 Ch4 边界（防混淆）

| 章 | 主题 | 内容 |
|----|------|------|
| **Ch3** | System Programming Concepts | 理论：syscall 模型、`errno`、glibc 身份、可移植性 |
| **Ch4** | Universal I/O Model | 实战：`open` / `read` / `write` / `lseek`、fd |

→ 对照：[LKD §5.1 libc≠syscall](../../05-linux-kernel/chapter-05-system-calls/notes/section-5.1-与内核通信.md)

---

## 原书示例清单（man7 官方按章文件列表）

| Listing | 文件 | 作用 |
|---------|------|------|
| 3-1 | `lib/tlpi_hdr.h` | 公共头文件（常用系统头 + `Boolean` + `min`/`max`） |
| 3-2 | `lib/error_functions.h` | 错误处理函数声明 |
| 3-3 | `lib/error_functions.c` | 错误处理函数实现 |
| 3-4 | `lib/ename.c.inc` | `errno` → 名字 的静态表 |
| 3-5 | `lib/get_num.h` | 命令行数值解析函数声明 |
| 3-6 | `lib/get_num.c` | 命令行数值解析函数实现 |
| — | `lib/alt_functions.[hc]` | libc 函数的替代版（**无 Listing 号**） |
| — | `progconc/syscall_speed.c` | 测系统调用开销（**无 Listing 号**） |

> 出处：[man7 — List of source code files, by chapter](https://man7.org/tlpi/code/online/all_files_by_chapter.html)。三个最常记错的点：**3-1 是 `tlpi_hdr.h`**（不是 `error_functions.h`）；**3-5 是 `get_num.h`**（`.c` 才是 3-6）；`ename.c.inc` 是 Listing 3-4。详见 [3.5](notes/3.5-example-programs.md)。

---

## 易错清单

1. **`errno` 成功时不清零** —— 判成败只能看返回值，不能 `if (errno != 0)`。
2. **`errno` 是 TLS** —— 每线程一份，不能拿它做线程间通信。
3. **库函数失败不一定设 `errno`** —— `getenv` 找不到返回 `NULL` 且 `errno = 0`（先读 man NOTES）。
4. **`strerror` 返回全局静态缓冲** —— 存指针等于存「最后一次」的结果；多线程下是数据竞争。
5. **`EINTR` 不是错误** —— 被信号打断要重试，不是报错退出。
6. **`EFAULT` 是内存 bug** —— 传了非法指针给内核，查野指针/越界，别去查磁盘或网络。
7. **直接 `syscall()` 拿的是 `-errno`** —— `syscall(2)` 库函数自带翻译层；只有手写 `syscall` 指令才是裸负值。
8. **`fopen` 与 `open` 不是一一对应** —— `fopen("w")` 隐含 `O_CREAT`，`open(O_WRONLY)` 没有。
9. **「用 `syscall()` 更快」是错的直觉** —— 它比 glibc 包装还慢（多一层壳），只在 glibc 没包装时才用。
10. **加编译旗标不一定只增不减** —— `-std=c99` → `__STRICT_ANSI__` → glibc 不再自动补 `_DEFAULT_SOURCE`/`_POSIX_C_SOURCE` → `strdup`/`clock_gettime`/`strsignal` 全部消失。
11. **`memmem` 守的是 `__USE_MISC`，不是 `__USE_GNU`** —— 所以严格 POSIX 反而锁掉它。
12. **`__USE_POSIX2008` 这个宏不存在** —— POSIX.1-2008 在 glibc 里的真名是 `__USE_XOPEN2K8`。
13. **`-std=c99` 想拿回 POSIX 声明必须显式加 FTM** —— `-D_POSIX_C_SOURCE=200809L` 或 `-D_GNU_SOURCE`。
14. **`strerror_r` 有两套签名** —— glibc 默认 POSIX 版（返回 `int`），只有 `_GNU_SOURCE` 才切 GNU 版（返回 `char *`）。
15. **`char` 的符号是实现定义** —— x86/aarch64 Linux 上 `(char)0xFF` 是 `-1`；`getchar()` 的返回值必须装进 `int`。
16. **`struct{char;int;char}` 是 12 字节** —— 不是 6 字节；跨平台传结构体必须显式定序 + 对齐。
17. **`reboot()` 没有 glibc 包装** —— 必须 `syscall(SYS_reboot, ...)`，且需要 `CAP_SYS_BOOT`。

---

## 章节链路

```text
Ch2  基本概念（用户态 / 内核态、地址空间、/proc）
  → Ch3  syscall 模型 + glibc 三种身份 + errno 范式 + 可移植性三件套
  → Ch4  open/read/write/lseek 实战（第一个真实 syscall 集）
  → Ch5  文件 I/O 细节（O_APPEND 原子性、fd 与 FILE*、大文件）
  → Ch49 mmap / Ch50 mprotect 等
```

---

## 双线提示

| 路线 | |
|------|--|
| 嵌入式 | 严格返回值 + `errno` 检查；musl 静态编译摆脱 glibc 方言；交叉编译前先确认 `__aarch64__` 分支与 `char` 符号性；`_FILE_OFFSET_BITS=64` 处理大日志 |
| HFT | 少 syscall（逐字节 `read` = N 次陷入）；区分 glibc 包装成本与真陷入；能用 vDSO 的调用不陷入内核；错误日志用 `strerror_r` + 完整上下文 |

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | syscall = 进内核的唯一正规入口；编号 + 寄存器 + 陷阱指令 |
| 2 | 库函数 ≠ syscall；有的纯用户态（`strcpy` 十万倍仍是 0 次 I/O） |
| 3 | 先判返回值，再读 `errno`；成功不清零 |
| 4 | 内核用 `[-4095, -1]` 表示错误（`MAX_ERRNO=4095`）；glibc 翻译成 `-1` + `errno` |
| 5 | 功能测试宏放在所有 `#include` 之前；`features.h` 只读一次 |
| 6 | `_GNU_SOURCE` 会改写而非追加：`_POSIX_C_SOURCE=200809L` + `_XOPEN_SOURCE=700` + `_DEFAULT_SOURCE=1` |
| 7 | 标准 typedef 管宽度，`PRI*` 宏管打印；`off_t` 别写死 `long` |
| 8 | Ch3 理论 · Ch4 才是 `open`/`read`/`write` 实战 |

---

## 参考

- Kerrisk, *The Linux Programming Interface*, **Chapter 3 — System Programming Concepts**
- [man7 官方源码清单（按章）](https://man7.org/tlpi/code/online/all_files_by_chapter.html) · [OUTLINE](../OUTLINE.md) · [Ch4](../chapter-04-file-io-universal/README.md)

---

## 代码示例

本章 `code/` 下有 **13 个** 可直接编译的程序，全部在 Compiler Explorer（gcc 13.3.0 / x86-64 / Ubuntu 24.04）上真实编译 + 运行过（其中 `c3_9` 跑 5 组旗标、`c3_10` 跑 2 组，共 **18 个作业**，`build code = 0`、`diagnostics = 0`），输出原样抄在对应笔记的实测块里。完整索引见 [`code/README.md`](code/README.md)。

| 文件 | 对应节 | 演示什么 |
|------|--------|----------|
| `c3_1_syscall_path.c` | 3.1 | 三条调用路径同写一份数据；失败分支的 `-1`+`errno` vs 裸 `-9`；`SIGPIPE` 的两种结局 |
| `c3_2_lib_vs_syscall.c` | 3.2 | 用 `/proc/self/io` 量「库函数不进内核」；`fopen` vs `open` 的 flag 差异 |
| `c3_3_glibc.c` | 3.3 | glibc 的三种身份与版本宏 / 运行期自报 |
| `c3_4_errno.c` | 3.4 | `errno` 六条规则逐条实测（含线程私有地址对比） |
| `c3_5_errno_traps.c` | 3.4 | 六种 `errno` 误用逐个打脸 |
| `c3_13_efault.c` | 3.4 | 六种坏指针喂给 `write(2)`，看内核怎么拦成 `EFAULT` |
| `c3_6_cli_args.c` | 3.5.1 | `argc`/`argv` 的真实形状 + `getopt(3)` 的三种结局 |
| `c3_7_get_num.c` | 3.5.2 | `atoi` 为什么不够用 + 复刻原书 `get_num.c` 的三态判定 |
| `c3_8_error_functions.c` | 3.5.2 | 六个「会自杀的」错误处理函数各关进子进程，抓输出与退出码 |
| `c3_9_ftm.c` | 3.6.1 | 同一个源文件 5 组旗标 → `__USE_*` 矩阵与声明可见性全表 |
| `c3_10_types.c` | 3.6.2 | LP64 数据模型 + 系统类型宽度 + `_FILE_OFFSET_BITS=64` 的作用 |
| `c3_11_portability.c` | 3.6.3 | 平台宏、字节序、`char` 符号性、负数右移、结构体对齐 |
| `c3_12_syscall_speed.c` | 3.7 | 系统调用 / glibc 包装 / 普通函数 / 纯用户态库函数的耗时对照 |

一次编完全部（在 `code/` 目录下）：

```bash
for f in c3_*.c; do gcc -O0 -Wall -Wextra -o "${f%.c}" "$f" || echo "FAIL $f"; done
```

需要额外旗标的（共 8 个作业）：

```bash
# c3_4 / c3_5 用线程，需要 -pthread
gcc -O0 -Wall -Wextra -pthread -o c3_4 c3_4_errno.c
gcc -O0 -Wall -Wextra -pthread -o c3_5 c3_5_errno_traps.c

# c3_9 五组旗标（笔记里给了完整对照表）
for f in "" "-D_POSIX_C_SOURCE=200809L" "-D_XOPEN_SOURCE=700" "-D_GNU_SOURCE" "-std=c99"; do
    gcc -O0 -Wall -Wextra $f -o c3_9 c3_9_ftm.c && ./c3_9
done

# c3_10 两组设定
gcc -O0 -Wall -Wextra                      -o c3_10 c3_10_types.c && ./c3_10
gcc -O0 -Wall -Wextra -D_FILE_OFFSET_BITS=64 -o c3_10 c3_10_types.c && ./c3_10
```

**几条实测结论**（都是本仓库跑出来的，不是书上抄的）：

- **库函数不进内核**：`strcpy`/`strlen`/`atoi` 放大到十万轮，`/proc/self/io` 的 `Δsyscr` 与空负载底噪完全相同；而 `fprintf ×100` 未 `fflush` 时 `Δsyscw = 0`，一次 `fflush` 才变成 1 次 `write`
- **同一个 `/dev/null`**：`open(O_WRONLY)` 成功、`fopen("w")` 失败（`errno=13`）—— 因为 `fopen` 隐含 `O_CREAT`
- **`errno` 不清零**：失败的 `open` 留下 `errno=2`，紧接着成功的 `close` 之后 `errno` 仍是 `9`
- **`strerror` 同一块缓冲**：`strerror(ENOENT)` 三次调用返回**同一个地址** `0x7ac7f39cb77a`
- **坏指针是 `EFAULT` 不是 `SIGSEGV`**：`write(1, NULL, 10)` 得 `errno=14`；`n=0` 时甚至直接成功（`write(fd,NULL,0)=0`）
- **`-std=c99` 会减掉声明**：五组旗标里只有这一组让 `strdup`/`clock_gettime`/`strsignal` 全部不可见
- **`memmem` 在严格 POSIX 下反而不可见**（它守 `__USE_MISC`）
- **系统调用 ≈ 116 倍普通函数调用**：`getppid()` 812.9 ns vs `nop_call()` 7.0 ns（CE 容器绝对值，不可迁移；可迁移的是比值）
