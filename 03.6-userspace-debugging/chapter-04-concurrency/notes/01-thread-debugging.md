# 4.1 多线程调试（thread / thread apply all bt / scheduler-locking / 死锁）

> 🔴 精读 · 交易系统调试的核心战场

## 本节要点

单线程程序「run 到断点 → 单步 → 看变量」就够了；多线程程序却有三个新难题：① gdb 默认只盯着**一个线程**，其他线程在干嘛不知道；② 断点命中的线程不确定；③ 数据竞争/死锁这类 bug 只在多线程交错时才浮现，单步一个线程往往复现不了。本节讲清 gdb 的多线程命令族，并用一个「无锁共享订单簿」的竞争示例 + 一个死锁示例，演示怎么定位。

## 先看清：gdb 眼中的线程

```gdb
(gdb) info threads
  Id   Target Id                                  Frame
* 1    Thread 0x7ffff7dca740 (LWP 12345) "orderbook_mt"  producer (...) at orderbook_mt.c:16
  2    Thread 0x7ffff75c9700 (LWP 12346) "orderbook_mt"  consumer (...) at orderbook_mt.c:24
```

| 列 | 含义 |
|----|------|
| `Id` | gdb 内部线程号（`thread N` 用这个） |
| `Target Id` / `LWP` | 内核线程号（light-weight process，`/proc/<pid>/task/<tid>` 里的 TID） |
| `*` | 当前线程（gdb 命令默认作用于它） |
| `Frame` | 该线程当前停在哪 |

```gdb
(gdb) thread 2          # 切换到线程 2
(gdb) bt                # 看线程 2 的调用栈（bt 默认只看当前线程！）
```

> ⚠️ 新手最容易踩的坑：**`bt` 只显示当前线程的栈**。切线程要 `thread N` 再 `bt`，或干脆用下面的一键全览。

## thread apply all bt：全线程栈全景（最常用）

一个命令打印所有线程的调用栈，是定位「某个线程卡住了 / 崩了」的第一动作：

```gdb
(gdb) thread apply all bt

Thread 2 (Thread 0x7ffff75c9700 (LWP 12346)):
#0  0x00007ffff7e0b4a0 in __lll_lock_wait_private () ...
#1  ... in pthread_mutex_lock ()
#2  ... in consumer () at orderbook_mt.c:24

Thread 1 (Thread 0x7ffff7dca740 (LWP 12345)):
#0  producer () at orderbook_mt.c:16
#1  ... in start_thread ()
```

```gdb
(gdb) thread apply 1 2 bt       # 只看 1、2 号线程
(gdb) thread apply all bt full  # 全线程栈 + 每帧局部变量（信息最全）
```

## 多线程下的断点行为

断点命中时，**所有线程都会停在断点处**（默认 `scheduler-locking off`），但 gdb 只切到**触发断点的那个线程**展示。要让断点只对特定线程生效：

```gdb
(gdb) break consumer thread 2   # 断点只在线程 2 命中时停
(gdb) break orderbook_mt.c:24 thread 2
```

> 条件断点里引用线程号要小心：`break ... if ...` 的表达式在当前线程上下文求值，多线程下用 `$_thread`（gdb 内建变量，当前线程号）做过滤。

## scheduler-locking：单步时锁住其他线程

这是**定位数据竞争的关键开关**。默认 `off` 时，你 `step` 一步，其他线程也在跑——于是「复现竞态」时变量状态飘忽不定。锁定后，单步只走当前线程，其他线程冻结，竞态被「放大」成确定性：

```gdb
(gdb) set scheduler-locking step    # 单步/next 时锁住其他线程（最常用）
(gdb) set scheduler-locking on      # 完全锁死其他线程（断点间也不跑）
(gdb) set scheduler-locking off     # 默认，所有线程自由运行
(gdb) set scheduler-locking replay  # rr replay 模式专用（见 4.2）
(gdb) show scheduler-locking
```

| 取值 | 行为 | 适用 |
|------|------|------|
| `off` | 其他线程自由运行 | 默认、普通断点观察 |
| `on` | 其他线程全程冻结 | 极端隔离，看单线程纯逻辑 |
| `step` | 仅 `step`/`next`/`finish` 期间锁其他线程 | **复现数据竞争** ✅ |
| `replay` | rr 可逆调试专用 | 配合 `record` 使用 |

## 数据竞争实战：无锁共享订单簿

