# Ch3 demos — 内存类

本目录是 3.1 / 3.2 / 3.3 三篇笔记共用的「病人」。三份程序都在 Compiler Explorer
（**gcc 13.3.0 / clang 18.1.0**，x86-64，Ubuntu 24.04 容器）上真编译、真跑过，
输出原样抄在对应笔记里。

> ⚠️ **先说清楚本目录「实测」与「格式示意」的边界**：
> - `c3_1_*` / `c3_2_*` / `c3_3_*` 的 **ASan / LSan / UBSan 输出**是**实测**（CE 能跑）。
> - **valgrind 输出是格式示意** —— 本仓库的验证环境（Windows 本地 + CE 容器）**装不了 valgrind**（本地无 Linux 环境；CE 不提供 valgrind 这类重型动态插桩工具）。3.1 笔记里的 valgrind 报告字段／措辞依官方手册给出，行号按本目录源码的真实行号对齐，方便你上 Linux 机器时逐字段对上。
> - **MSan 也拿不到可信输出** —— clang `-fsanitize=memory` 在 CE 容器里跑出一份 `SIGSEGV` 而不是 MSan 报告，判断是容器 libc 未做 MSan 插桩导致的假失败，因此 3.1 笔记里不写 MSan 的输出。

## 三份 demo

| 文件 | 对应节 | 演示什么 | 怎么跑 |
|------|--------|----------|--------|
| `c3_1_mem_bugs.c` | 3.1 / 3.2 | **五个内存错误，一个程序，用 argv 选**：`1`=堆越界写、`2`=栈越界写（★ valgrind 盲区）、`3`=use-after-free、`4`=double free、`5`=泄漏（100×400 字节） | `./c3_1_mem_bugs <1..5>` |
| `c3_2_uninit_read.c` | 3.1 / 3.2 | **未初始化读**：ASan 看不见（实测 exit 0、零报告），valgrind / MSan 才看得见。用栈上未初始化 `int` + 堆上未赋值的 `struct order`，并打印出残留垃圾值 | 无参数 |
| `c3_3_ubsan_ops.c` | 3.3 | **UBSan 能抓的四种「看不见的错」**：①有符号溢出 ②移位越界（实测 `1<<40` 得 **256**，不是 0）③对 `LONG_MIN` 取负 ④除零 | 无参数 |

## 编译

```bash
# ---- 3.1 / 3.2：同一份程序，两套构建 ----
gcc -g -O0 -Wall -Wextra -o c3_1_plain c3_1_mem_bugs.c          # 裸编译（对照：什么都不报）
gcc -g -O0 -fsanitize=address -o c3_1_asan c3_1_mem_bugs.c      # ASan + LSan

gcc -g -O0 -fsanitize=address -o c3_2_asan c3_2_uninit_read.c   # 期望：exit 0、零报告
clang -g -O1 -fsanitize=memory -fno-omit-frame-pointer \
      -o c3_2_msan c3_2_uninit_read.c                            # 期望：MSan 报告（本环境跑不出来）

# ---- 3.3 ----
gcc -g -O1 -fsanitize=undefined -o c3_3_ubsan c3_3_ubsan_ops.c
gcc -g -O1 -fsanitize=undefined -fno-sanitize-recover=all \
      -o c3_3_ubsan_stop c3_3_ubsan_ops.c
gcc -g -O0 -Wall -Wextra -o c3_3_plain c3_3_ubsan_ops.c
```

## 运行与期望结果（均为实测）

| 命令 | 期望结果 | 退出码 |
|------|----------|--------|
| `./c3_1_plain 1` | `case 1: 写了 buf[8]...` + `case 1 跑完了 main` ← **什么都没报** | **0** |
| `./c3_1_asan 1` | `heap-buffer-overflow`，栈指向 `:38`，分配点 `:34`，含 `fa fa 00[fa]fa` 影子图 | **1** |
| `./c3_1_asan 2` | `stack-buffer-overflow`，`WRITE of size 32`，列出栈帧内 `dst [32,40)` / `src [64,96)` | **1** |
| `./c3_1_asan 3` | `heap-use-after-free`，`freed by :63` / `previously allocated by :59` / 报错点 `:64` | **1** |
| `./c3_1_asan 4` | `attempting double-free`，三段栈 `:76` / `:75` / `:72`，无影子图，末尾 `ABORTING` | **1** |
| `./c3_1_asan 5` | `LeakSanitizer: detected memory leaks`，`Direct leak of 40000 byte(s) in 100 object(s)` at `:84` | **1** |
| `./c3_2_asan` | **零报告** —— ASan 不管「值是否初始化」 | **0** |
| `./c3_3_ubsan` | 4 条 `runtime error:`（`:31` 溢出 / `:35` 移位 / `:40` 取负 / `:54` 除零），stdout 打出 4 行错值，最后 SIGFPE | **136** |
| `./c3_3_ubsan_stop` | 只报第 1 条（`:31` 溢出），**stdout 为空** | **1** |
| `./c3_3_plain` | **零诊断**，stdout 打出 4 行错值，最后 SIGFPE | **136** |

## 五个必须知道的坑

1. **为什么全套用 `-O0` 而不是 ASan 官方推荐的 `-O1`** —— 实测 `-O1 -fsanitize=address` 下五个 case 的检出一览：

   | case | `-O0` | `-O1` |
   |------|-------|-------|
   | 1 堆越界 | ✅ | ✅ |
   | 2 栈越界 | ✅ | ✅（但栈帧归到 `main`、写方变成 `memset`） |
   | 3 UAF | ✅ | ❌ **检不出（exit 0、零报告）** |
   | 4 double free | ✅ | ✅ |
   | 5 泄漏 | ✅ | ✅ |

   case 3 在 `-O1` 下 `p[0] = 7` 与紧随的 `g_sink = p[0]` 一起成了死代码被删掉——**UAF 在 `-O1` 下「不存在」了**。这条是本章最该记住的反直觉结论：**优化会改变 bug 的存在性，不只是改变报告措辞**。
2. **`volatile int g_sink` 不是装饰** —— 每个 case 末尾都从越界/UAF 的地址**真读一次**再写进 `g_sink`，否则 `-O1/-O2` 会把整片代码当死码删掉（`-O0` 其实也会删一部分，靠 volatile 保住）。
3. **case 1 在裸编译下「成功」跑完不是巧合** —— 它只越界 1 字节（`buf[8]` 踩进红区第一字节），物理上完全可写，所以不崩。这也是内存 bug 的教学要点：**「不崩」和「没错」是两件事**。
4. **`c3_3` 每行 `printf` 后都有 `fflush(stdout)`** —— ④ 除零会 SIGFPE 直接杀进程，而 stdout 接管道时是**全缓冲**的：不刷的话前面四条「看似合理的错值」全留在缓冲区里随进程消失，你只能看到 UBSan 报告、看不到任何程序输出。`c3_3_ubsan_stop` 的 stdout 为空就是同一个坑的另一个面（第一个 UB 在第一次 `printf` 之前就 abort 了）。
5. **`c3_3` 的除零必须写成两个 `volatile`** —— 写成 `1 / v_zero`（常量分子）时 gcc 13.3 `-O0` 会把它折成一段**不含 `idiv`** 的无分支序列，结果是「不崩、直接打出 0」。演示脚本在 `chapter-01-methodology/code/c1_4_hypothesis_div.c`，完整推演见 1.4 笔记。
