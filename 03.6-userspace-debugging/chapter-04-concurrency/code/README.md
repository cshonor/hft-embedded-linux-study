# Ch4 并发类 · 可跑的 demo

本章两个程序。它们都**不依赖 gdb / TSan 才能看**——因为它们回答的是同一个问题的
两个侧面：4.1 教的 `thread apply all bt` 到底在给你什么信息。

| 文件 | 演示什么 | 一句话看点 |
|------|----------|------------|
| `c4_1_data_race.c` | 同一件事的四种写法：裸自增 / 互斥锁 / C11 原子 / 每线程私有 | 裸自增静默丢掉 **50%** 的更新，退出码仍是 **0** |
| `c4_2_deadlock.c` | 确定性 AB-BA 死锁 + 进程内看门狗自证 | 看门狗用 `pthread_mutex_timedlock` 证明「两把锁都被持有 + 没人推进」 |

> **关于「实测」**：下面的数字来自 Compiler Explorer（godbolt.org）上的
> **gcc 13.3.0 / clang 18.1.0**，不是本机跑出来的（本环境没有 C 编译器）。
> 结论（丢更新、退出码、报告形状）是可靠的；**耗时随机器变化，别看绝对值，
> 看同一台机器上的倍数关系**。

---

## c4_1_data_race.c —— 一行 C 代码不等于一个原子操作

```bash
# 只看结果对不对（此时才谈得上比较耗时）
cc -g -O2 -pthread -Wall -Wextra -o c4_1  c4_1_data_race.c
./c4_1 race      # 裸自增，无同步
./c4_1 mutex     # 互斥锁
./c4_1 atomic    # C11 原子操作
./c4_1 local     # 每线程私有 + 最后合并

# 查竞争（必须用 clang，理由见下）
cc -g -O1 -fsanitize=thread -pthread -o c4_1_tsan c4_1_data_race.c
./c4_1_tsan race
```

### 实测结果（gcc 13.3.0，`-g -O2 -pthread`，每模式 3 次取最小）

| 模式 | 实际结果（期望 200000） | 丢失 | 耗时 | 退出码 |
|------|------------------------|------|------|--------|
| `race` | **100000** | **100000（50.00%）** | 11.8 ms | **0** |
| `mutex` | 200000 | 0 | 25.5 ms | 0 |
| `atomic` | 200000 | 0 | 8.1 ms | 0 |
| `local` | 200000 | 0 | 8.1 ms | 0 |

程序对 `race` 模式自己打出的结论：

```text
实际结果 = 100000   （丢了 100000 次更新，50.00%）
耗时     = 11.8 ms（3 次最小值）

注意：进程**没有崩**，退出码仍是 0，stderr 也是空的。
      结果错了，进程却「成功」了 —— 这就是 1.2 决策树里
      「结果偶尔不对」必须用 TSan、不能盯退出码的原因。
```

### TSan 实测（clang 18.1.0，`-g -O1 -fsanitize=thread -pthread`）

```text
实际结果 = 200000   （丢了 0 次更新，0.00%）
...
--- stderr ---
==================
WARNING: ThreadSanitizer: data race (pid=2)
  Read of size 4 at 0x5555569dd660 by thread T2:
    #0 th_race /app/example.c (output.s+0xdce64)

  Previous write of size 4 at 0x5555569dd660 by thread T1:
    #0 th_race /app/example.c:104:16 (output.s+0xdce8a)

  Location is global 'g_race' of size 4 at 0x5555569dd660 (output.s+0x1489660)

  Thread T2 (tid=5, running) created by main thread at:
    #0 pthread_create .../tsan_interceptors_posix.cpp:1022:3 (output.s+0x5f7bb)
    #1 run_threads /app/example.c:134:9 (output.s+0xdcabf)
    #2 main /app/example.c:150:24 (output.s+0xdcabf)
  ...
SUMMARY: ThreadSanitizer: data race /app/example.c:104:16 in th_race
==================
ThreadSanitizer: reported 2 warnings
```

**两次运行的结论正好互补**：

| 运行 | 结果 | TSan | 说明 |
|------|------|------|------|
| `./c4_1 race`（gcc，无 TSan） | 100000（错） | 不存在 | 结果错了，退出码 0，静默 |
| `./c4_1_tsan race`（clang + TSan） | 200000（**对的**） | **报 2 条竞争** | 结果对了，TSan 照样报 |

第二条就是 4.3 自测题 Q5 的实证：**TSan 检查的是「竞争关系」本身，
不是「竞争的后果」**。所以它比「多跑几遍看结果」可靠。

对照组 `./c4_1_tsan atomic`：退出码 0，stderr 全空 —— C11 原子操作是 TSan
认可的同步，不是竞争。

---

## c4_2_deadlock.c —— 让程序自己证明它死了

