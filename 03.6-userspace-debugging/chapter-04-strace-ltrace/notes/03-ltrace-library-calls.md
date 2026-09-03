# 4.3 ltrace 库调用追踪（与 strace 对比 / malloc-free 追踪）

> 🔴 精读 · 从「系统调用」往上一层，看「库函数」

## 本节要点

strace 看的是**系统调用**（进程让内核做什么），ltrace 看的是**库函数调用**（进程调了动态库里的哪个函数、传了什么参数）。两者是同一件事的两个层次：`fopen` 是库函数，strace 里看不到，ltrace 里能看到；`fopen` 底层落成的 `openat` syscall，ltrace 里看不到，strace 里能看到。本节讲清 ltrace 的定位、与 strace 的对比，以及最实用的场景——追踪 `malloc`/`free` 定位内存泄漏。

## ltrace 是什么

ltrace 用 `ptrace` 接管进程，但拦截的是**动态链接函数的调用**（通过 PLT 跳转表），把「函数名 + 参数 + 返回值」打印出来。它能看到：

| 能追到的 | 例子 |
|----------|------|
| glibc 标准库函数 | `malloc`、`free`、`strcpy`、`memcpy`、`printf` |
| 其他动态库函数 | `pthread_create`（libpthread）、`dlopen`（libdl） |
| 库函数返回值 | `malloc(1024) = 0x555...` |

## ltrace vs strace：一张表说清

| 维度 | strace | ltrace |
|------|--------|--------|
| 追踪对象 | **系统调用**（syscall） | **库函数调用**（library call） |
| `fopen` 能追到吗 | ❌（只看到 `openat`） | ✅（看到 `fopen`） |
| `malloc` 能追到吗 | ❌（只看到 `brk`/`mmap`） | ✅（看到 `malloc`/`free`） |
| 依赖 | 内核 syscall 表 | 动态链接 PLT/GOT |
| 静态链接程序 | ✅ 照常工作 | ❌ 追不到（无动态链接） |
| 适用 | 看程序与内核的交互 | 看程序与库的交互、内存分配 |

> **一句话**：`fopen` 是库函数，`openat` 是 syscall；库函数是 syscall 的「封装层」。strace 看底层内核交互，ltrace 看上层库调用。两个工具**互补**，常常配合使用。

```bash
# 同一个 readfile.c（见 4.1），ltrace 看到的是库函数
ltrace ./readfile /tmp/a.txt
fopen("/tmp/a.txt", "r")            = 0x5555555592a0
fgets(0x7fff..., 128, 0x5555555592a0) = 0x7fff...
fputs("hello\n", 0x7f...)           = 1
fclose(0x5555555592a0)              = 0
# ↑ 全是库函数；对比 4.1 里 strace 看到的是 openat/read/write
```

## 基本用法

```bash
ltrace ./prog              # 从头追踪库函数
ltrace -c ./prog           # 只输出汇总统计
ltrace -p 12345            # attach 到运行中进程
ltrace -f ./prog           # 也追踪线程
ltrace -e malloc+free ./prog   # 只看 malloc 和 free
ltrace -o trace.log ./prog     # 输出到文件
```

参数与 strace 高度一致（`-c` 统计、`-e` 过滤、`-f`、`-p`、`-o`），因为两者都是 ptrace 家族、共用同一套设计思路。

## 输出格式解读

```
函数名(参数...) = 返回值
```

| 部分 | 例子 | 含义 |
|------|------|------|
| 函数名 | `malloc` | 库函数名 |
| 参数 | `(1024)` | 参数（字符串加引号、结构体尽量解码） |
| 返回值 | `= 0x5555555592a0` | 返回指针/整数 |

```bash
ltrace ./orderbook 2>&1 | head
malloc(1024)                      = 0x5555555592a0
memset(0x5555555592a0, 0, 1024)   = 0x5555555592a0
strcpy(0x5555555592a0, "order")   = 0x5555555592a0
free(0x5555555592a0)              = <void>
```

指针返回值的意义：`malloc` 返回的 `0x555...` 是堆上分配的地址，配合 `free` 的参数能**配平**——同一个指针先 `malloc` 后 `free`，没配平就是泄漏。

## 实战：用 ltrace 定位内存泄漏

```c
// leak.c —— 每次循环 malloc 但不 free（模拟泄漏）
#include <stdlib.h>
#include <string.h>
void handle_order(void) {
    char *buf = malloc(1024);        // 分配
    strcpy(buf, "buy");
    // 忘了 free(buf)  ← 泄漏点
}
int main(void) {
    for (int i = 0; i < 100; i++)
        handle_order();
    return 0;
}
```

```bash
gcc -g -O0 -o leak leak.c
ltrace -c -e malloc+free ./leak
% time     seconds  usecs/call     calls      function
------ ----------- ----------- --------- ---------------
 100.00    0.000123         123       100      malloc
   0.00    0.000000           0         0      free
------ ----------- ----------- --------- ---------------
```

**`malloc` 100 次、`free` 0 次** —— 泄漏实锤。这正是 ltrace 相对 strace 的杀手锏：strace 里 100 次 `malloc` 会变成 `brk`/`mmap` 若干次（glibc 有自己的内存池，不是每次 malloc 都真去问内核要），**层不对，看不出 100 vs 0 的配平关系**。ltrace 在「库调用」这一层，正好看清 `malloc`/`free` 的配对。

