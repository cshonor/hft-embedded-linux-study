# 7.1 程序结构（多线程 / 网络 / 共享内存的 bug 埋点）

> 🔴 精读 · 一个程序、四类雷，把前六章串起来

## 本节要点

本章用一个**迷你下单引擎 `trader.c`** 贯穿全部五节：它有两个线程（feed 收单 + match 撮合）共享一个订单簿，同时埋了**四类雷**——崩溃、竞态、泄漏、卡住，分别对应 Ch2/Ch3/Ch4/Ch5 的工具。本节先讲清程序架构和埋点，后面四节各击破一类。核心思路：**真实系统的 bug 不是「一次一个」，而是「一个程序里藏着多类问题」，要会分诊 + 多工具接力**。

## 程序架构

```
                    ┌─────────────────────────────┐
                    │       共享订单簿 g_book       │
                    │  （链表，锁 g_book_lock 保护） │
                    └───────┬─────────────┬───────┘
               加锁插入    │             │   加锁摘除
              ┌───────────▼───┐    ┌────▼────────────┐
              │  feed 线程     │    │  match 线程      │
              │ 模拟收单：      │    │ 撮合：           │
              │ malloc 订单 →  │    │ 摘头部订单 →     │
              │ 塞进订单簿      │    │ 累计 g_total →   │
              └───────────────┘    │ free 成交订单     │
                                    └──────────────────┘
                         共享统计量 g_total（锁 g_stat_lock 保护）
```

- **feed 线程**：循环 200 次，每次 malloc 一个订单塞进订单簿头部（生产）。
- **match 线程**：循环摘订单簿头部订单，累加 qty 到 `g_total`，然后 free（消费）。
- **两把锁**：`g_book_lock` 保护订单簿链表，`g_stat_lock` 保护统计量 `g_total`。

## 完整代码（含四类雷，宏独立开关）

```c
// trader.c —— 迷你下单引擎（Ch7 贯穿示例）
// 架构：feed 线程模拟收单 + match 线程撮合，共享订单簿 g_book
// 默认（无宏）= 正确版本；四类雷用 -DBUG_xxx 独立开启：
//   -DBUG_CRASH   崩溃：feed 里订单 id 越界写栈数组
//   -DBUG_RACE    竞态：feed/match 无锁累加 g_total
//   -DBUG_LEAK    泄漏：成交订单摘除后不 free
//   -DBUG_HANG    卡住：feed 同一线程重复锁 g_book_lock（自死锁）
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

typedef struct order {
    int    id;
    double price;
    long   qty;
    struct order *next;
} order_t;

order_t *g_book = NULL;
pthread_mutex_t g_book_lock = PTHREAD_MUTEX_INITIALIZER;  // 保护订单簿
pthread_mutex_t g_stat_lock = PTHREAD_MUTEX_INITIALIZER;  // 保护统计量
volatile int g_running = 1;
volatile long g_total = 0;        // 累计撮合 qty

void *feed_thread(void *arg) {
    for (int i = 1; i <= 200; i++) {
        order_t *o = malloc(sizeof(order_t));
        o->id = i;
        o->price = (double)(i * 10);
        o->qty = 100 + i;

#ifdef BUG_CRASH
        {   // 雷1（崩溃）：id 当数组下标，id 最大 200 远超 16 → 越界写栈
            int slots[16];
            slots[o->id] = 1;    // 越界写破坏栈帧 → 偶发段错误
        }
#endif

#ifdef BUG_HANG
        pthread_mutex_lock(&g_book_lock);    // 第一次锁，成功
        pthread_mutex_lock(&g_book_lock);    // 雷4：同线程再锁（非递归锁）→ 自死锁
        o->next = g_book;                    // 永远到不了这里
        g_book = o;
        pthread_mutex_unlock(&g_book_lock);
        pthread_mutex_unlock(&g_book_lock);
#else
        pthread_mutex_lock(&g_book_lock);
        o->next = g_book;
        g_book = o;
        pthread_mutex_unlock(&g_book_lock);
#endif

#ifdef BUG_RACE
        g_total += o->qty;                   // 雷2：feed 无锁写 g_total
#endif
        usleep(1000);
    }
    g_running = 0;
    return NULL;
}

void *match_thread(void *arg) {
    while (g_running || g_book) {
        pthread_mutex_lock(&g_book_lock);
        order_t *o = g_book;
        if (o) {
            g_book = o->next;

#ifdef BUG_RACE
            g_total += o->qty;               // 雷2：match 无锁写 g_total（与 feed 竞争）
#else
            pthread_mutex_lock(&g_stat_lock);
            g_total += o->qty;
            pthread_mutex_unlock(&g_stat_lock);
#endif

#ifdef BUG_LEAK
            /* 雷3（泄漏）：成交订单摘除后不 free */
#else
            free(o);
#endif
        }
        pthread_mutex_unlock(&g_book_lock);
        usleep(500);
    }
    return NULL;
}

int main(void) {
    pthread_t feed, match;
    pthread_create(&feed, NULL, feed_thread, NULL);
    pthread_create(&match, NULL, match_thread, NULL);
    pthread_join(feed, NULL);
    pthread_join(match, NULL);
    printf("total matched qty = %ld\n", g_total);
    return 0;
}
```