下面这个程序，`producer` 线程不停往链表头插订单，`consumer` 线程不停遍历链表——`head` 和 `o->next` **没有任何锁保护**。跑一会儿 `consumer` 遍历到一半，`producer` 把 `head` 改了，`p` 变成野指针 → 段错误：

```c
// orderbook_mt.c —— 多线程订单簿，埋数据竞争
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct order { int id; struct order *next; } order_t;
order_t *head = NULL;              // 共享，无锁 ← 竞争源

void *producer(void *arg) {
    for (int i = 0; ; i++) {
        order_t *o = malloc(sizeof(order_t));
        o->id = i;
        o->next = head;            // 竞争点 1
        head = o;                  // 竞争点 2
    }
    return NULL;
}
void *consumer(void *arg) {
    for (;;)
        for (order_t *p = head; p; p = p->next)  // 竞争点 3：遍历时 head 被改
            if (p->id < 0) printf("corrupt!\n");
    return NULL;
}
int main(void) {
    pthread_t t1, t2;
    pthread_create(&t1, NULL, producer, NULL);
    pthread_create(&t2, NULL, consumer, NULL);
    pthread_join(t1, NULL); pthread_join(t2, NULL);
    return 0;
}
```

```bash
gcc -g -O0 -pthread -o orderbook_mt orderbook_mt.c
./orderbook_mt          # 跑几秒后
# Segmentation fault (core dumped)
```

```gdb
gdb ./orderbook_mt core
(gdb) thread apply all bt
Thread 2 (... LWP ...):
#0  consumer (...) at orderbook_mt.c:24     # ← 崩溃线程是 consumer
#1  ... in start_thread ()
Thread 1 (... LWP ...):
#0  producer (...) at orderbook_mt.c:16     # ← producer 还在拼命插单
(gdb) thread 2
(gdb) frame 0
(gdb) print p
$1 = (order_t *) 0x7ffff0001234            # ← p 是野指针，指向已 free 或乱码
(gdb) x/2gx p
0x7ffff0001234: 0x0000000000000000 0x0000000000000000   # 内容已被破坏
```

结论一目了然：`consumer` 在第 24 行遍历时，`p` 已被 `producer` 并发改坏。修复方向 = 给链表加锁（`pthread_mutex`）或改无锁结构（RCU / 不可变节点），不是 gdb 的锅。

## 死锁定位：两个线程卡在各自的锁上

死锁是另一种高频多线程 bug——程序不崩，但**所有线程都不动了**。典型场景：线程 A 先拿锁 1 再拿锁 2，线程 B 先拿锁 2 再拿锁 1，锁序相反 → 互相等待：

```gdb
(gdb) thread apply all bt

Thread 2 (Thread ...):
#0  __lll_lock_wait_private () from libc
#1  pthread_mutex_lock ()
#2  worker_B () at deadlock.c:42        # ← B 卡在拿 lock2
#3  start_thread ()

Thread 1 (Thread ...):
#0  __lll_lock_wait_private () from libc
#1  pthread_mutex_lock ()
#2  worker_A () at deadlock.c:18        # ← A 卡在拿 lock1
#3  start_thread ()
```

两个线程**都卡在 `pthread_mutex_lock`**，谁都不往前走 → 死锁实锤。进一步看锁序：

```gdb
(gdb) thread 1
(gdb) frame 2
(gdb) info locals
lock = &lock1      # ← A 在等 lock1，但它其实已经持有了 lock2
(gdb) thread 2
(gdb) frame 2
(gdb) info locals
lock = &lock2      # ← B 在等 lock2，但它已经持有了 lock1
```

锁序相反（A: lock2→lock1，B: lock1→lock2）就是根因。修法：统一加锁顺序（都先 lock1 再 lock2），或用 `pthread_mutex_trylock` + 超时回退。

## 动手：不装 gdb，也把「死锁」证出来（实测）

上面那些 gdb 命令都需要 gdb。这里换一条路：**让程序自己报警**。

`code/c4_2_deadlock.c` 里两个线程反序加锁（A: `lock_a`→`lock_b`，B: `lock_b`→`lock_a`），
另有一个看门狗线程，每 200ms 采样一次两个工作线程的「心跳」（**只有真正拿到两把锁才 +1**）
和「当前阶段」。

