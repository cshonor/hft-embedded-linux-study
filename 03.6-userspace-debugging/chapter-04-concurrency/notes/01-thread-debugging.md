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
