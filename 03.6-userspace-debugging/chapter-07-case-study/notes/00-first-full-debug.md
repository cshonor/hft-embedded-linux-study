# 7.0′ 第一次全流程实战：一个程序，四类雷，九种跑法

> 🟢 零起点 · 前置：[1.0 学前篇](../../chapter-01-methodology/notes/00-gcc-first-steps.md) → [2.0′](../../chapter-02-crash/notes/00a-first-segfault.md) → [3.0′](../../chapter-03-memory/notes/00-first-asan-report.md) → [4.0′](../../chapter-04-concurrency/notes/00-first-tsan-race.md) → [5.0′](../../chapter-05-behavior/notes/00-first-strace-trace.md)
>
> 前面每一章练的是**一件**工具对付**一类**问题。真实世界的程序不这么客气：
> 一个多线程下单程序可以同时埋着崩溃、竞态、泄漏、卡死四种雷，
> 而且「结果对」不代表「没问题」。
> 这一篇不引入任何新工具，只做一件事：**把前六章串成一次完整体检**——
> 同一份代码骨架，用宏开关分别引爆四类雷，看每类雷落在哪个工具手里、
> 退出码是几。读完这一篇，1.2 那张「症状 → 工具」决策树就算真正长在你身上了。

---

## 先认识这台「体检床」

`code/c7_1_trader.c`：一个迷你下单引擎，两个线程——

- `feed` 线程：`malloc` 一张订单，塞进共享订单簿（加锁），塞满 200 单收工；
- `match` 线程：从订单簿头部摘单（加锁），累加成交额 `g_total`，`free` 掉。

**正确基线先钉死**：`g_total = 100+1 + 100+2 + … + 100+200 = 40100`。
这个数字是后面一切判断的锚——没有它，连「结果错了」都无从谈起。

同一份骨架，用编译宏换破坏点（控制变量法，1.3）：

| 变体 | 宏 | 埋的雷 |
|------|----|--------|
| 崩溃 | `-DBUG_CRASH` | feed 里 `slots[o->id]` 越界写栈（id 到 200，数组只有 16） |
| 竞态 | `-DBUG_RACE` | feed 和 match 无锁累加 `g_total` |
| 泄漏 | `-DBUG_LEAK` | match 摘单后不 `free` |
| 卡住 | `-DBUG_HANG` | feed 对同一把非递归锁连锁两次 → 自己等自己 |

## 九种跑法一张表：退出码就是体检结论

gcc 13.3.0 / clang 18.1.0 实测汇总（完整输出见 [`code/README.md`](../code/README.md)）：

| 跑法 | 关键输出 | 退出码 | 对应章 |
|------|----------|--------|--------|
| 基线 | `total = 40100`，malloc 200 / free 200 / 存活 0 | **0** | — |
| 基线 + ASan | `total = 40100`，无泄漏报告 | **0** | Ch3 |
| 基线 + TSan | `total = 40100` **但 TSan 报 1 条竞争** | **66** | Ch4 ⚠️ |
| 基线 + TSan + `-DFIX_RUNNING` | TSan 静默 | **0** | Ch4 |
| `-DBUG_CRASH` 无栈保护 | `Program terminated with signal SIGSEGV (11)` | **139** | Ch2 |
| `-DBUG_CRASH` + `-fstack-protector-all` | `*** stack smashing detected ***` | **134** | Ch2 |
| `-DBUG_RACE` + TSan | `total = 80200`（2 × 40100）；TSan 报 3 条 | **66** | Ch4 |
| `-DBUG_LEAK` + ASan | `6400 byte(s) leaked in 200 allocation(s)` | **1** | Ch3 |
| `-DBUG_HANG` | 看门狗 alarm 判定「自死锁实锤」 | **4** | Ch5 |

退出码对照（2.0′ 的信号分诊表在这里全部用上了）：
**139 = 128+11（SIGSEGV）、134 = 128+6（SIGABRT）**——被信号打死；
**66** = TSan 的约定退出码——工具主动报警；**1** = LeakSanitizer 报警；
**4** = 看门狗「判定它死了」主动收场。五种死法，五种气质，互不混淆。

## 全章最值得记住的一条：「正确版本」过不了 TSan

九行里最反直觉的是第 3 行：**基线结果 40100 完全正确、不崩不卡，
TSan 照样报 1 条数据竞争**：