```bash
cc -g -O1 -pthread -Wall -Wextra -o c4_2_deadlock code/c4_2_deadlock.c
./c4_2_deadlock abba        # 反序加锁 → 死锁
./c4_2_deadlock ordered     # 统一锁序 → 正常跑完
```

为什么这个 AB-BA 是**确定性**的：教科书写法跑十次有三次顺利跑完（谁先拿到第一把锁
看调度运气）。这里加了一次握手——两个线程各自先拿到自己的第一把锁、用原子变量通报
「我拿到了」、并**等对方也通报完**，才一起去抢第二把锁。于是环**必然**形成。

### `abba` 模式实测输出（gcc 13.3.0）

```text
[watchdog 第 0轮] A: hb=0     抢第二把锁(←死锁卡点) | B: hb=0     抢第二把锁(←死锁卡点)
[watchdog 第 1轮] A: hb=0     抢第二把锁(←死锁卡点) | B: hb=0     抢第二把锁(←死锁卡点)
[watchdog 第 2轮] A: hb=0     抢第二把锁(←死锁卡点) | B: hb=0     抢第二把锁(←死锁卡点)
[watchdog 第 3轮] A: hb=0     抢第二把锁(←死锁卡点) | B: hb=0     抢第二把锁(←死锁卡点)

=========================== 看门狗判定：死锁 ===========================
第 1 轮采样起，连续 3 次采样两个线程心跳都不变 → 没有任何线程在推进

[1] 两个线程各自停在哪（心跳 = 只有真正拿到两把锁才 +1）
    thread-A  hb=0      阶段=抢第二把锁(←死锁卡点)
    thread-B  hb=0      阶段=抢第二把锁(←死锁卡点)
    ↑ 两个心跳都停在同一个数字上不动了，阶段都停在「抢第二把锁」

[2] 独立取证：两把锁到底在谁手里（看门狗亲自去试）
    pthread_mutex_timedlock(&lock_a) → 被占用（timedlock 超时）
    pthread_mutex_timedlock(&lock_b) → 被占用（timedlock 超时）
    ↑ 两把锁同时被占，且持有者毫无进展 → 环形等待成立

[3] 还原锁序
    thread-A: 拿到 lock_a → 在等 lock_b
    thread-B: 拿到 lock_b → 在等 lock_a
    → A: a→b，B: b→a，两个方向相反，环闭合。这就是 AB-BA 死锁。
```

（退出码 **3** —— 是看门狗主动判定后自己退的，不是崩溃。这和 139/134 那类
「被信号打死」有本质区别：看到 3 就该想到「有人做了判断」。）

### 对照：同一份程序，锁序一统一就不死了

```text
[watchdog 第 0轮] A: hb=1     持有两把锁        | B: hb=171   持有两把锁
[watchdog 第 1轮] A: hb=171   持有两把锁        | B: hb=171   抢第一把锁
[watchdog 第 2轮] A: hb=338   持有两把锁        | B: hb=171   抢第一把锁
[watchdog 第 3轮] A: hb=392   抢第一把锁        | B: hb=286   持有两把锁
[watchdog 第 4轮] A: hb=400   已完成            | B: hb=400   已完成

[watchdog] 两个线程都已 ST_DONE，心跳从 0 涨到 400 —— 全程有推进，无死锁。
```

三件事值得记住：

1. **「没有进展」是可测量的，不用靠感觉**。心跳只在真正拿到两把锁之后 +1，
   所以心跳不动 = 没有线程在干活。这和「慢」有本质区别：慢的程序心跳还在涨。
   「你怎么区分死锁和慢」——这是最直接的答案。
2. **看门狗那两行 `timedlock` 是独立取证**。`ETIMEDOUT` 说明锁被别人占着；
   两把都超时、且持有者都不动，环就闭合了。这比「我觉得它卡住了」硬得多。
   这正是 gdb 做不到的事——gdb 要你**先**怀疑是死锁、**再**去 attach；
   而看门狗是**常驻**的，卡住那一刻自己就把证据留下了。
3. **第 0 轮的数字很不对称，别以为有 bug**：A 才 1 格、B 已经 171 格。两个原因——
   ①采样是**时间点快照**，打印 A 和打印 B 之间已经过了几微秒，不是同步事务；
   ②glibc 的普通互斥锁**不保证公平**（会 barging），同一线程可以连续赢很多次。

### gdb 那张 `thread apply all bt` 表，就是这件事的「带栈版本」

