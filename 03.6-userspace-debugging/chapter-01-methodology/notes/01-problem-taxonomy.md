# 1.1 问题分类学（崩溃 / 内存 / 并发 / 行为 / 性能）

> 🔴 精读 · 调试的第一步是「定性」

## 本节要点

调试最大的浪费是「用错工具」：拿着 gdb 查内存泄漏（gdb 不擅长），拿着 strace 查数据竞争（strace 看不到）。**先给问题定性——它属于哪一类——再选工具**，能省掉 80% 的瞎试时间。本节建立「五类问题」的分类学框架，它是本模块后续所有章节的索引。

## 五类问题总表

| 类别 | 典型症状 | 根因方向 | 第一工具 | 对应章节 |
|------|----------|----------|----------|----------|
| **崩溃** (crash) | 段错误、SIGABRT、非法指令、栈破坏 | 野指针 / 越界写 / 栈溢出 / 除零 | gdb + coredump | Ch2 |
| **内存** (memory) | 内存持续增长、偶发崩溃、值不对 | 泄漏 / 越界 / use-after-free / 未初始化读 | valgrind / ASan | Ch3 |
| **并发** (concurrency) | 结果时对时错、偶发死锁、竞态 | 数据竞争 / 锁序错误 / 忘记加锁 | TSan / gdb 多线程 | Ch4 |
| **行为** (behavior) | 卡住不动、调了不该调的、参数错 | 阻塞等 IO / 等锁 / 多余 syscall | strace / ltrace | Ch5 |
| **性能** (performance) | 太慢、CPU 高、延迟毛刺 | 热点函数 / cache miss / 多余拷贝 | perf | Ch6 |

## 为什么「分类」是第一步

调试工具是按**问题域**设计的，每个工具只能「看见」某一类问题：

```
strace 只能看见 syscall ──→ 看不见内存越界、数据竞争
valgrind 只能看见内存 ────→ 看不见 syscall 时序、性能热点
TSan 只能看见数据竞争 ────→ 看不见内存泄漏
perf 只能看见性能 ────────→ 看不见逻辑错误
```

**工具是「滤镜」**，选错滤镜，问题就「隐身」了。所以流程必须是：**症状 → 定性 → 选工具**，而不是「先打开 gdb 再说」。

## 五类问题的「边界模糊区」

真实 bug 常跨类，要会识别主次：

| 现象 | 可能是哪几类？ | 判断方法 |
|------|----------------|----------|
| 偶发崩溃 | 崩溃 + 内存（UAF）+ 并发（竞态写坏） | 先 coredump 看崩溃点，若崩溃点每次不同 → 怀疑内存/并发 |
| 结果时对时错 | 并发 + 内存（未初始化读） | TSan 抓竞态；ASan 抓未初始化 |
| 卡住 | 行为（等 IO）+ 并发（死锁） | strace 看停在哪：`recvfrom` 是等 IO，`futex` 是等锁 |
| 偶发段错误且只在多线程下 | 崩溃 + 并发 | TSan 优先，因为竞态往往是崩溃的根因 |

> **经验法则**：多线程程序里「偶发、难复现、位置漂移」的 bug，九成是并发或内存问题，不是逻辑问题——先上 TSan/ASan，别在 gdb 里死磕单线程现场。

## 分类学与 05.6 的对称

用户态五类问题，在内核态有精确对应（这正是 03.6 ↔ 05.6 对称的依据）：

| 问题类型 | 用户态工具 (03.6) | 内核态工具 (05.6) |
|----------|-------------------|-------------------|
| 崩溃 | coredump + gdb | Oops 日志 |
| 内存 | valgrind / ASan | KASAN / kmemleak |
| 并发 | TSan / helgrind | KCSAN / LOCKDEP |
| 行为 | strace / ltrace | printk / ftrace |
| 性能 | perf | perf |

## 动手：把三类雷装进一个程序（实测）

分类学不用背，**跑一遍就有体感**。下面这份程序把「崩溃 / 内存 / 并发」三颗雷装在一起，用 argv 选：

```c
/* code/c1_1_three_bugs.c 节选 */
static long g_orders_matched = 0;

static void *match_worker(void *arg)          /* 并发：非原子读改写 */
{
    long n = (long)arg;
    for (long i = 0; i < n; i++)
        g_orders_matched++;
    return NULL;
}

static int crash_null_deref(void)             /* 崩溃：解引用 NULL */
{
    struct order { long id; long qty; };
    struct order *o = NULL;

    printf("case 1: 准备解引用一个 NULL 订单指针\n");
    fflush(stdout);
    o->qty = 100;                             /* ← SIGSEGV 在这里 */
    return 0;
}

static int leak_orders(void)                  /* 内存：只借不还 */
{
    for (int i = 0; i < 100; i++) {
        char *fill = malloc(400);             /* 40000 B，全部未 free */
        if (!fill) return 1;
        memset(fill, 0, 400);
    }
    printf("case 2: 100 个 400 字节的订单快照已分配（全部未释放）\n");
    return 0;
}
```

完整源码见 `code/c1_1_three_bugs.c`（含 `case 3` 的双线程计数）。**三种症状、三种编译方式，实测结果如下**（gcc 13.3.0 / clang 18.1.0，基础参数 `-g -O0 -Wall -Wextra`）：