```text
WARNING: ThreadSanitizer: data race
  Write of size 4 ... by thread T1:
    #0 feed_thread example.c:197      ← g_running = 0（一把锁都没拿）
  Previous read of size 4 ... by thread T2 (mutexes: write M0):
    #0 match_thread example.c:211     ← 读 g_running（正经拿着锁！）
  As if synchronized via sleep        ← 这次没出事纯靠 nanosleep 撞对了时序
  Location is global 'g_running'
```

三个教训，一个比一个深：

1. **锁只保护一边等于没锁**。match 读的时候拿着锁，feed 写的时候没拿——
   竞争成立只需要「至少一方写、双方无先后约束」，跟另一方拿不拿锁无关。
2. **`volatile` 不是同步原语**。`volatile int g_running` 只保证「每次真去读内存」，
   不提供任何线程间顺序保证（3.2 的同一条：volatile 不给原子性）。
3. **「一直跑得好好的」是最弱的证据**。TSan 那句 `As if synchronized via sleep`
   翻译过来就是：你没出事只是因为睡眠恰好把两次访问隔开了——**靠时序运气
   不是同步**。

修复（`-DFIX_RUNNING`）：`g_running` 换成 C11 `atomic_int`，退出判定挪进锁内。
改完 TSan 静默，退出码回到 0。

## 四个雷各自的「落网方式」

| 雷 | 落网方式 | 新手要看到的点 |
|----|----------|----------------|
| 崩溃 | 无保护时 SIGSEGV（139）；加 `-fstack-protector-all` 变 SIGABRT（134） | 同一个 bug 两种退出码：金丝雀**主动** abort，换来准确的死亡时刻，而不是几百行后莫名其妙地崩 |
| 竞态 | `total = 80200 = 2×40100`；TSan 报 3 条 | 报 3 条 ≠ 3 个独立 bug——第 3 条是骨架自带的 `g_running`；第 2 条是破坏点放错位置**连坐**出的 use-after-free |
| 泄漏 | LSan：`6400 字节 = 200 × sizeof(order_t)(32)` | 结果 40100 依然正确、程序正常跑完——**「结果对」发现不了泄漏**；没工具时它的唯一表现是内存曲线爬几小时后 OOM |
| 卡住 | 看门狗 `alarm(3)` 超时打印现场后 `_exit(4)` | 退出码 4 不是信号杀死，是「有人主动判定它死了」；现场三板斧（进度不涨 / 锁 EBUSY / 两线程都停）正好对应 5.0′ strace 的三个观察点 |

还有一个贯穿四个雷的暗线：**崩溃/泄漏变体的 stdout 都只剩开头两行**，
最后的统计行全丢——全缓冲 + 进程被信号杀死 = 缓冲区陪葬（5.0′ 的 `c5_1`
专讲这个坑，本章第三次实证）。开头两行能活下来，是因为源码里紧跟了
`fflush(stdout)`。**日志里必须 fflush，是拿三次事故换来的纪律。**

## 把九行跑法变成你的排查套路

```text
① 跑基线，记下正确锚点（这里是 40100）——没有锚点，一切判断免谈
② 症状分诊（1.2 决策树）：
     崩了？     → Ch2：退出码 128+N 查信号，gdb bt 看栈
     结果错？   → Ch4：TSan 挂上重跑（结果对也要挂！）
     慢慢漏？   → Ch3：ASan/LSan，或先自己数 malloc/free
     卡住了？   → Ch5：strace -p 看最后一句；gdb 看各线程栈顶
     只是慢？   → Ch6：采样，热点表第一名说话
③ 修复后回归：同一张九行表重跑一遍，全部回到预期才算完
```

第②步里最反直觉的一条再强调一次：**「结果对」不是免检金牌**。
基线 40100 分毫不差，TSan 照样抓出竞争；LSan 抓的泄漏更是结果完全正确。
所以体检的标准动作是**每个工具都过一遍**，而不是「出问题再说」。

## 衔接