```bash
cc -g -O1 -pthread -Wall -Wextra -o c4_2_deadlock c4_2_deadlock.c
./c4_2_deadlock ordered   # 统一锁序 → 正常跑完，退出码 0
./c4_2_deadlock abba      # 反序加锁 → 死锁，看门狗报出，退出码 3
```

### 为什么这个 AB-BA 是**确定性**的

教科书的 AB-BA 例子跑十次有三次顺利跑完——谁先拿到第一把锁看调度运气。
这里加了一次握手：两个线程各自先拿到自己的第一把锁、用原子变量通报「我拿到了」、
并**等对方也通报完**，才一起去抢第二把锁。于是环**必然**形成。

### `ordered` 模式实测输出（正常收工）

```text
[watchdog 第 0轮] A: hb=1     持有两把锁        | B: hb=171   持有两把锁
[watchdog 第 1轮] A: hb=171   持有两把锁        | B: hb=171   抢第一把锁
[watchdog 第 2轮] A: hb=338   持有两把锁        | B: hb=171   抢第一把锁
[watchdog 第 3轮] A: hb=392   抢第一把锁        | B: hb=286   持有两把锁
[watchdog 第 4轮] A: hb=400   已完成            | B: hb=400   已完成

[watchdog] 两个线程都已 ST_DONE，心跳从 0 涨到 400 —— 全程有推进，无死锁。

主线程：两个 worker 都正常返回，退出码 0。
```

### `abba` 模式实测输出（死锁，退出码 3）

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

[2] 独立取证：两把锁到底在谁手里（看门狗亲自去试）
    pthread_mutex_timedlock(&lock_a) → 被占用（timedlock 超时）
    pthread_mutex_timedlock(&lock_b) → 被占用（timedlock 超时）
    ↑ 两把锁同时被占，且持有者毫无进展 → 环形等待成立

[3] 还原锁序
    thread-A: 拿到 lock_a → 在等 lock_b
    thread-B: 拿到 lock_b → 在等 lock_a
    → A: a→b，B: b→a，两个方向相反，环闭合。这就是 AB-BA 死锁。
```

退出码 **3** 是**看门狗主动判定后自己退的**，不是崩溃 —— 这和 139/134 那类
「信号打死的」有本质区别。看到 3 就该想到「有人做了判断」。

---

## 踩坑记录（都是实跑出来的）

### 坑 1：耗时数字在容器里噪声很大

第二轮测量时出现过 `atomic` 8.2 ms 反而快过 `local` 20.7 ms 的荒谬结果
（`local` 是纯寄存器累加 + 同样的假活，理应不慢于要 `lock xadd` 的 `atomic`）。
那是容器 CPU 调度的噪声。所以程序改成**每模式跑 3 次取最小值**，
并加了 `-D` 无关的 `REPEAT` 宏（TSan 下自动降为 1 次，因为插桩后耗时无意义）。

**教训**：容器里的单次计时不可信；要么重复取最小，要么只信倍数关系。

### 坑 2：`volatile int` 自增**不足以**稳定复现丢更新

第一版 `race` 模式写的是 `g_race++`（`g_race` 为 `volatile`），150 万次自增只跑
1.0 ms —— 两线程几乎没交错，**一次更新都没丢**（结果正好等于期望值）。

改成把 `g_race++` 显式拆成三步、中间插 40 次假活后，稳定丢 50%。
这不是造假：现实里「读」和「写回」之间本来就有 cache miss、分支预测、
调度抢占撑开的窗口，只是我们把它显式化了。

**教训**：「竞态偶发」的根源是窗口窄 + 依赖时序。想让 demo 稳定复现，
就得把窗口撑开（也顺便说明了为什么生产环境的竞态那么难抓）。

### 坑 3：gcc 版 TSan 在本环境跑不起来 —— 必须用 clang

```text
[gcc 13.3.0 + -fsanitize=thread]
--- stdout ---
--- stderr ---
FATAL: ThreadSanitizer: unexpected memory mapping 0x7913d9072000-0x7913d9500000
（退出码 66，但程序一行都没执行）
```

同一个程序换成 clang 18.1.0 就完全正常。看到 `unexpected memory mapping`
不要怀疑自己的代码，那是 TSan 运行时和容器地址空间布局冲突。

### 坑 4：`ordered` 模式第 0 轮的数字看着「不对」

```text
[watchdog 第 0轮] A: hb=1     持有两把锁        | B: hb=171   持有两把锁
```

A 才 1 格、B 已经 171 格，差了 170 倍。原因有两个，都正常：

1. 采样是**时间点快照**，不是同步事务 —— 打印 A 和打印 B 之间已经过了若干微秒；
2. glibc 的普通互斥锁**不保证公平**（会 barging），同一个线程可以连续赢很多次。

看到这种数字不要以为有 bug，也不要拿它当性能结论。