| case | 类别 | 编译方式 | 实测退出码 | 实测输出（关键行） |
|------|------|----------|-----------|-------------------|
| 1 | 崩溃 | 裸编译 | **139** | `Program terminated with signal SIGSEGV (11)` |
| 2 | 内存 | `-fsanitize=address` | **1** | `ERROR: LeakSanitizer: detected memory leaks`<br>`Direct leak of 40000 byte(s) in 100 object(s) allocated from:`<br>`SUMMARY: AddressSanitizer: 40000 byte(s) leaked in 100 allocation(s).` |
| 2 | 内存 | 裸编译 | **0** | `case 2: 100 个 400 字节的订单快照已分配（全部未释放）` ← 一声不吭 |
| 3 | 并发 | `clang -fsanitize=thread` | **66** | `WARNING: ThreadSanitizer: data race (pid=2)`<br>`Location is global 'g_orders_matched' of size 8 at 0x5555569dc658` |
| 3 | 并发 | 裸编译 | **0** | `case 3: 期望撮合 200000 笔，实际记到 200000 笔` ← 计数居然一分不差 |

三件事一眼可见：

1. **退出码 = 128 + 信号号**：SIGSEGV(11) → 139、SIGFPE(8) → 136。这是 shell 里判断「怎么死的」最快的办法（`./prog; echo $?`）。TSan 检出问题则用自己的约定 **66**。
2. **内存类和并发类在裸编译下「完全正常」**：case 2 退出 0 还打印了成功信息；case 3 的计数甚至 **200000 分毫不差**（竞争存在 ≠ 一定丢更新，这正是竞态「偶发」的物理来源）。**「看起来没问题」不等于没问题**——这就是这两类问题必须上专用工具的原因。
3. **工具是分类学的执行者**：同一份程序，`-fsanitize=address` 让它吐泄漏，`-fsanitize=thread` 让它吐竞态，裸编译让它一声不吭。「问题类型 ↔ 工具」的对应在这里是硬的。

> ⚠️ **踩坑提醒**：并发那颗雷必须用 **clang** 的 TSan。gcc 13.3 的 TSan 在云编译环境（Compiler Explorer 容器）里直接起不来：
> ```text
> FATAL: ThreadSanitizer: unexpected memory mapping 0x7153d7872000-0x7153d7d00000
> ```
> 退出码也是 66，但**没有任何 race 报告**——看到这行 FATAL 要意识到「是环境不支持 ASLR 固定映射」，不是「程序没有竞态」。换 clang 立刻正常。

## HFT 关联

1. **分诊决定响应速度**：交易进程在生产出问题时，第一句话先问「崩了 / 慢了 / 卡了 / 结果错了 / 内存涨了」——这五选一定位了该 dump core、上 perf、上 strace 还是上 TSan，而不是盲目重启掩盖问题。
2. **并发与内存是 HFT 重灾区**：多线程行情/下单程序里，竞态和 UAF 是最隐蔽、最致命的两类（偶发错单、偶发崩溃），分类学提醒你**优先怀疑这两类**。
3. **行为类是「系统集成」的入口**：链路不通、配置读错、端口没绑上，这类问题 stract 一眼可见，别上来就怀疑算法逻辑。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么说「选错工具，问题就隐身了」？

> 因为调试工具本质是**滤镜**，每个工具只能观测特定问题域：strace 只看 syscall（看不见内存越界）、valgrind 只看内存（看不见 syscall 时序）、TSan 只看数据竞争（看不见泄漏）、perf 只看性能（看不见逻辑错误）。问题类型和工具观测域不匹配时，问题在工具视角下不存在，自然「隐身」。

**Q2:** 一个多线程程序「偶发段错误、每次崩溃点不同」，最该先怀疑哪类问题？为什么？

> 优先怀疑**并发**（或内存）。「偶发、难复现、崩溃位置漂移」是多线程竞态的典型签名——多个线程写同一块内存没加锁，谁先写坏、写到哪不确定，所以崩溃点随机。逻辑 bug 通常是「稳定、可复现、固定位置」。这种情况先上 TSan 抓竞态，比在 gdb 里单步死磕高效。

**Q3:** 进程「卡住不动」，可能属于哪两类？怎么区分？

> 可能是**行为类**（阻塞等 IO）或**并发类**（死锁等锁）。用 strace attach 看最后停在哪：停在 `recvfrom`/`read` 是等网络/文件数据（行为类，查对端）；停在 `futex(...FUTEX_WAIT...)` 是等锁（并发类，查锁序）。两者排查方向完全不同。

**Q4:** 「分类学」和「工具清单」的学习顺序应该是什么？为什么？

> 应该**先学分类学、再学工具**。因为分类学是「地图」，告诉你有哪些问题类型、每种该往哪走；工具清单是「路标」，具体教某条路怎么走。没有地图，路标再多也不知道该去哪个。这就是本章（Ch1 方法论）放在所有工具章前面的原因。

**Q5:** 上面实测里 case 3 的计数是 `200000`，一分不差，为什么 TSan 却报 data race？「结果对」能证明「没有 bug」吗？

> 不能。`g_orders_matched++` 在机器层面是「读 → 加 → 写」三条指令，两个线程可能交错成「都读到 N，都写回 N+1」，丢更新。但**丢不丢取决于当时的交错**——两次运行都是 200000，只说明这两次恰好没交错到丢失的那一步（10 万次循环里窗口极小）。TSan 看的是**有没有形成竞争的内存访问对**（`Location is global 'g_orders_matched'`），与「这次恰好有没有丢」无关。所以竞态是「偶发」的：裸编译下表现为 99.99% 正常 + 0.01% 错单，这才是它在 HFT 里最可怕的地方。

</details>

## 交叉引用

- [1.2 症状 → 工具决策树](02-symptom-to-tool.md)
- [Ch2 崩溃类](../../chapter-02-crash/README.md)
- [03.6 模块导读](../../README.md)
