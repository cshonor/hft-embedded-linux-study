# 5.2 strace 实战分析（-c 统计 / -f 子进程 / -p attach / 阻塞与多余 syscall 定位）

> 🔴 精读 · 从「流水账」到「结论」

## 本节要点

裸的 strace 输出是海量流水账，实战要靠一组参数把「噪声」压下去、把「信号」提出来：`-c` 统计调用频次与耗时、`-e` 过滤只关心某类调用、`-f`/`-p` 覆盖子进程与运行中进程、`-T`/`-ttt` 拿到精确耗时。本节讲清这些参数，并用两个实战场景——「进程卡住」和「热路径多余 syscall」——演示怎么从输出得出结论。

## -c：先看统计，别陷进流水账

`-c` 汇总所有 syscall 的**调用次数、失败次数、总耗时、占比**，是「程序时间都花在哪」的第一眼：

```bash
strace -c ./orderbook 2>&1
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 99.98    0.000234         234         1           write
  0.02    0.000000           0         1           execve
  0.00    0.000000           0         1           exit_group
------ ----------- ----------- --------- --------- ----------------
100.00    0.000234                     3                     total
```

| 列 | 含义 |
|----|------|
| `% time` | 该 syscall 占总耗时比例（定位耗时大户） |
| `usecs/call` | 单次平均耗时（微秒） |
| `calls` | 调用次数 |
| `errors` | 失败次数 |

> 看到某个 syscall 的 `calls` 异常高、或 `usecs/call` 异常大，就顺着往下挖。`-c` 是**先宏观后微观**的入口。

## -e trace=：只关心某一类调用

```bash
strace -e trace=file ./prog          # 只看文件相关（open/read/write/stat...）
strace -e trace=network ./prog       # 只看网络（socket/connect/send/recv...）
strace -e trace=process ./prog       # 只看进程（fork/exec/clone...）
strace -e trace=read,write ./prog    # 只看指定 syscall
strace -e trace=!futex ./prog        # 排除 futex（多线程程序巨吵）
strace -e trace=openat,read,write -e read=3 ./prog   # 只读 fd 3 的 read
```

```bash
# 追踪网络程序只看网络调用
strace -e trace=network ./client
socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) = 3
connect(3, {sa_family=AF_INET, sin_port=htons(8080), sin_addr=inet_addr("127.0.0.1")}, 16) = 0
sendto(3, "hello", 5, 0, NULL, 0) = 5
recvfrom(3, "world", 4096, 0, NULL, NULL) = 5
```

## -f / -p：覆盖子进程与运行中进程

```bash
# 多线程程序：clone 出来的线程也要追
strace -f -e trace=futex ./orderbook_mt 2>&1 | head
# 每个线程的 syscall 前有 [pid 12345] 前缀

strace -f -p 12345       # attach 到运行中进程 + 追它 fork 的子进程
```

```bash
# attach 到卡住的进程，看它卡在哪个 syscall
strace -p $(pidof matching_engine)
# strace: Process 12345 attached
# futex(0x..., FUTEX_WAIT_PRIVATE, 2, NULL) = 0   ← 卡在等锁（futex）
# ... 或
# recvfrom(5, ...)                               ← 卡在等网络数据
```

> attach 的进程被 ptrace 接管时也会暂停，看完 `Ctrl-C` 退出 strace 进程即恢复。attach 前先 `ps -o wchan` 判断方向（见 5.4），再用 strace 精确定位卡在哪个 syscall、什么 fd。

## -T / -ttt：拿到精确耗时

```bash
strace -T -e trace=read,write ./prog
# read(3, "hello\n", 4096) = 6 <0.000012>       # <耗时> 12 微秒
# write(1, "hello\n", 6)   = 6 <0.000045>       # 45 微秒

strace -ttt -T -e trace=recvfrom ./client
# 1725358201.123456 recvfrom(5, "world", 4096, 0, NULL, NULL) = 5 <0.250123>
#        ↑ 绝对时间戳                    ↑ 这个 recvfrom 花了 250ms ← 延迟毛刺！
```

`-T` 直接标出每个 syscall 耗时，`-ttt` 给出微秒级绝对时间。两者结合能还原「哪个 syscall 之间卡了 250ms」——这是 HFT 延迟溯源的原始素材。

## 实战一：定位「进程卡住」

```bash
# 1. 先看进程在等什么
ps -o pid,stat,wchan -L -p $(pidof server)
# 12345 S  sk_wait_data     ← 卡在等 socket 数据

# 2. strace attach 精确定位
strace -p 12345 -e trace=recvfrom,read,poll,epoll_wait -T
# recvfrom(5, ...) = -1 EAGAIN (Resource temporarily unavailable) <0.000001>
# epoll_wait(3, ...) = 1 <0.250000>
# recvfrom(5, ...)           ← 最后停在这，一直没返回
```

结论：进程卡在 `recvfrom(fd=5)` 等数据，对端一直不发 → 不是死锁，是网络/对端问题。若是 `futex(...FUTEX_WAIT...)` 停在最后，则是锁等待（可能死锁）。

