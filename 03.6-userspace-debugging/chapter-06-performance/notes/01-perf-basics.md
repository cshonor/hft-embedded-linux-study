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
