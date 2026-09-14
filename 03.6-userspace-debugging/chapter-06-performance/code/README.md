# Ch6 性能类 · 可跑的 demo

| 文件 | 演示什么 | 一句话看点 |
|------|----------|------------|
| `c6_1_cache_miss.c` | 同一份计算，只改**访存顺序** | 加法次数一模一样，按列遍历慢 **11.2 倍** |
| `c6_2_self_sampler.c` | 不装 perf，自己在进程内实现一个采样 profiler | 采出与 `perf report` 同构的热点表，`slow_sqrt` 占 **87.8%** |

> **关于「实测」**：下面数字来自 Compiler Explorer（godbolt.org）的 **gcc 13.3.0**。
> 耗时随机器变化 —— **别看绝对值，看同一台机器上的倍数关系**。
>
> **本章的诚实边界**：perf 本体测不了。CE 容器里没有 PMU 权限、也没装 perf，
> 所以 `perf stat`（IPC、cache-misses）和 `perf report` 的**输出形状是格式示意**。
> 但「采样 → 聚合 → 热点表」这条原理链是**完整实测**的：`c6_2` 用
> `setitimer(ITIMER_PROF)` + `SIGPROF` + `backtrace()` 把 perf 的三步自己走了一遍，
> 得到的表与 `perf report` 同构。

---

## c6_1_cache_miss.c —— 「IPC 远小于 1」的另一面

6.1 说 `perf stat` 的 `instructions/cycles ≪ 1` 意味着程序访存密集——CPU 大半时间
在等内存。CE 上测不了 IPC，但能测**同一现象的另一面**：加法次数完全一样，
只改访问顺序，耗时差十倍以上。差掉的那部分就是 CPU 在等 cache line。

```bash
cc -g -O2 -Wall -Wextra -o c6_1_cache_miss c6_1_cache_miss.c
./c6_1_cache_miss
```

### 实测输出

```text
=== 同一份计算，只改访存顺序 ===

实验 A：4096×4096 的 int 矩阵求和，矩阵 64 MiB（两边都是 16777216 次加法）
  按行遍历（地址连续，1 条 cache line 装 16 个 int）:    11.88 ms
  按列遍历（stride 16384 B，每步换一条 cache line）  :   132.54 ms
  → 慢 11.2 倍。加法次数一模一样，多出来的时间全花在等 cache line。

实验 B：5000000 次「读一个 int 决定下一步」（next[] 有 1048576 个节点，4096 KiB）
  顺序下标（预取器有效）:          3.63 ms
  指针追逐（地址依赖数据）:       92.86 ms
  → 慢 25.6 倍。这就是「追 cache」的真实价格，也是 HFT 里
     为什么宁可用数组也不用链表的硬理由。
```

### 尺寸就是结论：16 MiB 时只慢 1.4 倍

同一台机器上，把实验 A 的 N 从 2048 提到 4096（矩阵 16 MiB → 64 MiB）：

| N | 矩阵 | 按行 | 按列 | 倍数 |
|---|------|------|------|------|
| 2048 | 16 MiB | 5.77 ms | （程序算出的倍数） | **1.4×** |
| 4096 | 64 MiB | 11.88 ms | 132.54 ms | **11.2×** |

16 MiB 的矩阵**整个装得进容器的 L3**，所以按列遍历只是多花点 L3 延迟，
差别不明显；64 MiB 超出 L3 之后，按列变成几乎每次访问都要去主存。
**慢了不是「算法差」，是工作集装不装得下。** 这个拐点本身就说明：
调 cache 问题的第一步永远是问「数据集多大、cache 多大」。

---

## c6_2_self_sampler.c —— 自制一个 profiler

`perf record` 的原理就三句话（见 6.1）：① 定期中断；② 中断里记录 PC + 栈；
③ 事后按函数聚合。这个程序把三步在进程内自己实现一遍：

| 步骤 | perf 用的 | 本程序用的 |
|------|-----------|-----------|
| ① 触发采样 | PMU 硬件计数器 + 内核 NMI | `setitimer(ITIMER_PROF)` → `SIGPROF` |
| ② 记录现场 | 内核读 PMU + 栈回溯 | 信号处理函数里 `backtrace()`，只存裸 PC |
| ③ 聚合 | `perf report` / `perf script` | 跑完在普通上下文里 `backtrace_symbols()` 解析并排序 |

**为什么用 `ITIMER_PROF` 而不是墙钟定时器**：它按**进程 CPU 时间**计时，
进程被调度走时不计时——这正是 profiler 想要的（否则 IO 等待会污染比例）。

```bash
cc -g -O2 -fno-inline -rdynamic -Wall -Wextra -o c6_2_self_sampler c6_2_self_sampler.c
./c6_2_self_sampler
```

三个编译参数缺一不可：