- 前置：五篇 0′（见文首链路），尤其 [2.0′](../../chapter-02-crash/notes/00a-first-segfault.md) 的信号分诊表和 [4.0′](../../chapter-04-concurrency/notes/00-first-tsan-race.md) 的 TSan 报告读法
- 展开：7.1 程序结构与埋点（[notes/01-program-structure.md](01-program-structure.md)）、7.2 崩溃 coredump（[notes/02-crash-coredump.md](02-crash-coredump.md)）、7.3 竞态多线程定位（[notes/03-race-tsan.md](03-race-tsan.md)）、7.4 泄漏 valgrind/ASan（[notes/04-leak-valgrind.md](04-leak-valgrind.md)）、7.5 卡住 strace（[notes/05-hang-strace.md](05-hang-strace.md)）
- demo：`code/c7_1_trader.c` + [`code/README.md`](../code/README.md)（九种跑法完整输出 + 四个踩坑记录，含「-DBUG 写进程序参数导致五变体全跑成正确版」这种验证事故）

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 九行跑法表里，哪些退出码是「被信号打死」、哪些是「工具主动报警」？各举一个。

> 被信号打死：139 = 128+11（SIGSEGV，越界写踩到未映射内存）、
> 134 = 128+6（SIGABRT，栈金丝雀主动 abort）。
> 工具主动报警：66（TSan 发现竞争后按约定退出）、1（LSan 发现泄漏）、
> 4（看门狗 alarm 超时后主动 _exit，属于「有人判定它死了」）。
> 区分意义：信号码指向「死法」（查信号分诊表），工具码指向「哪类问题
> 被哪个工具抓到」——两者提供的下一步动作完全不同。

**Q2:** 基线版本结果 40100 完全正确，为什么 TSan 还报警？match 读 `g_running` 时明明拿着锁。

> 因为竞争的成立条件是「至少一方写、且两次访问之间没有先后约束」——
> 跟另一方拿不拿锁无关。feed 写 `g_running = 0` 时不在任何临界区，
> match 持锁读它并不能约束 feed。锁是「约定」，只对遵守约定的人生效；
> 有一方不遵守，约定就形同虚设。加上 volatile 只保证真访存、
> 不提供线程间顺序，所以「结果对」只是 nanosleep 撞对了时序的运气
> （TSan 原话 As if synchronized via sleep）。

**Q3:** `-DBUG_RACE` 的结果是稳定的 80200 而不是「偶尔差一点」，这个细节说明了什么？TSan 报的 3 条是不是 3 个 bug？

> 80200 = 2 × 40100：feed 和 match 各自把全部订单加了一遍——
> 这个变体的破坏方式决定了后果是「稳定翻倍」，不是丢更新；
> Ch4 的 c4_1 那种「结果差万分之几」才是丢更新型。两者 TSan 都能当场抓出。
> 3 条报告 ≠ 3 个独立 bug：第 1 条是 g_total 无锁累加（本次埋的雷），
> 第 2 条是雷的位置不当连坐出的 use-after-free（feed 在锁外读已 free 的订单），
> 第 3 条是骨架自带的 g_running 竞争。修 bug 要按「根因」聚类，不能按报告条数。

**Q4:** 泄漏变体结果 40100 正确、程序正常退出，为什么还说它是 bug？LSan 报告里的 6400 这个数字怎么验算？

> 泄漏的代价不在单次运行的正确性，而在**长期运行**：每张订单 32 字节
> 只进不出，内存曲线单调上涨，跑几小时到几天后 OOM——交易系统的
> 真实死法。验算：LSan 报 6400 字节 / 200 块，程序自报 malloc 200 / free 0，
> sizeof(order_t) = 32（int id + 4 字节填充 + double price + long qty +
> 指针 next，对齐后 32），200 × 32 = 6400，三处完全对上。
> 结构体填充是「内存账算不平」的常见原因，别按字段宽手算。

**Q5:** 为什么四个雷里崩了三个（CRASH/LEAK/HANG）的 stdout 都只剩开头两行？这对写日志有什么直接要求？

> stdout 默认全缓冲：printf 只是把内容写进用户态缓冲区，攒够或换行才
> 真正 write 给内核。进程被信号杀死（或 LSan/看门狗 _exit）时，
> 没来得及 flush 的缓冲区内容全部陪葬——统计行就是这么丢的。
> 开头两行能活下来是因为源码紧跟 fflush(stdout)。
> 直接要求：关键日志（尤其是「程序跑到哪了」的心跳行）必须 fflush，
> 或直接用无缓冲的 stderr / write(2)。事故现场的日志完整度，
> 取决于你出事前的 fflush 纪律。

</details>
