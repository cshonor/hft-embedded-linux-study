# 7.5 卡住 → strace 定位

> 🔴 精读 · 程序「永久卡死」，strace 一眼看出卡在等锁还是等 IO

## 本节要点

用 7.1 的 `trader.c` 开 `BUG_HANG` 雷：feed 线程**同一线程重复锁 `g_book_lock`**（非递归锁），第二次锁自己已持有的锁 → 自死锁，程序永久卡住。本节走一遍「观察卡住 → strace 定位阻塞点 → 区分等锁 vs 等 IO → 修复」的流程，核心工具是 Ch5 的 strace。重点体会：**卡住时先判断「卡在哪类操作上」，排查方向截然不同**。

## 第一步：观察「永久卡住」

```bash
gcc -g -O0 -pthread -o trader_hang -DBUG_HANG trader.c
./trader_hang
# （光标闪烁，什么都不输出，程序卡死不动）
```

程序没有输出 `total matched qty = ...`，也没有崩溃，就是**一动不动**。按 Ctrl-C 杀掉。

> 关键判断：卡住 ≠ 崩溃。崩溃会留 core，卡住是「活着但不干活」——线程停在某些阻塞操作上（等锁、等 IO、死循环）。定位卡住要用「看它现在停在哪」的工具，strace 是首选。

## 第二步：strace attach 看卡在哪

```bash
./trader_hang &
PID=$!
sleep 1
strace -p $PID -f
```

`-f` 跟踪所有线程，`-p` attach 到运行中进程：

```text
strace: Process 12345 attached
[pid 12346] futex(0x404080, FUTEX_WAIT_PRIVATE, 2, NULL <unfinished ...>
[pid 12345] futex(0x404080, FUTEX_WAIT_PRIVATE, 2, NULL <unfinished ...>
```

两个线程（`12345` 主线程、`12346` feed 线程）都卡在 **`futex(...FUTEX_WAIT_PRIVATE...)`** 上。

> `futex` 是 Linux 用户态锁（mutex）的底层实现：`FUTEX_WAIT` = 线程在**等一把锁**。两个线程都等锁 → 卡死，且是「等锁」而非「等 IO」。

对比 5.2 讲的两种卡住：

| 卡在 | 系统调用 | 含义 | 排查方向 |
|------|----------|------|----------|
| **等锁** | `futex(...FUTEX_WAIT...)` | 线程在等 mutex | 查锁序 / 死锁 / 忘解锁（Ch4） |
| **等 IO** | `recvfrom` / `read` / `poll` | 线程在等网络/文件数据 | 查对端有没有发数据 |

这里两个线程都 `futex` 等锁 → **锁问题**，不是网络问题。

## 第三步：用 gdb 看「谁持着锁、谁在等」

strace 告诉我们「卡在等锁」，但「谁持着锁不放」要看 gdb（4.1 讲过）：

```bash
gdb -p $PID
(gdb) thread apply all bt
```

```text
Thread 2 (Thread 0x7f... (LWP 12346) "trader_hang"):
#0  futex_wait (...)                      ← 在等锁
#1  __pthread_mutex_lock_full (...)
#2  feed_thread (arg=0x0) at trader.c:32   ← feed 第二次 lock g_book_lock 卡住
#3  start_thread ...

Thread 1 (Thread 0x7f... (LWP 12345) "trader_hang"):
#0  pthread_join (...)                    ← 主线程在等线程结束
#1  main (argc=1, argv=...) at trader.c:77
```

解读：

- **feed 线程**卡在 `trader.c:32` 的 `pthread_mutex_lock` —— 也就是 `BUG_HANG` 里的**第二次 lock**。
- **主线程**卡在 `pthread_join` —— 在等 feed 线程结束，而 feed 永远结束不了。

回看 feed 的代码：

```c
#ifdef BUG_HANG
        pthread_mutex_lock(&g_book_lock);    // 第一次锁，成功（自己持有）
        pthread_mutex_lock(&g_book_lock);    // 雷4：第二次锁 → 自死锁！
        ...
#endif
```

**根因**：`pthread_mutex_lock` 默认是**非递归锁**——同一线程已经持有 `g_book_lock`，再 lock 一次，会**自己等自己**，永久阻塞。这就是「自死锁」（self-deadlock）。

## 第四步：修复

```c
// 修复 1：删掉多余的第二次 lock（根本修法）
pthread_mutex_lock(&g_book_lock);
o->next = g_book;
g_book = o;
pthread_mutex_unlock(&g_book_lock);

// 修复 2：若确实需要可重入，用递归锁初始化
pthread_mutex_t g_book_lock;
pthread_mutexattr_t attr;
pthread_mutexattr_init(&attr);
pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);  // 递归锁
pthread_mutex_init(&g_book_lock, &attr);
```

> 但**递归锁是「掩盖问题」不是「解决问题」**：递归锁让你「重复锁不崩」，但往往掩盖了「代码逻辑里不该重复锁」的设计缺陷。绝大多数情况应该用修复 1——理清锁的使用范围，删掉多余 lock。

## 区分「自死锁」和「ABBA 死锁」

本例是**自死锁**（单线程自己等自己）。4.1 里讲的经典死锁是 **ABBA 循环等待**（两线程互相等）：

```
自死锁（本例）：            ABBA 死锁（4.1）：
  线程 A 持锁 L，再锁 L     线程 A 持 L1 等 L2
  → A 等 A 自己            线程 B 持 L2 等 L1
  → 永久卡                 → A 等 B、B 等 A → 循环等待
```

