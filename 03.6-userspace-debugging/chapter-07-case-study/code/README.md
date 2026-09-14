# Ch7 实战 · 可跑的 demo

`c7_1_trader.c` —— 一个程序埋四类雷，把 Ch2–Ch6 的工具串起来。

架构与笔记 7.1 的图一致：`feed` 线程 `malloc` 订单塞进共享订单簿，`match` 线程摘头部
订单、累加 `g_total`、`free`。**同一份骨架，只换破坏点**（控制变量法，见 1.3）：

| 变体 | 宏 | 破坏点 | 期望症状 |
|------|----|--------|----------|
| 基线 | 无 | — | `g_total = 40100`，干净退出 |
| 崩溃 | `-DBUG_CRASH` | feed：`slots[o->id]` 越界写栈（`id` 最大 200，数组只有 16） | 段错误 / 栈金丝雀报警 |
| 竞态 | `-DBUG_RACE` | feed + match 无锁累加 `g_total` | 结果错，TSan 报错 |
| 泄漏 | `-DBUG_LEAK` | match：摘除订单后不 `free` | 内存只增不减 |
| 卡住 | `-DBUG_HANG` | feed：同线程重复锁非递归锁 → 自死锁 | 永久卡死 |
| 修复 | `-DFIX_RUNNING` | 把 `g_running` 从 `volatile` 换成 C11 原子 | 让基线也过 TSan |

---

## 九种跑法的实测结果汇总（gcc 13.3.0 / clang 18.1.0）

| 跑法 | 关键输出 | 退出码 |
|------|----------|--------|
| 基线 | `total matched qty = 40100`；malloc 200 / free 200 / 仍存活 0 | **0** |
| 基线 + TSan | `total = 40100`，但 TSan 报 **1 条**：`g_running` 竞争 | **66** |
| 基线 + TSan + `-DFIX_RUNNING` | `total = 40100`，TSan **静默** | **0** |
| 基线 + ASan | `total = 40100`，**无泄漏报告** | **0** |
| `-DBUG_CRASH`（无栈保护） | stdout 只剩表头；stderr `Program terminated with signal SIGSEGV (11)` | **139** |
| `-DBUG_CRASH` + `-fstack-protector-all` | stdout 只剩表头；stderr `*** stack smashing detected ***: terminated` | **134** |
| `-DBUG_RACE` + TSan | `total matched qty = 80200`（= 2 × 40100）；TSan 报 **3 条** | **66** |
| `-DBUG_LEAK` + ASan | stdout 只剩表头；LSan 报 `6400 byte(s) leaked in 200 allocation(s)` | **1** |
| `-DBUG_HANG` | stdout 只剩表头；看门狗 alarm 打印现场 | **4** |

**这张表就是 Ch7 的全部内容。** 下面逐行给出完整输出与解读。

```bash
# 全部编译命令
cc -g -O0 -pthread -Wall -Wextra                        -o c7_1_base  c7_1_trader.c
cc -g -O0 -pthread -Wall -Wextra -fstack-protector-all -DBUG_CRASH -o c7_1_crash c7_1_trader.c
cc -g -O1 -pthread -fsanitize=thread  -DBUG_RACE   -o c7_1_race c7_1_trader.c
cc -g -O1 -pthread -fsanitize=address -DBUG_LEAK   -o c7_1_leak c7_1_trader.c
cc -g -O0 -pthread -Wall -Wextra -DBUG_HANG        -o c7_1_hang c7_1_trader.c
cc -g -O1 -pthread -fsanitize=thread               -o c7_1_tsan c7_1_trader.c
cc -g -O1 -pthread -fsanitize=thread -DFIX_RUNNING -o c7_1_fixed c7_1_trader.c
```

---

## 1. 基线：先把「正确」钉死

```text
迷你下单引擎：feed 塞 200 单，match 撮合；正确基线 = 40100
变体： 无（正确版本）

total matched qty = 40100   （期望 40100）
订单计数：malloc 200 个，free 200 个，仍存活 0 个（每个 32 字节，约 0 字节）
进程进度：feed 到 200/200，match 走了 356 轮
```

`g_total = 100+1 + 100+2 + … + 100+200 = 40100`。**这是后面每一行的锚**——
没有它，你连「结果错了」都判断不了（1.2 决策树的第一步）。

`match 走了 356 轮` > 200：因为 match 要空转几轮才确认「feed 已收工 + 订单簿已空」。

---

## 2. ⚠️ 最反直觉的一条：「正确版本」本身就过不了 TSan