## 四类雷埋点一览

| 雷 | 宏 | 埋在哪 | 对应章节 | 症状 |
|----|----|--------|----------|------|
| 崩溃 | `BUG_CRASH` | feed：`slots[o->id]` 越界写栈数组 | Ch2 coredump | 偶发段错误 |
| 竞态 | `BUG_RACE` | feed + match：无锁累加 `g_total` | Ch4 TSan | 结果时对时错 |
| 泄漏 | `BUG_LEAK` | match：摘除订单后不 free | Ch3 valgrind | 内存持续增长 |
| 卡住 | `BUG_HANG` | feed：同线程重复锁 `g_book_lock` | Ch5 strace | 程序永久卡死 |

## 正确版本先跑通

```bash
gcc -g -O0 -pthread -o trader trader.c
./trader
# total matched qty = 40100
```

> `g_total = 100+1 + 100+2 + ... + 100+200 = 101+102+...+300 = 40100`。这是「正确基线」，后面每个雷的排查都以「为什么结果不是 40100 / 为什么崩 / 为什么卡」为出发点。

## 编译各雷的命令速记

```bash
gcc -g -O0 -pthread -o trader_crash -DBUG_CRASH trader.c     # 7.2 崩溃
gcc -g -O1 -pthread -fsanitize=thread -o trader_race -DBUG_RACE trader.c  # 7.3 竞态
gcc -g -O0 -pthread -o trader_leak -DBUG_LEAK trader.c       # 7.4 泄漏
gcc -g -O0 -pthread -o trader_hang -DBUG_HANG trader.c       # 7.5 卡住
```

## 动手：把 `trader.c` 变成真能跑的文件（实测）

上面那段代码已经不是纸上谈兵了 —— 它是 [`code/c7_1_trader.c`](../code/c7_1_trader.c)，
一个能编译能跑的文件。和笔记版本相比多了两样东西，都是为了「在没有 valgrind / gdb
的环境里也能演示」：

| 增加的东西 | 为什么加 |
|------------|----------|
| `main` 里的 `alarm(3)` 看门狗 | 容器里没有 `timeout` 命令，卡死的进程没法收场；而且「进程自己报警」本身也是生产环境的做法 |
| 订单分配计数（`g_alloc` / `g_freed`） | 没有 valgrind 时，「自己数 malloc/free」是唯一可用的降级手段 |

### 基线先跑通

```bash
cc -g -O0 -pthread -Wall -Wextra -o c7_1_base code/c7_1_trader.c
./c7_1_base
```

实测（gcc 13.3.0）：

```text
迷你下单引擎：feed 塞 200 单，match 撮合；正确基线 = 40100
变体： 无（正确版本）

total matched qty = 40100   （期望 40100）
订单计数：malloc 200 个，free 200 个，仍存活 0 个（每个 32 字节，约 0 字节）
进程进度：feed 到 200/200，match 走了 356 轮
```

- `40100 = (100+1) + (100+2) + … + (100+200)` —— **正确基线**，后面每个变体都以它为锚。
- `malloc 200 / free 200 / 仍存活 0` —— 基线没有泄漏。
- `match 走了 356 轮` > 200：match 要多空转几轮才确认「feed 已收工 + 订单簿已空」。
- 那句 `变体： …` 是**自报家门**，不是装饰。下面坑 1 会讲它救了多大的场。

### 一个程序，九种跑法

| 跑法 | 关键输出 | 退出码 |
|------|----------|--------|
| 基线 | `total = 40100`；malloc 200 / free 200 / live 0 | **0** |
| 基线 + TSan | `total = 40100` **正确**，但 TSan 报 **1 条** `g_running` 竞争 | **66** |
| 基线 + TSan + `-DFIX_RUNNING` | TSan **静默** | **0** |
| 基线 + ASan | `total = 40100`，无泄漏报告 | **0** |
| `-DBUG_CRASH`（无栈保护） | `Program terminated with signal SIGSEGV (11)` | **139** |
| `-DBUG_CRASH` + `-fstack-protector-all` | `*** stack smashing detected ***` | **134** |
| `-DBUG_RACE` + TSan | `total = 80200`（= 2×40100）；TSan 报 **3 条** | **66** |
| `-DBUG_LEAK` + ASan | `LeakSanitizer: 6400 byte(s) leaked in 200 allocation(s)` | **1** |
| `-DBUG_HANG` | 看门狗判定「自死锁实锤」 | **4** |