两者 strace 表现一样（都是 `futex FUTEX_WAIT`），gdb `thread apply all bt` 能区分：自死锁是**一个线程**在「自己已持有的锁」上等待；ABBA 是**两个线程**各自持一把锁、等对方那把。

## 为什么「卡住」要用 strace 而不是 gdb

| 工具 | 卡住场景的优势 | 局限 |
|------|----------------|------|
| **strace** | attach 就能看「所有线程停在哪」，`futex` vs `recvfrom` 一眼分类 | 看不到「谁持着锁」 |
| **gdb** | `thread apply all bt` 看完整调用栈 + 锁的持有关系 | 会暂停程序，可能改变时序 |

**标准流程**：先 strace 定性「等锁还是等 IO」→ 若是等锁，gdb 深入看「谁持锁谁等锁」→ 定位死锁/忘解锁。两者接力，不是二选一。

## HFT 关联

1. **「卡死」是交易系统的致命故障**：撮合引擎卡住 = 停止成交 = 直接经济损失。卡住比崩溃更隐蔽（不报错、不留 core、监控可能只看到「无输出」），所以 strace attach 是「卡住现场」的第一工具。
2. **`futex` vs `recvfrom` 是分水岭**：卡住时先 strace 看停在哪——等锁（futex）查锁序/死锁，等 IO（recvfrom）查对端/网络。方向错了排查就是白费（5.2 的核心经验）。
3. **非递归锁是默认、也是陷阱**：`PTHREAD_MUTEX_INITIALIZER` 初始化的默认是非递归锁，重复 lock 即自死锁。HFT 代码里「一个函数内 lock，又调用另一个也会 lock 的函数」是自死锁的常见来源，要警惕锁的重入。
4. **锁的持有范围要最小化**：锁内不要 sleep、不要调用可能再锁的函数（本例 feed 锁内本不该有第二次 lock）。锁越短，死锁和延迟毛刺越少。

```bash
# HFT 场景：撮合引擎卡住，第一时间 attach 看所有线程
strace -p $(pidof matching_engine) -f -e trace=futex,recvfrom,read,poll
# 只看 futex（锁）和 recvfrom/read/poll（IO）这几类，快速定性
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么 `pthread_mutex_lock` 对同一把锁 lock 两次会永久卡住？

> 因为默认的 `pthread_mutex_t`（`PTHREAD_MUTEX_INITIALIZER`）是**非递归锁**：它记录「当前是否被持有」，一旦某线程持有，任何 lock 请求（包括持有者自己）都必须等待锁被释放。而「释放」要靠持有者 unlock——持有者却卡在第二次 lock 上，永远等不到自己 unlock，于是**自己等自己**，永久阻塞。这就是自死锁。

**Q2:** 卡住时，strace 看到 `futex FUTEX_WAIT` 和看到 `recvfrom`，排查方向有什么不同？

> `futex FUTEX_WAIT` 表示线程在**等锁**（mutex 的底层实现），该查锁序/死锁/忘解锁（Ch4 并发类）。`recvfrom`/`read`/`poll` 表示线程在**等 IO 数据**，该查对端有没有发数据、网络/文件是否就绪（Ch5 行为类）。两者根因完全不同：一个是「线程间同步问题」，一个是「外部数据问题」。方向错了，排查就是白费力气。

**Q3:** 递归锁能「解决」这个自死锁吗？为什么不推荐？

> 递归锁让同一线程重复 lock 不阻塞（内部计数，lock 两次要 unlock 两次才真正释放），确实「不卡了」。但它是**掩盖问题**而非解决问题：重复 lock 往往暴露「锁的使用范围没理清」（一个函数 lock 后又调用另一个也 lock 的函数），递归锁让这个设计缺陷「合法化」，埋下更难发现的隐患（如 lock/unlock 次数不匹配）。正确做法是理清锁范围、删掉多余 lock。

**Q4:** 自死锁和 ABBA 死锁在 strace/gdb 表现上怎么区分？

> strace 表现一样（都是 `futex FUTEX_WAIT` 等锁），要 gdb 区分：自死锁是**单个线程**在「自己已持有的锁」上等待（`thread apply all bt` 里该线程栈显示第二次 lock 同一把锁）；ABBA 死锁是**两个线程**各自持一把锁、互相等对方那把（两个线程栈分别显示「持 L1 等 L2」和「持 L2 等 L1」）。看「等锁的线程数量和锁的关系」即可区分。

**Q5:** 为什么卡住场景「先 strace 定性、再 gdb 深入」？

> 因为 strace attach 就能无侵入地看到「所有线程当前停在哪」，且 `futex` vs `recvfrom` 能**立刻分类**（等锁 vs 等 IO），决定后续排查方向。gdb 虽然能看完整调用栈和锁持有关系，但会暂停程序、可能改变时序，且不知道方向时上来就 gdb 效率低。所以标准流程是：strace 定性 → 若是等锁，gdb 深入看持锁/等锁关系。两者接力，不是二选一。

</details>

## 交叉引用

- [7.4 泄漏 → valgrind 定位](04-leak-valgrind.md)
- [5.1 strace 入门](../../chapter-05-behavior/notes/01-strace-basics.md)
- [5.2 strace 实战分析](../../chapter-05-behavior/notes/02-strace-practical-analysis.md)
- [4.1 多线程调试（死锁签名）](../../chapter-04-concurrency/notes/01-thread-debugging.md)
- [Ch7 实战](../README.md)