这是本次改写的最大发现，值得单独放在最前面讲。

```bash
./c7_1_tsan            # 基线代码 + TSan
```

```text
total matched qty = 40100   （期望 40100）
--- stderr ---
==================
WARNING: ThreadSanitizer: data race (pid=2)
  Write of size 4 at 0x555555670b78 by thread T1:
    #0 feed_thread /app/example.c:197:5          ← RUNNING_SET(0)

  Previous read of size 4 at 0x555555670b78 by thread T2 (mutexes: write M0):
    #0 match_thread /app/example.c:211:28        ← !RUNNING_GET()

  As if synchronized via sleep:
    #0 nanosleep ...
    #1 sleep_us /app/example.c:90:5
    #2 feed_thread /app/example.c:195:9

  Location is global 'g_running' of size 4 at 0x555555670b78
  ...
SUMMARY: ThreadSanitizer: data race /app/example.c:197:5 in feed_thread
==================
ThreadSanitizer: reported 1 warnings
（退出码 66）
```

**结果 40100 完全正确，进程也不崩不卡，但 TSan 说它有数据竞争。** 三处细节特别有教学价值：

1. **读方是持着锁的**：`by thread T2 (mutexes: write M0)` —— match 读 `g_running` 时
   正经拿着 `g_book_lock`。
2. **写方一把锁都没拿**：`feed_thread:197` 就是 `g_running = 0`，它不在任何临界区里。
   所以**锁只保护一边是没用的** —— 竞争要成立只需要「至少一方写、且双方无 happens-before」。
3. **TSan 还点出了「侥幸同步」**：`As if synchronized via sleep` ——
   它发现这次运行之所以看起来顺序正常，纯粹是因为 `nanosleep` 恰好把两个访问隔开了。
   **靠睡眠对齐时序不是同步，只是运气。**

根因：`volatile int g_running` **不是同步原语**（和 3.2 里 `volatile` 不给原子性是同一条）。
`volatile` 只保证「每次都真的访存」，不提供 happens-before。

### 修好它：`-DFIX_RUNNING`

```bash
./c7_1_fixed          # = -fsanitize=thread -DFIX_RUNNING
```

```text
total matched qty = 40100   （期望 40100）
订单计数：malloc 200 个，free 200 个，仍存活 0 个（每个 32 字节，约 0 字节）
--- stderr ---
（空）
（退出码 0）
```

改动两处：`g_running` 换成 `atomic_int` + `atomic_store/load`；退出条件挪进
`g_book_lock` 临界区内判定（顺带消掉「在 `while` 条件里无锁读 `g_book`」这个隐藏竞争）。

顺带一提：基线 + ASan 也是干净的（`exit 0`，无泄漏报告）—— 基线**只**有并发问题，
没有内存问题。这正是「多工具各查一类」的实证：**换一个工具，结论就换一个面**。

---

## 3. 崩溃：`-DBUG_CRASH`

feed 里 `int slots[16]; slots[o->id] = 1;`，而 `id` 从 1 涨到 200 →
最远写出 `200 × 4 = 800` 字节，踩穿 `feed_thread` 的栈帧。

### 无栈保护（`exit 139`）

```text
变体： CRASH
--- stderr ---
Program terminated with signal SIGSEGV (11)
```

### 带栈金丝雀（`-fstack-protector-all`，`exit 134`）

```text
变体： CRASH
--- stderr ---
*** stack smashing detected ***: terminated
Program terminated with signal SIGABRT (6)
```

**同一个 bug，两种退出码**：139 = SIGSEGV（11），134 = SIGABRT（6）。金丝雀先发现
「栈被写坏了」，于是主动 `abort()` —— 于是你拿到了**准确的死亡时刻**，而不是
在几百行之后某处莫名其妙地崩。

### 注意 stdout 只剩两行

两种情况下 stdout 都**只有**开头那两行（引擎信息 + 变体名），最后的
`total matched qty`、订单计数全都不见了。原因就是 Ch5 讲的 **stdout 全缓冲 +
进程被信号杀死 = 缓冲区内容全丢**；开头两行能活下来，是因为源码里紧跟其后
调了 `fflush(stdout)`。

**这是本 demo 里「为什么必须 fflush」的第二次实证**（Ch1 的 `c1_2_shrink_demo.c`
是第一次）。

---

## 4. 竞态：`-DBUG_RACE`

```bash
./c7_1_race
# total matched qty = 80200   （期望 40100）   退出码 66
```