> ⚠️ 更强大的内存泄漏工具是 Ch5 的 **valgrind**（`memcheck`），它不仅能定位「漏了」，还能定位「哪一行代码漏的」。ltrace 适合**快速定性**（有没有泄漏、量级多大），valgrind 适合**精确归因**（泄漏源在哪）。两者是「定性 → 定位」的接力。

## ltrace 的局限与陷阱

| 局限 | 说明 | 应对 |
|------|------|------|
| 追不到静态链接程序 | 无 PLT/GOT，无法拦截 | 换 strace，或重编为动态链接 |
| 追不到内联函数 | 编译器把函数体内联了，没有调用 | `-O0 -fno-inline` 重编 |
| 追不到自己写的静态函数 | 编译期绑定，不走 PLT | 无解（用 gdb 断点） |
| 参数解码不如 strace 全 | 结构体/指针解码有限 | 配合 gdb 看现场 |
| 性能开销大 | 每个库调用都要拦截 | 不能生产热路径使用（同 strace） |
| 高版本 glibc 变体函数 | `malloc` 可能被优化成 `__libc_malloc` | 用 `-e malloc` 前缀匹配 |

> 核心机制回顾：动态链接下，程序调用库函数是**间接跳转**——先跳到 PLT（Procedure Linkage Table）存根，存根再查 GOT（Global Offset Table）拿到真实地址跳过去。ltrace 在 PLT 这层设陷阱，所以能拦到「调用了哪个库函数」。这也解释了为什么静态链接（无 PLT）和编译期内联（无跳转）都追不到。

## HFT 关联

1. **内存泄漏快速定性**：订单/行情进程长时间运行，`ltrace -c -e malloc+free` 看 `malloc` 与 `free` 次数是否配平——不配平就是泄漏，比肉眼读代码快一个量级。
2. **库调用链还原**：追踪 `fopen`/`strcpy`/`memcpy` 等库函数，定位「配置文件读错」「字符串越界拷贝」「缓冲区操作」在**哪一层**出问题——这些 strace 看不见（它只看 syscall）。
3. **配合 strace 分层定位**：先 ltrace 看「哪个库函数行为不对」，再 strace 看「它到底让内核做了什么」，一层层下钻，把「表象（库函数）→ 机制（syscall）」串起来。
4. **⚠️ 别在热路径跑 ltrace**：它比 strace 开销还大（每个库调用都拦），只能开发期定性，生产观测交给 valgrind（离线）或 eBPF（06.7，在线零开销）。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 同样跑 `readfile.c`，strace 和 ltrace 分别看到什么？为什么？

> strace 看到 **syscall**：`openat`/`read`/`write`/`close`；ltrace 看到**库函数**：`fopen`/`fgets`/`fputs`/`fclose`。因为 `fopen` 是 glibc 库函数，它内部会去调用 `openat` 等 syscall——ltrace 拦在库调用层，strace 拦在 syscall 层，所以看到的是同一件事的两个层次。

**Q2:** 定位内存泄漏，为什么 ltrace 比 strace 更直接？

> 因为 `malloc` 是**库函数**，glibc 有自己的内存池：程序 100 次 `malloc`，glibc 可能只向内核要一两次 `brk`/`mmap`。strace 看到的 `brk`/`mmap` 次数和 `malloc` 次数**对不上**，无法判断泄漏。ltrace 直接拦 `malloc`/`free` 库调用，能精确看到「100 次 malloc、0 次 free」的配平关系，泄漏一目了然。

**Q3:** 为什么 ltrace 追不到静态链接的程序，也追不到 `-O2` 下内联的小函数？

> ltrace 依赖**动态链接的 PLT/GOT 间接跳转**来拦截。静态链接把库代码直接编进可执行文件，没有 PLT 存根，无处下陷阱；`-O2` 内联把函数体直接展开到调用点，根本没有「函数调用」这个动作，自然追不到。前者换 strace 或重编动态链接，后者用 `-O0 -fno-inline` 重编。

**Q4:** `malloc(1024) = 0x5555555592a0` 里，返回值 `0x555...` 是什么？为什么能用来配平泄漏？

> 是 `malloc` 在**堆上分配的地址**（`0x55...` 是典型堆区段）。`free` 的参数就是这个指针。追踪时把每个 `malloc` 返回的指针和后续 `free` 的参数配平——只 `malloc` 没 `free` 的指针就是泄漏。ltrace 的 `-c` 统计直接给出 `malloc` vs `free` 的次数差，快速定性。

**Q5:** ltrace 能替代 valgrind 吗？两者在内存问题上怎么分工？

> 不能。ltrace 只能**快速定性**（有没有泄漏、大概多少次），它只看「调用了没配对」，看不到「哪一行代码漏的」；valgrind（memcheck）能**精确归因**——定位泄漏的具体分配点、未初始化读、越界写等，还能给出调用栈。分工：ltrace 定性（快、粗），valgrind 定位（慢、准），是「定性 → 定位」的接力。

</details>

## 交叉引用

- [4.1 strace 入门](01-strace-basics.md)
- [4.2 strace 实战分析](02-strace-practical-analysis.md)
- [03.6 模块导读](../../README.md)