- `-g`：符号表带行信息；
- `-fno-inline`：热点函数被内联进调用者后就采不到独立符号，表里只会剩一个大坨的
  `do_work`（就是 6.1 说的「fno-inline 的取舍」）；
- `-rdynamic`：把符号导出到 `.dynsym`，`backtrace_symbols` 才给得出函数名
  —— 不加就只有裸偏移（见 2.3）。

### 实测输出

```text
workload 跑完：sum=13429258.94 buf=8888888888

共采到 172 个样本（≈ 172 ms 进程 CPU 时间）
（已跳过每个栈开头的 2 帧：frame 0 = backtrace() 内部帧，frame 1 = 采样函数 on_prof 自己）

【自用 self】≡ perf report 的 Overhead 列 —— CPU 时间花在谁身上
  占比      样本  函数
    87.8%     151  slow_sqrt
     6.4%      11  fmt
     3.5%       6  do_work
     1.2%       2  +0x1995a6
     1.2%       2  ./output.s() [0x4010f0]

【含子调用 inclusive】≡ 火焰图里该帧的宽度 —— 调用链经过谁
  占比      样本  函数
   100.0%     172  do_work
   100.0%     172  main
   100.0%     172  +0x2a1ca
   100.0%     172  __libc_start_main
   100.0%     172  _start
    87.8%     151  slow_sqrt
     8.7%      15  fmt

【折叠栈 collapsed】≡ stackcollapse-perf.pl 的输出 —— 可直接喂给 flamegraph.pl
  slow_sqrt;do_work;main;+0x2a1ca;__libc_start_main;_start                 151
  fmt;do_work;main;+0x2a1ca;__libc_start_main;_start                       11
  do_work;main;+0x2a1ca;__libc_start_main;_start                           6
  +0x1995a6;fmt;do_work;main;+0x2a1ca;__libc_start_main;_start             2
  ./output.s() [0x4010f0];fmt;do_work;main;+0x2a1ca;__libc_start_main;_start 2
  （共 5 种不同的调用栈）
```

对着 6.1 那张 `perf report` 示意表看：`Overhead / Symbol` 两列一模一样。
唯一区别是 perf 还能顺带给出 `cycles / instructions / cache-misses`
（因为它的采样源是 PMU），而我们的采样源只是个定时器。

关于 folded 格式：**把它贴到自己的 Linux 上跑 `flamegraph.pl` 就是一张真火焰图**
（本环境没有 perl / FlameGraph 脚本，所以到 folded 为止）。火焰图只是把这张表
画成了宽度——这正好接上 [6.2](../notes/02-flamegraph.md)。

---

## 踩坑记录（都是实跑出来的）

### 坑 1：矩阵 16 MiB 时「cache miss 实验」看不出效果

第一版用 N=2048（16 MiB），按列只比按行慢 1.4 倍——因为矩阵整个装进了容器的 L3。
不是程序错了，是**规模没到拐点**。改到 N=4096（64 MiB）才有 11.2 倍。

**教训**：做 cache 实验必须先确认工作集**真的超过**了被测层级的容量，
否则测的是「L3 延迟」而不是「主存延迟」。

### 坑 2：`printf` 少传一个参数，gcc 只说 warning 不说 error

```c
printf("  按列遍历（stride %d B）: %8.2f ms\n", N * 4);   /* ← 忘了 t_col */
```

`%8.2f` 读了一个不存在的参数，打出的数字看着像模像样（5.77 ms），
和按行一模一样——差点被当成「两者一样快」。`-Wall` 的
`warning: format '%f' expects a matching 'double' argument` 是唯一线索。

**教训**：`-Wall` 不是可选项；**打印出来的数字也要交叉验证**
（那次「两行一样但倍数写着 1.4×」的自相矛盾就是破绽）。

### 坑 3：`backtrace()` 在信号处理函数里会拿到两层自己的帧

第一次跑出来 self 表只有一行 `+0x45330`，占 100% —— 那是**采样函数自己**
（`on_prof` 是 `static`，不进 `.dynsym`，所以只显示裸偏移）。glibc/x86-64 上的
真实栈布局是：

```text
frame 0 = backtrace() 内部帧
frame 1 = on_prof（我们的采样函数，static → 显示成 +0x45330）
frame 2 = 真正被打断的那个函数   ← self 应该记这一帧
frame 3+ = 各级调用者
```

所以源码里加了 `SKIP_FRAMES 2`。换 libc / 架构后如果 inclusive 表第一行又出现
一个 100% 的怪名字，把 `SKIP_FRAMES` 加 1 即可。

### 坑 4：`-rdynamic` 只救「非 static」的函数

源码里 `slow_sqrt` / `fmt` / `do_work` 刻意**不加 `static`**。如果加了，
`.dynsym` 里就没有它们，表里会变成 `output.s(+0x401914)` 这样的裸偏移
—— 和 2.3 里 `c2_2_backtrace.c` 踩的是同一个坑。这是「符号表决定你能看见什么」
的第二次实证。