**结果是 80200 = 2 × 40100** —— feed 和 match **各自**把全部订单加了一遍。
注意这个变体的后果是「稳定多算一倍」，不是「偶尔差一点」；真正难抓的是
4.3 的 `c4_1` 那种**丢更新**（结果只差万分之几）。但 TSan 对两种都当场报出。

TSan 一共报了 **3 条**，逐条都很值得看：

### 第 1 条：`g_total` 本身

```text
  Write of size 8 at 0x5555569dc6b0 by thread T2 (mutexes: write M0):
    #0 match_thread /app/example.c:216:21          ← g_total += o->qty

  Previous write of size 8 at 0x5555569dc6b0 by thread T1:
    #0 feed_thread /app/example.c:191:17           ← g_total += o->qty

  Location is global 'g_total' of size 8 at 0x5555569dc6b0
SUMMARY: ThreadSanitizer: data race /app/example.c:216:21 in match_thread
```

两个线程在同一行语义上写同一个全局变量，**都没拿 `g_stat_lock`**。
（注意 TSan 写的是 `(mutexes: write M0)` —— match 这时拿着 `g_book_lock`，
但 `g_book_lock` 保护的是订单簿，不是统计量。**拿错锁 = 没拿锁**。）

### 第 2 条：`free` 与「读已释放内存」

```text
  Write of size 8 at 0x720800001030 by thread T2 (mutexes: write M0):
    #0 free ... tsan_interceptors_posix.cpp:724:3
    #1 match_thread /app/example.c:227:13          ← free(o)

  Previous read of size 8 at 0x720800001030 by thread T1:
    #0 feed_thread /app/example.c:191:23           ← 读 o->qty
SUMMARY: ThreadSanitizer: data race /app/example.c:227:13 in match_thread
```

**这是一条真实的 use-after-free**，而且是被 `-DBUG_RACE` 顺带引入的：
雷点是「feed 无锁写 `g_total`」，但写的位置在 `pthread_mutex_unlock(&g_book_lock)`
**之后**，于是 feed 读 `o->qty` 的这一刻，这个订单可能已经被 match 撮合掉并
`free` 了。

**教训：破坏点放错位置会连坐出第二个 bug。** 真实的代码审查里，
「这一行到底在锁内还是锁外」就是靠这种报告揪出来的。

### 第 3 条：和基线同款的 `g_running`

```text
  Read of size 4 at 0x555555670b78 by thread T2 (mutexes: write M0):
    #0 match_thread /app/example.c:211:28
  Previous write of size 4 at 0x555555670b78 by thread T1:
    #0 feed_thread /app/example.c:197:5
SUMMARY: ThreadSanitizer: data race /app/example.c:211:28 in match_thread
```

`-DBUG_RACE` 只是把另外两条也一起暴露出来了。**同一个程序，TSan 报 3 条，
但不代表有 3 个独立 bug** —— 第 3 条是骨架本身就有的（见上面第 2 节）。

---

## 5. 泄漏：`-DBUG_LEAK`

```bash
./c7_1_leak           # ASan（含 LeakSanitizer）
```

```text
变体： LEAK
--- stderr ---

=================================================================
==2==ERROR: LeakSanitizer: detected memory leaks

Direct leak of 6400 byte(s) in 200 object(s) allocated from:
    #0 0x7554f1d4804f in malloc (/opt/compiler-explorer/gcc-13.3.0/lib64/libasan.so.8+0xdc04f)
    #1 0x4017b4 in feed_thread /app/example.c:160

SUMMARY: AddressSanitizer: 6400 byte(s) leaked in 200 allocation(s).
（退出码 1）
```

三个细节：

1. **`6400 = 200 × 32 = sizeof(order_t)`**，和程序自己数的 `malloc 200 / free 0`
   完全对上。**所以没有 valgrind 时，「自己数分配次数 + 打印 `sizeof`」是唯一可用的
   降级手段**——只是没有调用栈，定位不到「哪一行泄漏」。
2. **Direct / Indirect 是「退出那一刻还有没有指针指进这条链」，会随写法变**：
   本次实测 `Direct leak … 200 object(s)`（无 Indirect）；上一版 match 循环里保留了
   一个局部指针，跑出来是 `6368 Direct(199) + 32 Indirect(1)`。**两种都对，
   别把分类当正确性指标**——要看的是总量账 `6400 / 200 块`。
3. 报告里的 `#1 feed_thread /app/example.c:160` **就是 malloc 那一行**，
   直接指到案发现场。

