# 4.3 TSan / Helgrind 数据竞争检测

> 🔴 精读 · 抓「偶发、难复现、位置漂移」的数据竞争（data race）

## 本节要点

多线程程序里最阴险的一类 bug 是**数据竞争**：两个线程同时访问同一块内存、至少一个是写、且两者之间**没有同步**（没加锁、没原子操作、没 happens-before 关系）。它导致结果「时对时错」、崩溃点漂移，gdb 单步根本没法复现。ThreadSanitizer（TSan，`-fsanitize=thread`）和 Helgrind（valgrind 子工具）是专门抓它的工具——本节讲数据竞争的定义、TSan 原理与报错解读、与 Helgrind 的取舍。

## 先看数据竞争长什么样

```c
// race.c —— 经典数据竞争：非原子自增
#include <pthread.h>
#include <stdio.h>

int g_counter = 0;          // 共享变量，无任何保护

void *incr(void *arg) {
    for (int i = 0; i < 1000000; i++)
        g_counter++;        // 读-改-写，非原子 → 数据竞争
    return NULL;
}

int main(void) {
    pthread_t t1, t2;
    pthread_create(&t1, NULL, incr, NULL);
    pthread_create(&t2, NULL, incr, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    printf("counter = %d (expected 2000000)\n", g_counter);
    return 0;
}
```

普通编译运行，结果「时对时错」：

```bash
gcc -O2 -pthread -o race race.c
./race   # 第一次
# counter = 1999998   ← 丢了 2 次更新
./race   # 第二次
# counter = 1999991   ← 这次丢了 9 次，每次都不一样
```

`g_counter++` 其实是三步：读 → 加一 → 写回。两个线程的这三步交错执行时，就会「互相覆盖」——A 读到 100，B 也读到 100，各自加一写回 101，两次自增只涨了 1。**丢了更新**，且丢多少取决于调度，所以偶发、难复现。

## 什么是数据竞争（严格定义）

数据竞争 = 同时满足三个条件：

| 条件 | 说明 |
|------|------|
| ① 两个线程访问**同一内存位置** | 同一变量、同一结构体字段 |
| ② 至少一个是**写** | 都是读没关系（读读安全） |
| ③ 两者之间**无 happens-before** | 没有锁、原子操作、join、信号量等同步关系 |

> 关键：数据竞争是**未定义行为**（UB），不是「结果差一点点」。C11/C++11 内存模型规定，含数据竞争的程序行为未定义——编译器可以基于「无竞争」假设做优化，导致更诡异的结果。所以它不是「容错问题」而是「必须修」的正确性 bug。

## TSan：编译期插桩 + happens-before 追踪

ThreadSanitizer 是 `-fsanitize=thread`，编译期插桩：

```bash
gcc -g -O1 -fsanitize=thread -pthread -o race_tsan race.c
./race_tsan
```

报错：

```text
==================
WARNING: ThreadSanitizer: data race (pid=12345)
  Write of size 4 at 0x7b0400000080 by thread T2:
    #0 incr race.c:9            ← 写方：T2 在 race.c:9 写 g_counter
    #1 <null> <null>

  Previous write of size 4 at 0x7b0400000080 by thread T1:
    #0 incr race.c:9            ← 另一个写方：T1 也在 race.c:9 写
    #1 <null> <null>

  Location is global 'g_counter' of size 4 at 0x7b0400000080 (race+0x...)

  Thread T2 (tid=..., running) created by main thread at:
    #0 pthread_create race.c:17   ← T2 在哪创建
  Thread T1 (tid=..., running) created by main thread at:
    #0 pthread_create race.c:16   ← T1 在哪创建

SUMMARY: ThreadSanitizer: data race race.c:9 in incr
==================
```

解读三步：

1. **冲突双方**：报告给出「当前访问」（Write by T2）和「历史访问」（Previous write by T1），两者都指向 `race.c:9`——**同一行代码，两个线程都在这写**，实锤。
2. **位置**：`Location is global 'g_counter'` —— 竞争的是哪个变量。
3. **线程来源**：`Thread T2 created by ... pthread_create race.c:17` —— 两个线程分别在哪创建，帮你定位「谁在并发」。

## 修复：加锁 / 原子 / 局部化

TSan 报的每个竞争，标准修法有四种：

```c
// 修法 1：互斥锁（通用）
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
void *incr(void *arg) {
    for (int i = 0; i < 1000000; i++) {
        pthread_mutex_lock(&m);
        g_counter++;
        pthread_mutex_unlock(&m);
    }
    return NULL;
}

// 修法 2：原子操作（无锁，HFT 常用，低延迟）
#include <stdatomic.h>
atomic_int g_counter = 0;
void *incr(void *arg) {
    for (int i = 0; i < 1000000; i++)
        atomic_fetch_add(&g_counter, 1);
    return NULL;
}

// 修法 3：每线程局部累加，最后合并（避免共享热点，HFT 最优）
void *incr(void *arg) {
    long local = 0;               // 线程私有
    for (int i = 0; i < 1000000; i++)
        local++;
    g_counter += local;           // 只最后合并一次（仍需同步）
    return NULL;
}
```

> 修法 3 是 HFT 的核心思路：**把共享写入降到最少**。每线程在私有变量里算，最后一次性合并，锁的竞争从「每次自增」降到「每线程一次」。

## TSan 的局限（必须诚实）

