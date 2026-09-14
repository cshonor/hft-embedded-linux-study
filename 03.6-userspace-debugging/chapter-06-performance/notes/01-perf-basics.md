# 6.1 perf 基础采样（record / report 定位热点函数）

> 选读 · 用 perf 快速定位「CPU 花在哪个函数上」

## 本节要点

perf 是 Linux 内核自带的性能分析工具，基于 **PMU（Performance Monitoring Unit）** 硬件计数器。性能排查的第一步永远是「**采样定位热点**」——先搞清楚 CPU 时间花在哪几个函数上，再谈怎么优化。本节讲 `perf stat`（统计）与 `perf record/report`（采样）两个核心用法，以及如何读热点报告。深入的系统级性能分析（CPU/内存/IO/网络）交给 06.6。

## 先看一个有热点的程序

```c
// hot.c —— 有明显热点：slow_sqrt 被反复调用
#include <stdio.h>

// 热点 1：故意慢的牛顿迭代（100 次），模拟「计算密集」函数
double slow_sqrt(double x) {
    double r = x;
    for (int i = 0; i < 100; i++)
        r = (r + x / r) / 2.0;
    return r;
}

// 热点 2：字符串拼接（模拟日志/序列化开销）
void fmt(double v, char *buf) {
    for (int i = 0; i < 10; i++)
        buf[i] = "0123456789"[(int)v % 10];   // 故意低效
    buf[10] = 0;
}

void do_work(void) {
    double sum = 0;
    char buf[11];
    for (int i = 0; i < 10000000; i++) {
        sum += slow_sqrt((i % 100) + 1);
        fmt(sum, buf);          // 每轮都调字符串，也是热点
    }
    printf("sum=%.2f buf=%s\n", sum, buf);
}

int main(void) {
    do_work();
    return 0;
}
```

编译（**务必带调试信息 + 关掉部分优化保证函数不被内联**）：

```bash
gcc -g -O2 -fno-inline -o hot hot.c
# -g        → perf report 才能显示函数名和源码行
# -fno-inline → 防止 slow_sqrt 被内联进 do_work，保证能看到独立热点
```

## perf stat：先看宏观「这程序贵在哪」

`perf stat` 不采样，而是用硬件计数器统计**整个运行期间的事件总数**：

```bash
perf stat ./hot
# Performance counter stats for './hot':
#
#        1,234.56 msec task-clock          #    1.000 CPUs utilized
#          12,345      context-switches    #   10.0 K/sec
#           1,234      cpu-migrations      #    1.0 K/sec
#           5,678      page-faults         #    4.6 K/sec
#  4,123,456,789      cycles              #    3.34 GHz
#  3,987,654,321      instructions        #    0.97 insn per cycle
#     45,678,901      branch-misses       #    1.11% of all branches
#
#       1.234567890 seconds time elapsed
```

> ⚠️ **上面这些数字是「列的形状示意」，不是实测**——`1,234.56` / `4,123,456,789`
> 一眼就是编的。这里本环境**跑不了 perf**（容器里没装、也没有 PMU 权限），
> 所以本节所有 `perf stat` / `perf report` 的输出都只保证**列名和量级形状**是对的。
> 真正可实测的是**同一现象的替代实验**：见本节末尾「动手」——
> 用 `clock_gettime` 测「同一份计算、只改访存顺序」的倍数差，
> 再把 perf 的三步原理自己在进程内实现一遍。

关键看几个数：

| 指标 | 含义 | 异常信号 |
|------|------|----------|
| `task-clock` | CPU 实际执行时间 | 远小于 wall time → 程序在等 IO/锁，不是计算密集 |
| `instructions / cycles`（IPC） | 每周期执行指令数 | 远小于 1 → 大量 stall（cache miss/分支预测失败） |
| `context-switches` | 上下文切换 | 极高 → 线程太多争抢 CPU |
| `branch-misses` | 分支预测失败率 | 高 → 数据相关分支太多，考虑分支消除 |
| `page-faults` | 缺页 | 极高 → 内存访问模式差或内存不足 |

> `perf stat` 是**第一眼体检**：先判断「是计算密集（IPC 高）还是访存密集（IPC 低）」，决定下一步查 CPU 热点还是查 cache miss。