## 实战二：定位「热路径多余 syscall」

```c
// tick.c —— 交易循环里每轮都调 clock_gettime（模拟"误以为很便宜"）
#include <time.h>
#include <stdio.h>
int main(void) {
    struct timespec ts;
    for (int i = 0; i < 1000; i++) {
        clock_gettime(CLOCK_REALTIME, &ts);   // 每轮都取时间
        // ... 实际处理 ...
    }
    return 0;
}
```

```bash
gcc -O2 -o tick tick.c
strace -c ./tick 2>&1
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 94.12    0.004100           4      1000           clock_gettime   # ← 1000 次！
```

`-c` 立刻暴露：`clock_gettime` 调了 1000 次，占总耗时 94%。但注意——**`clock_gettime` 通常走 vDSO，不该是 syscall**。若 strace 里出现了它，说明要么 vDSO 没生效，要么代码用了会陷入内核的时钟源。这就是「本可避免的 syscall」的典型。

> ⚠️ **strace 的输出本身可能误导你**：vDSO 里的 `clock_gettime`/`gettimeofday` 正常情况下**不进内核、不产生 syscall**，strace 根本看不到。若 strace 显示了它，反而是线索——说明走了 syscall 路径，值得深挖（时钟源配置 / 是否禁用了 vDSO）。

## strace 的性能开销：为什么不能用于生产热路径

strace 用 ptrace，**每个 syscall 都要两次上下文切换**（进入时停一次、返回时停一次），对 syscall 密集的程序可放大 **20×–100×** 甚至更多：

| 场景 | 影响 |
|------|------|
| syscall 稀疏的程序 | 影响小，可接受 |
| 网络/文件密集程序 | 明显变慢，时序被扰动 |
| 低延迟热路径 | **时序全乱**，测出来的延迟不真实 |

所以 strace 是**开发/诊断工具**，不是生产观测工具。生产环境零开销的动态追踪要交给 eBPF（`06.7` 模块），strace 定位「逻辑对不对」，eBPF 回答「生产环境到底多快」。

## HFT 关联

1. **`-c` 抓「多余 syscall」**：热路径里每个 syscall 都是延迟成本，`-c` 一眼看出哪个 syscall 被过度调用（`gettimeofday`、不必要的 `read`、频繁 `futex`），针对性优化。
2. **`-ttt -T` 延迟溯源**：微秒时间戳 + 单 syscall 耗时，还原「哪一步卡了 250ms」，是延迟毛刺定位的原始数据。
3. **attach 定位卡死**：生产进程卡住先 `strace -p`（轻、无需符号），区分「等锁 futex」「等 IO recvfrom」「忙循环」三类，再决定上不上 gdb。
4. **⚠️ 别在热路径跑 strace 测延迟**：ptrace 开销会污染时序，测出的数字不真实；生产观测用 eBPF（06.7）。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** `-c` 统计里 `% time` 和 `calls` 分别帮你发现什么问题？

> `% time` 高 = 该 syscall 是**耗时大户**（可能卡在这里，如网络 read）；`calls` 异常高 = 该 syscall 被**过度调用**（本可避免，如每轮都 gettimeofday）。前者指向「慢在哪」，后者指向「多做了什么」，两个维度不同。

**Q2:** 多线程程序直接 `strace ./prog` 能看到所有线程的 syscall 吗？

> 不能。默认 strace 只追**主线程**。要看所有线程，必须加 `-f`（追踪 clone 出来的线程），输出里每条 syscall 前会带 `[pid <tid>]` 前缀区分是哪个线程。

**Q3:** 进程卡住，`strace -p` 最后一行是 `futex(...FUTEX_WAIT...)` vs `recvfrom(5, ...)`，结论有何不同？

> `futex(FUTEX_WAIT)` 表示在**等锁**——可能是死锁或长时间持锁；`recvfrom` 表示在**等网络数据**——对端没发/网络问题。两者分别是「同步问题」和「IO 问题」，排查方向完全不同（前者查锁序，后者查对端与网络）。

**Q4:** 为什么说 strace 不能用来测生产环境的真实延迟？

> strace 用 ptrace 拦截，每个 syscall 要两次上下文切换（进、出），对 syscall 密集程序放大 20–100 倍，且会**扰动时序**（本来并发的被串行化）。用它测延迟，数字被 ptrace 开销污染，不真实。生产零开销观测要用 eBPF。

**Q5:** strace 里居然出现了 `clock_gettime` syscall，为什么这本身是个「异常信号」？

> 因为 `clock_gettime`/`gettimeofday` 正常情况下走 **vDSO**（内核映射到用户态的只读代码页），不陷入内核、不产生 syscall。strace 能看到它，说明走了真正的 syscall 路径——要么 vDSO 被禁用/未生效，要么用了特殊时钟源。这是个值得深挖的性能线索，而非正常现象。

</details>

## 交叉引用

- [5.1 strace 入门](01-strace-basics.md)
- [5.3 ltrace 库调用追踪](03-ltrace-library-calls.md)
- [5.4 attach 运行中进程](04-attach-running-process.md)
- [03.6 模块导读](../../README.md)
