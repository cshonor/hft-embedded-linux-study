# 7.3 竞态 → TSan / gdb 多线程定位

> 🔴 精读 · 结果「时对时错」，TSan 稳定揪出数据竞争

## 本节要点

用 7.1 的 `trader.c` 开 `BUG_RACE` 雷：feed 和 match 两个线程**无锁累加**共享变量 `g_total`，造成数据竞争——结果时对时错、不崩不卡、极难靠肉眼发现。本节走一遍「观察症状 → TSan 定位竞争双方 → 修复」的流程，核心工具是 Ch4 的 ThreadSanitizer。重点体会：**为什么普通跑很难发现，TSan 却能稳定复现**。

## 第一步：观察「时对时错」

```bash
gcc -g -O0 -pthread -o trader_race -DBUG_RACE trader.c
./trader_race
# total matched qty = 38766    ← 第一次
./trader_race
# total matched qty = 40100    ← 第二次（碰巧对了）
./trader_race
# total matched qty = 39214    ← 第三次，又错了
```

正确值应是 **40100**（7.1 算过），但三次跑出三个不同结果——**偶发、结果漂移、不崩不卡**。这就是数据竞争的典型签名。

> 为什么错：feed 和 match 都在 `g_total += o->qty`，这是「读-改-写」三步。两线程交错时互相覆盖：A 读到 1000，B 也读到 1000，各自加 qty 写回，A 的更新被 B 覆盖丢失。丢多少取决于调度，所以每次结果不同。

## 第二步：TSan 定位竞争双方

```bash
gcc -g -O1 -pthread -fsanitize=thread -o trader_race_tsan -DBUG_RACE trader.c
./trader_race_tsan
```

```text
==================
WARNING: ThreadSanitizer: data race (pid=12345)
  Write of size 8 at 0x7b0400000080 by thread T2 (match):
    #0 match_thread trader.c:56           ← match 在 56 行写 g_total
    #1 <null> <null>

  Previous write of size 8 at 0x7b0400000080 by thread T1 (feed):
    #0 feed_thread trader.c:41            ← feed 在 41 行写 g_total
    #1 <null> <null>

  Location is global 'g_total' of size 8 at 0x7b0400000080 (trader_race_tsan+0x...)

  Thread T2 'match' (running) created by main thread at:
    #0 pthread_create trader.c:76
  Thread T1 'feed' (running) created by main thread at:
    #0 pthread_create trader.c:75

SUMMARY: ThreadSanitizer: data race trader.c:56 in match_thread
==================
```

解读（4.3 讲过）：

1. **竞争双方**：`match_thread`（56 行）和 `feed_thread`（41 行）都在写 `g_total`，一个 `Write` 一个 `Previous write`——**两个线程无锁写同一变量，实锤**。
2. **位置**：`Location is global 'g_total'` —— 竞争的是全局统计量。
3. **线程来源**：两个线程都在 `main` 里 `pthread_create`（75/76 行），确认是 feed 和 match 并发。

## 第三步：修复（加锁或原子）

```c
// 修法 1：用已有的 g_stat_lock 保护（与正确版本一致）
pthread_mutex_lock(&g_stat_lock);
g_total += o->qty;
pthread_mutex_unlock(&g_stat_lock);

// 修法 2：原子操作（无锁，HFT 低延迟首选）
#include <stdatomic.h>
// 把 volatile long g_total 改成 atomic_long g_total
atomic_fetch_add(&g_total, o->qty);
```

修复后重新 TSan 编译跑，**无 data race 报告**，结果稳定 `40100`。

## 为什么「普通跑难发现，TSan 稳定复现」

这是本节的灵魂，回顾 4.3：

```
普通运行：只有「竞争造成错误结果」时你才知道出事了
          → 但大多数时候结果碰巧对（丢的更新少，不明显）
          → 偶发、难复现、看不出规律

TSan 运行：在【每次】内存访问时检查 happens-before
          → 只要两个无同步线程访问同一位置且有写，【当场】报
          → 不依赖「结果错没错」，稳定报出竞争关系本身
```

所以 TSan 抓的是「**竞争关系**」，不是「**竞争后果**」——即使某次结果碰巧对了，TSan 照样报。这就是「偶发 bug 能被稳定复现」的原因。

## gdb 多线程兜底（TSan 之外的备选）

如果环境装不了 TSan（如某些嵌入式交叉编译链），可以用 Ch4 的 gdb 多线程技巧辅助：

```bash
gdb ./trader_race
(gdb) break match_thread
(gdb) break feed_thread
(gdb) run
(gdb) thread apply all bt     # 看两个线程分别停在哪个写 g_total 的地方
(gdb) info threads            # 列出所有线程
```