**注意第 2 行**：这一行推翻了本节的一个隐含假设。见下。

### ⚠️ 「正确版本」其实是有 bug 的

这个变体（`-DBUG_RACE`）看似只是把「正确版本」的统计量保护拿掉，但实测发现：
**即使完全不打开任何 `-DBUG_*` 宏，把基线交给 TSan，它也会报 1 条数据竞争。**

```bash
cc -g -O1 -pthread -fsanitize=thread -o c7_1_tsan code/c7_1_trader.c
./c7_1_tsan            # 基线代码 + TSan → 退出码 66
```

```text
  Write of size 4 at 0x555555670b78 by thread T1:
    #0 feed_thread /app/example.c:197:5          ← g_running = 0（无锁写）
  Previous read of size 4 at 0x555555670b78 by thread T2 (mutexes: write M0):
    #0 match_thread /app/example.c:211:28        ← while 条件里读 g_running
  As if synchronized via sleep:
    #1 sleep_us /app/example.c:90:5
    #2 feed_thread /app/example.c:195:9
  Location is global 'g_running' of size 4
SUMMARY: ThreadSanitizer: data race /app/example.c:197:5 in feed_thread
```

根因：`volatile int g_running` **不是同步原语**（和 3.2 里 `volatile` 不给原子性
是同一条规则）。`volatile` 只保证「每次都真的访存」，不提供 happens-before。
而且写方（feed 的 `g_running = 0`）**一把锁都没拿** —— 锁只保护一边是没用的。

修法（源码里用 `-DFIX_RUNNING` 开关，方便你 A/B 对比）：`g_running` 换成
`atomic_int` + `atomic_store/load`，并把退出条件挪进 `g_book_lock` 临界区内判定。

```bash
cc -g -O1 -pthread -fsanitize=thread -DFIX_RUNNING -o c7_1_fixed code/c7_1_trader.c
./c7_1_fixed          # → 退出码 0，stderr 全空
```

**这就是本节「正确基线」这个概念要加的一课**：`g_total == 40100` 只说明
**业务逻辑对**，不说明**没有数据竞争**。这两件事必须用两个不同的手段来验
（前者靠断言，后者靠 TSan）。Q3 的答案因此要补一句：**基线只定义了「值应该对」，
不定义「并发安全」**。

### 坑 1：`-DBUG_*` 差点全都没生效

第一轮验证时我把 `-DBUG_CRASH` 写进了「程序参数」位置而不是「编译参数」位置，
于是**五个变体全都跑了正确版本**，输出整齐划一地都是：

```text
变体： 无（正确版本）
total matched qty = 40100
```

五个「通过」其实是五个「没生效」。**唯一看出问题的线索就是那句自报家门的
`变体： …`** —— 如果没有它，这次会得出完全相反的结论。

> 给调试程序留一句「我是谁、我开了哪些开关」，成本一行 `printf`，
> 收益是防止整批实验静默失效。这和「正确基线」是同一类思路：**先确认你在测的是
> 你以为在测的东西。**

### 坑 2：`usleep` 在 strict POSIX 下被 glibc 藏了

```text
gcc:   warning: implicit declaration of function 'usleep'; did you mean 'sleep'?
clang: error:   call to undeclared function 'usleep'
```

`usleep` 已在 POSIX.1-2008 中被移除。gcc 只给警告（于是按「返回 int」的隐式声明
链接，**能跑**），clang 直接报错。源码改成 `nanosleep` 封装的 `sleep_us()`。
**同一份 C 代码在两个编译器下可能是「警告」vs「致命错误」——CI 里两个都跑一遍很有必要。**

## HFT 关联

1. **真实系统的 bug 是「混装」的**：撮合引擎出问题，往往是「竞态写坏了数据 → 引发崩溃 → 崩溃前的分配泄漏」多类问题叠加。分诊（Ch1）+ 多工具接力（Ch2–Ch6）是唯一正确姿势，别指望一个工具通吃。
2. **这个骨架就是真实撮合引擎的缩影**：feed（行情/下单入口）和 match（撮合）正是 HFT 最核心的两个并发角色，共享订单簿是数据竞争和死锁的高发区。
3. **正确基线是排查的锚**：任何 bug 排查的第一步都是「确定正确行为应该是什么」（这里是 `g_total=40100`），否则连「错没错」都不知道。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 这个程序里两把锁（`g_book_lock` 和 `g_stat_lock`）分别保护什么？为什么要两把而不是一把？