1. **不能与 ASan 同开**：TSan 需要独占运行时（它和 ASan 都接管内存访问），`-fsanitize=address,thread` 会直接报错。查内存和查并发要**分开两次编译**。
2. **内存开销大**：TSan 为每个内存位置维护访问历史（shadow），典型开销 5–10× 内存、5–15× 时间。大程序可能跑不动，需要缩小测试范围。
3. **只抓「实际发生的」竞争**：TSan 是动态检测，只报执行路径上真实发生的竞争。没跑到的代码路径里的竞争它看不见（和 ASan 同理）。
4. **假阳性可能**：极少数情况下，TSan 的 happens-before 追踪不完整（比如用了它不认识的自定义同步原语），可能误报。真报错要人工确认「是否真的无同步」。

## Helgrind：无需重编译的竞态检测

Helgrind 是 valgrind 的并发子工具，和 memcheck 一样**无需重编译**：

```bash
gcc -O1 -pthread -o race race.c   # 正常编译，不加 sanitize
valgrind --tool=helgrind ./race
# ==12345== Possible data race during write of size 4 at 0x... by thread #3
# ==12345==    at 0x...: incr (race.c:9)
# ==12345==  This conflicts with a previous write of size 4 by thread #2
# ==12345==    at 0x...: incr (race.c:9)
```

| 维度 | TSan | Helgrind |
|------|------|----------|
| 原理 | 编译期插桩 + happens-before | 动态二进制翻译 + happens-before |
| 需重编译 | ✅ 必须 `-fsanitize=thread` | ❌ 不需要 |
| 性能开销 | 5–15× 时间 / 5–10× 内存 | 约 100× 时间（极慢） |
| 死锁检测 | 弱（主要抓竞争） | ✅ 强（锁序分析，能报潜在死锁） |
| 适用 | 开发期 CI 常驻 | 现有二进制定性、锁序排查 |

> **取舍**：TSan 快、进 CI；Helgrind 慢但**能抓潜在死锁**（它分析锁的获取顺序，报「锁序不一致」这类 TSan 不报的问题），且对没有源码的二进制可用。两者互补。

## HFT 关联

1. **偶发错单的第一嫌疑是竞态**：下单结果偶发不对，先别怀疑撮合算法，先上 TSan 查共享订单状态是不是没锁好。1.1 分类学的经验法则：多线程「偶发、难复现、位置漂移」九成是并发或内存问题。
2. **原子操作是低延迟首选**：HFT 里锁的争用会造成延迟毛刺，`atomic_fetch_add` 这类无锁原语在无争用下几乎零开销，是计数器、序列号、状态标志的标准做法。
3. **每线程私有 + 最后合并**：把共享写入降到最低，是 HFT 降低锁竞争、减少 cache 失效（false sharing）的核心手段。TSan 帮你在开发期确认「哪些变量还在被并发写」。
4. **TSan 进 CI 但缩小范围**：TSan 内存开销大，撮合引擎全量跑可能 OOM。CI 里通常用「TSan 编译 + 小数据集回归」的方式，专门抓竞态。

```bash
# HFT 场景：CI 里对撮合引擎跑 TSan 回归（小数据集）
gcc -g -O1 -fsanitize=thread -pthread -o engine_tsan matching_engine.c ...
./engine_tsan --sim tiny.csv     # 用极小数据集，避免 TSan 内存爆炸
# 任一 data race → 非零退出 → CI 判失败
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 数据竞争的三个必要条件是什么？「两个线程都读同一变量」算竞争吗？

> 三个条件：①两个线程访问同一内存位置；②至少一个是写；③两者之间无 happens-before 关系（无锁/原子/join 等同步）。「都读」不构成竞争——读读是安全的，因为没人改。只有「至少一方写」才可能产生竞争，因为写会改变别人读到的值。

**Q2:** `g_counter++` 明明是「一行代码」，为什么也有数据竞争？

> 因为 `g_counter++` 在机器层面不是原子操作，它拆成三步：读内存（load）→ 加一 → 写回（store）。两个线程的这三步可能交错：A 读 100，B 也读 100，各自加一写回 101，两次自增只涨 1，丢掉一次更新。这就是「读-改-写」（RMW）竞态，一行高级语言代码不等于一个原子操作。

**Q3:** TSan 和 Helgrind 的核心区别？怎么选？

> 核心区别在实现：TSan 是编译期插桩（需 `-fsanitize=thread` 重编译，快，约 5–15×），Helgrind 是动态二进制翻译（无需重编译，极慢，约 100×）。选择：开发期 CI 常驻用 TSan；拿到没有源码的二进制、或要查「潜在死锁/锁序错误」用 Helgrind（它锁序分析更强，能报 TSan 不报的潜在死锁）。

**Q4:** 为什么 TSan 不能和 ASan 一起开？

> 因为两者都接管内存访问，会冲突：ASan 用自己的 shadow memory 记录地址合法性，TSan 也用 shadow 记录访问历史，两个运行时抢占同一资源、且插桩代码互相干扰，编译器直接禁止 `-fsanitize=address,thread` 组合。查内存错误和查数据竞争必须分开两次编译、分别跑。

**Q5:** TSan 报了一个 data race，但你说「偶发难复现」，为什么 TSan 能稳定复现？

> 因为 TSan 不依赖「竞争造成错误结果」才报，它是在**每次**内存访问时检查 happens-before 关系：只要两个无同步的线程访问同一位置且有一个写，**当场**就报，不管这次调度有没有真的丢更新。它抓的是「竞争关系」本身，而不是「竞争的后果」，所以即使某次运行结果碰巧对了，TSan 也稳定报出竞争。

</details>

## 交叉引用

- [4.1 多线程调试](01-thread-debugging.md)
- [4.2 rr 可逆调试](02-rr-reversible-debugging.md)
- [3.3 UBSan（为什么不能与 TSan 同开）](../../chapter-03-memory/notes/03-undefinedbehaviorsanitizer.md)
- [Ch4 并发类](../README.md)