**注意这里不能用「结果对不对」发现问题**：`g_total` 依然是 40100（撮合逻辑没错），
退出码却是 1 —— 那 1 完全来自 LSan。**如果没开 sanitizer，这个 bug 的表现只是
「内存曲线一直往上爬」，跑几分钟到几小时才 OOM。**

而且 stdout 又只剩两行：LSan 在退出前 `_exit`，**stdio 缓冲区里的
`total matched qty = 40100` 直接丢了**。同一个「全缓冲」坑，这是第三次出现。

---

## 6. 卡住：`-DBUG_HANG`

```text
变体： HANG
--- stderr ---

[看门狗] alarm 已经 3 秒 —— 进程没结束，判定卡住。
  feed  线程：进度 0/200（★仍在跑★）
  match 线程：进度 0（★仍在跑★）
  g_book_lock: EBUSY —— 锁被持有，但两个线程都不再推进
               → 持有者自己在等自己，自死锁实锤
  （7.5 里 strace 会给同一件事的另一种视角：所有线程都停在 futex 上等锁）
  → 看门狗 _exit(4)（不是崩溃，是有人主动判定它死了）
（退出码 4）
```

雷点是 feed 里连着两次 `pthread_mutex_lock(&g_book_lock)`：非递归锁，
第二次锁自己 → 自死锁。之后 `match` 想拿同一把锁，也跟着卡住。

**为什么这个 demo 比笔记版本多一个看门狗**：容器里没有 `timeout` 命令，卡死的进程
没法收场。所以 `main` 里 `alarm(3)`，超时由 SIGALRM 处理函数打印现场后 `_exit(4)`。
正常基线跑 200 轮 × 1ms，不到 1 秒就结束，看门狗根本不会触发。

看门狗做的三件事，对应 7.5 里 strace 的三个观察点：

| 看门狗 | strace / gdb 的等价视角 |
|--------|------------------------|
| `进度 0/200`（心跳不再涨） | 没有新 syscall 输出 —— 程序不再往前走了 |
| `g_book_lock: EBUSY` | `futex(0x…, FUTEX_WAIT, …)` 卡住 —— 所有人都在等锁 |
| `feed 和 match 都卡住` | `thread apply all bt` 两个线程栈顶都是 `pthread_mutex_lock` |

**退出码 4 是「有人主动判定它死了」，和 139/134/136 那种「被信号打死」有本质区别。**

---

## 踩坑记录（含两处我自己写错的）

### 坑 1：`-DBUG_*` 写进了「程序参数」而不是「编译参数」

第一轮验证时，编译参数一栏是空的、`-DBUG_CRASH` 被当成了命令行参数传给程序 ——
于是**五个变体全都跑了正确版本**（都打印 `变体： 无（正确版本）`，结果都是 40100）。
看起来「五个都通过」，其实是「五个都没生效」。

**教训**：验证脚本的参数栏位要盯紧；输出里那句 `变体： …` 就是为了防这个 ——
**如果没有这一行自报家门，这次会得出完全错误的结论。**

### 坑 2：`write()` 的长度参数写死成 12，输出被截断

```c
write(STDERR_FILENO, "  → _exit(4)\n", 12);   /* 中文字符是 3 字节，实际要 15 字节 */
```

结果打印出 `  → _exit(` 就断了。`write()` **不看 NUL 终止符，只认长度**。
改成 `strlen(m)` 就好了。

### 坑 3：`usleep` 在 `_POSIX_C_SOURCE 200809L` 下被 glibc 藏了

```text
gcc: warning: implicit declaration of function 'usleep'; did you mean 'sleep'?
clang: error: call to undeclared function 'usleep'
```

`usleep` 已在 POSIX.1-2008 中被移除，glibc 在严格特性宏下不再声明它。
gcc 只给警告（于是**悄悄按「返回 int」的隐式声明链接**，能跑），clang 直接报错。
改成 `nanosleep` 封装的 `sleep_us()`。

**教训**：同一份 C 代码在 gcc 下「只是警告」，在 clang 下可能是**致命错误**。
CI 里两个编译器都跑一遍很有必要。

### 坑 4：`sizeof(order_t)` 是 32 不是 24

`int id` + `double price` + `long qty` + 指针：8（对齐后 price）+ 8（qty）+ 8（next）
+ 8（id + 4 字节填充）= 32。所以 LSan 报的是 `200 × 32 = 6400` 字节。
**结构体填充是「内存账」算不平的常见原因**，别手算也别奇怪。