本 demo 只能打印「阶段字符串」，因为 `backtrace()` 拿不到**别的线程**的栈。
gdb 能读到 `.symtab`，所以能把同一份信息升级成函数名 + 行号：

```gdb
(gdb) thread apply all bt
Thread 1:  #0 __lll_lock_wait_private   #1 pthread_mutex_lock
           #2 worker (... &lock_b) at c4_2_deadlock.c:<② 那一行>
Thread 2:  #0 __lll_lock_wait_private   #1 pthread_mutex_lock
           #2 worker (... &lock_a) at c4_2_deadlock.c:<② 那一行>
```

两份信息一一对应：

| 本 demo 看到的 | gdb 看到的 |
|----------------|------------|
| 阶段 = 抢第二把锁（←死锁卡点） | `#2 worker (...)` 停在②那一行的 `pthread_mutex_lock` |
| 两把锁都被占（timedlock 超时） | 两个线程的 `#0/#1` 都是 `__lll_lock_wait_private` / `pthread_mutex_lock` |
| 心跳不涨 | 谁的栈都不再前进 |

**先把 demo 跑一遍，再去看 gdb 输出，那张表就不是天书了。**

## HFT 关联

1. **崩溃/卡死第一命令 = `thread apply all bt`**：交易进程几十个线程，一眼看出「崩的是行情线程还是下单线程」「卡住的是网络线程还是风控线程」，比逐个 `thread N; bt` 高效一个量级。
2. **`scheduler-locking step` 复现竞态**：偶发错单往往是竞态，单步时锁住其他线程，把「千次一现」的竞争变成「每次必现」的确定性复现，是定位的胜负手。
3. **死锁 = 全线程卡 `mutex_lock`**：`thread apply all bt` 里所有线程栈顶都是 `pthread_mutex_lock`，立刻判定死锁，再逐帧 `info locals` 看各自持有的锁，还原锁序。
4. **thread-specific breakpoint 盯单线程**：只给「下单线程」打断点，不被行情线程的海量命中淹没。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么 `bt` 在多线程程序里「看错了栈」？正确姿势是什么？

> `bt` 只打印**当前线程**（`info threads` 里带 `*` 的那个）的调用栈。多线程下你以为在看崩溃线程，其实看的可能是别的线程。正确姿势：要么 `thread N` 切到目标线程再 `bt`，要么直接 `thread apply all bt` 一次看全。

**Q2:** `scheduler-locking` 的 `on` 和 `step` 区别？定位竞态用哪个？

> `on` = 其他线程全程冻结（断点之间也不跑）；`step` = 只在 `step`/`next`/`finish` 单步期间冻结，断点间仍自由跑。定位竞态用 `step`——它让你单步时「其他线程不捣乱」，又保留断点间正常的并发调度，最接近真实交错。

**Q3:** 断点默认行为在多线程下有什么「坑」？

> 任何一个线程跑到断点地址都会触发暂停，且**所有线程**都会停在断点处，但 gdb 只切到触发它的线程。调试时你不知道「这次是谁触发的」。要定向，用 `break ... thread N` 把断点绑定到特定线程，或条件断点里用 `$_thread` 过滤。

**Q4:** 死锁程序为什么 `thread apply all bt` 一看就知道？

> 死锁的签名是「所有（或一组）线程的栈顶都卡在 `pthread_mutex_lock`/`__lll_lock_wait_private`，且谁的栈都不再前进」。对比正常阻塞（如 `recv` 等网络数据）栈顶是别的 syscall，一眼能区分「死锁」还是「在等 IO」。

**Q5:** 段错误时 `thread apply all bt` 里 `#0` 帧是崩溃线程，但根因可能在别的线程，为什么？

> 崩溃线程只是「踩到了坏数据」的受害者，坏数据往往是**另一个线程**在并发写入时留下的（如本例 producer 改坏 head，consumer 崩）。所以定位多线程段错误不能只看 `#0`，要结合其他线程的栈判断「谁在并发改这块内存」。

</details>

## 交叉引用

- [5.4 attach 运行中进程](../../chapter-05-behavior/notes/04-attach-running-process.md)
- [4.2 rr 可逆调试](02-rr-reversible-debugging.md)
- [2.3 栈帧与回溯](../../chapter-02-crash/notes/03-stack-backtrace.md)
- [03.6 模块导读](../../README.md)
