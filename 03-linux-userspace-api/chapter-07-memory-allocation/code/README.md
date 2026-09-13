# Ch7 demos — 内存分配

本目录的 9 个 `.c` 与本章 4 篇笔记对应，**每个都在 Compiler Explorer（gcc 13.3.0，x86-64）上真实编译 + 运行过**，输出原样抄在对应笔记的「实测输出」块里。

## 8 个 demo

| 文件 | 对应节 | 演示什么 | 需要什么 |
|------|--------|----------|----------|
| `c7_1_program_break.c` | 7.1 | `sbrk(0)` / `sbrk(+n)` / `brk(addr)` 的正常语义；子进程演示「break 降过头 → SIGSEGV」 | `fork` + `wait` |
| `c7_2_free_and_sbrk.c` | 7.1 | 1000×1KB 与 256KB 两条路；`free` 后 break 只降一部分 | 无 |
| `c7_3_two_paths.c` | 7.1 | 16KB×10 → `[heap]` 台阶式扩张；256KB×4 → 匿名段每次 +260KB | `/proc/self/maps` |
| `c7_4_malloc_family.c` | 7.1 | `malloc(0)` / 不清零（tcache 覆写头部 16 字节）/ `calloc` 溢出保护 / `realloc` 四种情形 | 无 |
| `c7_5_memalign.c` | 7.1 | 16 字节默认对齐、`posix_memalign` 页对齐、非法对齐 → `EINVAL` | 无 |
| `c7_6_heap_errors.c` | 7.1 | 五类堆错误，ASan 逐个抓（用 `-DCASE=n` 单 case 编译） | ASan |
| `c7_7_alloca.c` | 7.2 | `alloca` 落在栈上 / 循环里不回收 / 与 VLA 对比 / 16MB → SIGSEGV | `fork` + `wait` |
| `ex7_1_simple_malloc.c` | 7.4 练习 1 | 用 `sbrk` + 空闲链表实现的 `my_malloc` / `my_free` | 无 |

## 1 个原书 Listing 的最小复刻

保留下来是为了能直接和书里的代码对照；深度实验用上面的 `c7_*`。

| 文件 | 原书 | 说明 |
|------|------|------|
| `free_and_sbrk.c` | Listing 7-1 | 支持命令行参数（分配个数 / 块大小 / free 范围）；默认 100×1KB 后全部 free |

## 编译

一次编完 8 个（ASan 那个单独编）：

```bash
for f in c7_*.c ex7_1_*.c; do gcc -O2 -Wall -Wextra -o "${f%.c}" "$f" || echo "FAIL $f"; done
```

单个（以 7.1 的两条路径为例）：

```bash
gcc -O2 -Wall -Wextra -o c7_3_two_paths c7_3_two_paths.c && ./c7_3_two_paths
```

Listing 7-1 复刻（和书一致，默认优化级别）：

```bash
cc -Wall -Wextra -o free_and_sbrk free_and_sbrk.c && ./free_and_sbrk
```

## 运行

| 命令 | 说明 |
|------|------|
| `./c7_1_program_break` | 安全归位 vs 降过头崩溃（后者在子进程） |
| `./c7_2_free_and_sbrk` | free 不影响 break 的量化 |
| `./c7_3_two_paths` | 两条路径的实时观测（需要 `/proc`） |
| `./c7_4_malloc_family` | 8 个语义实验（含 `realloc` 前后地址） |
| `./c7_5_memalign` | 三种对齐接口 |
| `./c7_7_alloca` | alloca 四实验 |
| `./ex7_1_simple_malloc` | 自制分配器 |
| `./free_and_sbrk 100 1024 1 1 100` | Listing 7-1（分配 100 个 1KB，全部 free） |

## 5 个必须知道的坑

1. **`brk` 降过头会拆掉 glibc 的堆**：`c7_1` 的 B 段故意演示 —— stdio 的缓冲区就住在堆上，降回去之后子进程 `SIGSEGV`，一行输出都写不出来。所以 `c7_1` 里 `sbrk(0)` 的取值被刻意安排在**首次 `printf` 之后**。
2. **输出会在崩溃路径上消失，但有两种机制**：① **数据没被 flush** —— ASan 报错后直接 `_exit`，stdio 里排队的 `printf` 内容全丢（`c7_6` CASE 4 因此在 `__lsan_do_leak_check()` 之前加了 `fflush(stdout)`）；② **缓冲区内存本身被 unmap** —— `c7_1` B 段把 break 降回去之后，glibc 的 stdio 缓冲区连同里面没来得及写出的数据一起没了（那里加 `fflush` 是为了让之前的输出先落地）。
3. **读 `/proc/self/maps` 别用 `fopen`**：`fopen` 自己会 `malloc` 一块缓冲，反而污染要观测的堆。`c7_3` 用的是 `open` + `read` + 静态缓冲区。
4. **`malloc` 会复用空闲块，掩盖真相**：`c7_3` 的逐点验证故意**不 free**，否则下一个请求会从空闲链表直接满足，两条路径都观测不到。
5. **CE 上 ASan 的 `detect_leaks` 默认关闭**：`c7_6` 用 `#ifdef __SANITIZE_ADDRESS__` + `__lsan_do_leak_check()` 显式触发，才能拿到泄漏报告。

## c7_6 的特别编译方式

每个 case 单独编成独立程序 —— 全放一个进程里会因 tcache 状态互相干扰：

```bash
for n in 1 2 3 4 5; do
    gcc -O1 -g -fsanitize=address -DCASE=$n -Wall -Wextra -o c7_6_case$n c7_6_heap_errors.c
    ./c7_6_case$n
done
```

## c7_4 的两条编译告警是故意的

`c7_4_malloc_family.c` 在 `-O2 -Wall -Wextra` 下会报 2 条 `-Wuse-after-free`（"pointer 'p' may be used after 'realloc'"）。

不是代码写错：我们**同时打印 `realloc` 前后的地址**，而 GCC 的指针分析无法区分「打印地址值」和「解引用」。**但告警指向的规则是真的** —— `realloc` 之后旧指针一律失效，生产代码里报这个要当真 bug 查。