## perf record + report：采样定位热点函数

`perf stat` 告诉你「贵」，但没告诉你「贵在哪个函数」。要定位热点，用 `perf record` 采样：

```bash
perf record ./hot        # 周期采样，记录每次中断时的 PC + 调用栈
# [ perf record: Woken up 1 times to write data ]
# [ perf record: Captured and wrote 0.123 MB perf.data ]

perf report              # 交互式浏览热点排行
```

`perf report` 输出（默认按采样占比降序）：

```text
Overhead  Command  Shared Object     Symbol
  68.21%  hot      hot               [.] slow_sqrt
  21.45%  hot      hot               [.] fmt
   8.11%  hot      hot               [.] do_work
   1.02%  hot      libc.so.6         [.] printf
   0.90%  hot      hot               [.] main
```

（同样只是**形状示意**。真实可跑的版本见末尾「动手」——自制采样器实测得到的是
`slow_sqrt 87.8% / fmt 6.4% / do_work 3.5%`，**同样的三列，真实的数字**。）

**读法**：`Overhead` 是「该函数被采样命中的占比」，约等于「CPU 时间花在这个函数上的比例」。这里一眼看出 `slow_sqrt` 占 68%——**热点实锤**，优化它收益最大。

常用参数：

```bash
perf record -g ./hot       # -g 记录调用栈（callgraph），火焰图必需
perf record -F 99 ./hot    # 采样频率 99 Hz（默认约 4000 Hz，太高开销大）
perf report --stdio        # 非交互、纯文本输出（脚本友好）
perf report -g graph       # 按调用图展示（看调用关系）
perf report -n             # 显示采样次数（不只是百分比）
```

### perf annotate：热点函数内部逐行看

定位到 `slow_sqrt` 后，想知道「这个函数里哪一行最贵」，用 annotate：

```bash
perf annotate slow_sqrt
```

```text
       │    double slow_sqrt(double x) {
       │        double r = x;
       │        for (int i = 0; i < 100; i++)
 87.32 │  a0:  divsd  %xmm1,%xmm0       ← 除法指令占 87% 时间
       │            r = (r + x / r) / 2.0;
 12.68 │  b0:  addsd  %xmm2,%xmm0       ← 加法/乘法只占零头
```

一眼看到 `divsd`（浮点除法）占了 87%——**除法是瓶颈**。优化方向立刻清晰：牛顿迭代收敛快，100 次太多，砍到 10 次或换查表法。

## 采样原理（为什么这么准）

perf 的 `record` 是**基于事件的周期采样**（不是每句指令都记录）：

```
PMU 硬件计数器在 CPU 内不断累加「事件」（默认是 cycles）
  ↓ 每累计 N 个事件（如每 100000 个周期）
触发一次 NMI 中断
  ↓ 中断处理程序记录当前 PC（程序计数器）+ 调用栈
写入 perf.data 缓冲区
```

- **采样频率**默认约 4000 Hz，意味着「CPU 在哪个函数上花的时间多，就被采样到的次数多」——这是统计规律，不是精确计数。
- 开销低（通常 <5%），可以用于接近生产的场景。
- 局限：采样是**统计近似**，会有误差；超短运行的程序样本不足；某些情况下会被「优化掉」的函数看不到。

## 动手：没有 perf，怎么把「采样定位热点」亲手做一遍（实测）

本环境没有 perf（容器里没装，也没有 PMU 权限）。但 perf 的**原理**只有三步，
完全可以自己实现——而且自己做一遍，比看十遍 `perf report` 的输出更懂它。

### 实验一：`code/c6_1_cache_miss.c` —— 低 IPC 到底是什么体验

6.1 说 IPC 远小于 1 意味着「访存密集」——CPU 在等内存。这个程序测的是同一现象的
**另一面**：加法次数完全一样，只改**访问顺序**，看耗时差多少。

```bash
cc -g -O2 -Wall -Wextra -o c6_1_cache_miss code/c6_1_cache_miss.c
./c6_1_cache_miss
```

实测输出（gcc 13.3.0）：