但 gdb 只能「停下来看现场」，**无法自动判定竞争**（它不知道两个访问之间有没有同步）。所以竞态排查 TSan 是首选，gdb 只是备选/补充。

## HFT 关联

1. **偶发错单的头号嫌疑**：下单结果偶发不对，先怀疑共享订单状态/统计量的竞态，而不是撮合算法逻辑错。TSan 能一次性把所有竞争点列出来。
2. **统计量是竞态重灾区**：`g_total` 这种「累计值」是典型的「人人都会碰、最容易忘加锁」的变量。真实系统里，监控指标、计数器、日志序列号都是竞态高发地。
3. **原子操作 vs 锁的取舍**：对 `g_total` 这种简单累加，`atomic_fetch_add` 在无争用下几乎零开销，比锁快得多，是 HFT 低延迟路径的标准选择。锁的优势在「多步操作的原子性」（如「检查 + 修改」），单步累加用原子更优。
4. **TSan 进 CI 但缩小范围**：TSan 内存开销大，撮合引擎全量跑会 OOM，CI 里用「TSan 编译 + 小数据集」专门抓竞态（4.3 讲过）。

```bash
# HFT 场景：CI 里 TSan 小数据集回归
gcc -g -O1 -fsanitize=thread -pthread -o engine_tsan matching_engine.c ...
./engine_tsan --sim tiny.csv      # 极小数据集，避免 TSan 内存爆炸
# 任一 data race → 非零退出 → CI 判失败
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** `g_total += o->qty` 一行代码为什么是数据竞争？

> 因为它在机器层面拆成「读 g_total → 加 qty → 写回」三步，不是原子的。两个线程这三步交错时互相覆盖：A 读到 1000，B 也读到 1000，各自加 qty 写回，A 的更新被 B 覆盖丢失。所以「一行代码」不等于「一个原子操作」，只要它内部有读-改-写，且两个线程无锁执行，就是数据竞争。

**Q2:** 为什么这个 bug「不崩不卡、结果时对时错」，比崩溃更难排查？

> 因为数据竞争的后果是「丢更新」而非「崩卡」：程序照常运行、不报错、不卡死，只是结果偶尔偏离正确值。而「丢多少」取决于线程调度的随机时序，所以偶发、结果漂移。崩溃至少会留 core、卡住至少能 strace，竞争则什么痕迹都不留，只能靠 TSan 这种「专门盯 happens-before 关系」的工具才能稳定暴露。

**Q3:** TSan 为什么能「稳定复现偶发竞争」？它和普通运行的本质区别？

> 普通运行只有「竞争造成错误结果」时才暴露问题（大多数时候碰巧对）。TSan 在**每次内存访问**时检查 happens-before 关系：只要两个无同步的线程访问同一位置且有一个写，**当场报**，不依赖结果错没错。它抓的是「竞争关系」本身而非「竞争后果」，所以即使某次结果碰巧对了，TSan 也稳定报出竞争。这是「偶发变稳定」的关键。

**Q4:** 修这个竞争，「加锁」和「原子操作」怎么选？各自适用场景？

> 对 `g_total += qty` 这种**单步累加**，用原子操作（`atomic_fetch_add`）更优：无锁、无争用时几乎零开销，是 HFT 低延迟首选。用锁（`pthread_mutex_lock`）更适合「多步操作的原子性」——比如「检查余额 + 扣款」这种必须作为一个整体、中间不能被打断的复合操作。单步累加用锁是「杀鸡用牛刀」，徒增争用和延迟。

**Q5:** gdb 多线程能替代 TSan 查竞态吗？为什么？

> 不能替代。gdb 只能「暂停程序、看各线程当前栈」，它无法判断「两个内存访问之间有没有 happens-before 关系」——因为它看不到同步关系的历史。gdb 能帮你「看到两个线程都在碰 g_total」，但「它们是否无锁并发」要人工推理，且停下来时竞争可能已经过去。TSan 是自动判定竞争的专用工具，gdb 只能作为「装不了 TSan」时的兜底补充。

</details>

## 交叉引用

- [7.2 崩溃 → coredump 定位](02-crash-coredump.md)
- [7.4 泄漏 → valgrind 定位](04-leak-valgrind.md)
- [4.3 TSan / Helgrind](../../chapter-04-concurrency/notes/03-threadsanitizer.md)
- [4.1 多线程调试](../../chapter-04-concurrency/notes/01-thread-debugging.md)
- [Ch7 实战](../README.md)