> `g_book_lock` 保护订单簿链表 `g_book`（feed 插入、match 摘除都会改它）；`g_stat_lock` 保护统计量 `g_total`。用两把锁是**细粒度锁**的思路：订单簿操作和统计量更新是两件独立的事，用一把大锁会把无关的临界区也串行化，降低并发度。当然细粒度锁也引入了「锁序」问题（两把锁的获取顺序必须一致，否则可能死锁，这正是 Ch4 的主题）。

**Q2:** 四个雷里，哪个「最阴险、最难发现」？为什么？

> **竞态雷（BUG_RACE）**最难。因为崩溃（雷1）至少会崩、会留 core；泄漏（雷3）会内存增长、valgrind 一查就出；卡住（雷4）会永久停住、strace 一眼看到。唯独竞态：结果「时对时错」（丢了几个更新但大多时候接近正确），程序不崩不卡不涨内存，单次跑可能完全正常，只有高并发压力下偶发。它需要 TSan 这类「专门盯 happens-before」的工具才能稳定暴露，靠肉眼和 gdb 几乎无解。

**Q3:** 为什么「正确基线」是调试的第一步？

> 因为调试的本质是「发现实际行为和预期行为的偏差」。如果不知道正确结果是什么（这里是 `g_total=40100`），你甚至无法判断「结果错了」。正确基线提供了判断锚点和验收标准：排查过程就是「找到为什么实际偏离了基线」，修复后以「回到基线」为验收。这对应 Ch1 方法论的「先定性、后定位」。

**Q4:** 用宏（`-DBUG_xxx`）隔离四类雷，相比「四个独立文件」，有什么好处？

> 好处是**共享同一份正确骨架**：四个雷都基于同一个程序，只有「破坏点」用宏切换，保证你排查时看到的其他部分是完全一致的（变量名、行号、逻辑都不变）。如果用四个文件，容易引入「文件之间除了雷还有其他差异」的干扰，让定位失真。宏隔离 = 控制变量法，是制造「最小可复现」的工程技巧（对应 1.3 最小复现）。

**Q5:** 这个程序为什么用 `while (g_running || g_book)` 作为 match 的循环条件？

> 因为要处理「生产/消费的边界」：`g_running` 表示 feed 还在生产，`g_book` 表示订单簿里还有未撮合的订单。只有当 feed 已停止（`g_running=0`）**且**订单簿已空（`g_book=NULL`）时，match 才能退出。两个条件用 `||`：只要「还在生产」或「还有存量」就得继续撮合。这是典型的生产者-消费者收敛条件。

**Q6:** 基线跑出 `g_total = 40100`，能不能说这个程序「并发上是正确的」？

> 不能，而且实测已经证明不行。把**基线代码**（不打开任何 `-DBUG_*` 宏）交给 TSan，
> 它照样报 1 条 data race —— `g_running` 是 `volatile int`，feed 无锁写它、match 持着
> `g_book_lock` 读它。`volatile` 不是同步原语（同 3.2：`volatile` 不给原子性），
> 而锁只保护一边也等于没保护。
>
> 所以「正确」要拆成两件事分开验：**值对不对**靠断言/基线比对（`== 40100`），
> **有没有数据竞争**靠 TSan。用一个手段验两件事，就是这次差点漏掉它的原因。
> 修法：`g_running` 换成 `atomic_int`，并把退出条件挪进锁内（源码里是 `-DFIX_RUNNING`）。

**Q7:** 为什么给这个 demo 加一句 `printf("变体： …")` 打印自己开了哪些宏？

> 因为它是**唯一能证明「实验真的按预期配置跑了」的证据**。实测教训：第一轮验证时
> `-DBUG_CRASH` 被误放进了「程序参数」而不是「编译参数」，五个变体全都跑成了
> 正确版本、全打印 `变体： 无（正确版本）`。如果输出里没有这一行，看到的会是
> 「五个变体全部通过」—— 结论完全相反。
> 这和 Q3 的「正确基线」是同一类思路：**先确认你在测的是你以为在测的东西。**

</details>

## 交叉引用

- [7.2 崩溃 → coredump 回溯定位](02-crash-coredump.md)
- [7.3 竞态 → TSan 定位](03-race-tsan.md)
- [7.4 泄漏 → valgrind 定位](04-leak-valgrind.md)
- [7.5 卡住 → strace 定位](05-hang-strace.md)
- [Ch7 实战](../README.md)