```text
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

**实验 A 有个更值得记住的细节：尺寸本身就是结论。**

| N | 矩阵 | 按行 | 按列 | 倍数 |
|---|------|------|------|------|
| 2048 | 16 MiB | 5.77 ms | — | **1.4×** |
| 4096 | 64 MiB | 11.88 ms | 132.54 ms | **11.2×** |

同一个程序、同样的代码，N=2048 时按列只慢 1.4 倍——因为 16 MiB **整个装得进
容器的 L3**，按列遍历只是多花点 L3 延迟。到 64 MiB 超出 L3 之后才变成「每次访问
都要去主存」。

**所以调 cache 问题的第一步永远是问两个数：数据集多大、cache 多大。**
「算法一样慢十倍」这种描述是没用的 —— 慢的是「装不下」。

**实验 B 更狠**：数组顺序走 3.63 ms，指针追逐 92.86 ms，差 **25.6 倍**。
两者都是「读一个 int 决定下一步跳哪」，唯一区别是指针追逐的地址**由数据本身决定**，
硬件预取器完全无法提前搬数据。这就是 6.1 HFT 关联第 2 条「HFT 程序常是访存密集」
的字面价格。

### 实验二：`code/c6_2_self_sampler.c` —— 自己实现一个 profiler

这个程序把 `perf record` 的三步在**进程内**做一遍，只用标准库：

| 步骤 | perf 用的 | 本程序用的 |
|------|-----------|-----------|
| ① 触发采样 | PMU 硬件计数器 + 内核 NMI | `setitimer(ITIMER_PROF)` → `SIGPROF` |
| ② 记录现场 | 内核读 PMU + 栈回溯 | 信号处理函数里 `backtrace()`，只存裸 PC |
| ③ 聚合 | `perf report` / `perf script` | 跑完在普通上下文里 `backtrace_symbols()` 解析排序 |

```bash
cc -g -O2 -fno-inline -rdynamic -Wall -Wextra -o c6_2_self_sampler code/c6_2_self_sampler.c
./c6_2_self_sampler
```

实测输出（gcc 13.3.0）：

```text
共采到 172 个样本（≈ 172 ms 进程 CPU 时间）
（已跳过每个栈开头的 2 帧：frame 0 = backtrace() 内部帧，frame 1 = 采样函数 on_prof 自己）

【自用 self】≡ perf report 的 Overhead 列 —— CPU 时间花在谁身上
  占比      样本  函数
    87.8%     151  slow_sqrt
     6.4%      11  fmt
     3.5%       6  do_work

【含子调用 inclusive】≡ 火焰图里该帧的宽度 —— 调用链经过谁
   100.0%     172  do_work
   100.0%     172  main
   100.0%     172  __libc_start_main
   100.0%     172  _start
    87.8%     151  slow_sqrt
     8.7%      15  fmt
```

对着本节上面那张 `perf report` 示意表看：**`Overhead / Symbol` 两列一模一样**。
唯一区别是 perf 还能顺带给出 `cycles / instructions / cache-misses`
（因为它的采样源是 PMU，我们的采样源只是个定时器）。

三个实现细节值得记：

1. **为什么用 `ITIMER_PROF` 而不是墙钟定时器**：它按**进程 CPU 时间**计时，
   进程被调度走时不计时。这正是 profiler 要的——否则 IO 等待会污染比例。
   （`perf stat` 里 `task-clock` 远小于 wall time 就是同一件事的宏观版。）
2. **`-fno-inline` 必须加**：热点函数被内联进调用者后就采不到独立符号，
   表里只会剩一个大坨的 `do_work`。这就是 HFT 关联第 3 条「fno-inline 的取舍」
   的**实测形态**——你可以把 `-fno-inline` 去掉再跑一次，亲眼看它退化。
3. **采样函数自己的帧要跳掉**：从信号处理函数里调 `backtrace()`，
   frame 0 是 `backtrace()` 内部帧、frame 1 是**采样函数自己**（它是 `static`，
   不进 `.dynsym`，所以显示成 `+0x45330` 这种裸偏移，永远占 100%）。
   跳过前 2 帧后 `self` 才是真正被打断的函数。

### 这个自制采样器还顺手给了 6.2 需要的 folded 格式

它最后会打出一段**折叠栈**：

```text
slow_sqrt;do_work;main;+0x2a1ca;__libc_start_main;_start                 151
fmt;do_work;main;+0x2a1ca;__libc_start_main;_start                       11
do_work;main;+0x2a1ca;__libc_start_main;_start                           6
（共 5 种不同的调用栈）
```

这正是 `stackcollapse-perf.pl` 的输出格式。**把它贴到自己的 Linux 上跑
`flamegraph.pl`，就得到一张真火焰图** —— 这正好接上 6.2。

## HFT 关联

1. **延迟毛刺先采样别猜**：某笔单延迟突然 250ms，先 `perf record` + `perf report` 看这段时间 CPU 花在哪，比读代码猜「可能是这里慢」高效得多——用数据说话。
2. **`instructions/cycles` 判访存密集**：HFT 程序常是访存密集（追 cache、追内存），IPC 远小于 1。看到低 IPC 就该往 cache miss 方向查（6.2 火焰图 + cache 事件），而不是盲目优化计算。
3. **`fno-inline` 的取舍**：采样要看得见独立函数才需要关内联；但生产构建内联能减调用开销。所以是「调试构建关内联采样，生产构建保持内联」两套目标。
4. **annotate 定位到指令**：HFT 里热点往往收敛到某几个关键函数（订单匹配、价格计算），annotate 能精确到「哪条指令（除法？跳转？）最贵」，直接指导重构。

```bash
# HFT 场景：对撮合引擎做 10 秒采样定位热点
perf record -F 99 -g ./matching_engine --sim data.csv &
PID=$!
sleep 10 && kill -INT $PID       # 跑 10 秒后发 SIGINT 结束采样
perf report --stdio | head -20    # 看 Top 热点函数
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** `perf stat` 和 `perf record/report` 的区别？各自的用途？

> `perf stat` 用硬件计数器**统计**整个运行期间的事件总数（cycles、instructions、context-switches、branch-misses 等），告诉你「程序整体贵在哪」——是计算密集还是访存密集，是 CPU 忙还是等 IO。`perf record/report` 是**采样**：周期中断记录 CPU 当前在哪个函数，统计出「各函数占用的 CPU 时间比例」，告诉你「贵在哪个函数」。一个是宏观体检，一个是热点定位。

**Q2:** 为什么 `perf report` 的 `Overhead` 能代表「CPU 时间占比」？

> 因为采样是统计规律：周期中断「随机」地打断程序，记录当时的 PC。一个函数占用 CPU 时间越多，中断落在它里面的概率就越大，被采样到的次数就越多。所以「采样命中占比 ≈ CPU 时间占比」。前提是采样频率够高、运行时间够长（样本足够多），否则统计误差大。

**Q3:** IPC（instructions/cycles）远小于 1 说明什么？对优化方向有什么指导？

> 说明 CPU 大量时间在「空转等待」——等内存（cache miss）、等分支预测失败回滚、等数据依赖。这是**访存密集**程序的特征（对比计算密集程序 IPC 接近 1 甚至 >1 超标量）。指导：此时不该优化算术计算，而该去查 cache miss（6.2）、数据布局、内存访问模式。HFT 程序常是这种，所以低延迟优化重点在内存子系统而非 CPU 计算。

**Q4:** 采样为什么会有误差？哪些场景不适用？

> 采样是统计近似而非精确计数：①中断是周期的，可能恰好漏掉某次短函数调用；②超短运行的程序样本不足，统计不可靠；③编译器优化（内联）会把函数「藏」进别的函数，看不到独立热点；④采样本身有开销，极端高频采样会扰动被测程序。所以采样定位「主要热点」很可靠，定位「微小开销」不可靠。

**Q5:** `perf record -g` 里的 `-g` 是干嘛的？什么时候必须加？

> `-g` 记录**调用栈**（call graph），让 perf 不只记录「当前在哪个函数」，还记录「是从哪个调用链上来的」。做**火焰图**（6.2）时**必须加** `-g`，因为火焰图要完整的调用栈来画出纵轴深度。只做扁平热点排行（`perf report` 默认）可以不加，加了也无妨。

</details>

## 交叉引用

- [6.2 火焰图](02-flamegraph.md)
- [1.2 症状 → 工具决策树](../../chapter-01-methodology/notes/02-symptom-to-tool.md)
- [Ch6 性能类](../README.md)
