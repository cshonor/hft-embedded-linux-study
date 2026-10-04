# 9. BPF 工具：睡眠与信号（naptime / signals / killsnoop，13.2.12–13.2.15）

> 底本：《BPF之巅》第 13 章 应用程序，13.2.12–13.2.15 节（印刷 p652–662）

## 13.2.15 naptime（先讲它——排障价值最高）

跟踪 **nanosleep(2) 系统调用**，显示调用者和睡眠时长。作者为调试一个"花几分钟似乎什么都没做"的慢内部构建程序而开发，怀疑含自愿睡眠：

```
# naptime.bt
TIME      PPID  PCOMM      PID    COMM       SECONDS
19:09:19  1     systemd    1975   iscsid     1.000
19:09:20  1     systemd    2274   mysqld     1.000
19:09:21  25137 sleep      2998   build-init 30.000    ← 抓到了！
19:09:22  2421  systemd    2298   irqbalance 9.999
```

- 捕获 **build-init 发起的 30 秒睡眠** → 定位到程序调整睡眠，**构建提速 10 倍以上**
- mysqld/iscsid 每秒睡 1 秒属于正常心跳
- "为解决无关问题而刻意调 sleep"的 hack 会在代码里保留多年进而导致性能问题——此工具专查这个

源代码：

```bash
#!/usr/local/bin/bpftrace
#include <linux/time.h>
#include <linux/sched.h>

tracepoint:syscalls:sys_enter_nanosleep
/args->rqtp->tv_sec + args->rqtp->tv_nsec/
{
    $task = (struct task_struct *)curtask;
    time("%H:%M:%S");
    printf("%-6d %-16s %-6d %-16s %d.%03d\n",
        $task->real_parent->pid, $task->real_parent->comm,
        pid, comm, ...);
}
```

- **父进程信息从 task_struct 读取不可靠**——task_struct 一变代码就要更新
- 可增强：打印 ustack 显示导致睡眠的代码路径（需帧指针）
- sys_enter_nanosleep 跟踪点，开销可忽略

## 13.2.12 signals

跟踪进程信号，显示**信号 × 目标进程**的计数摘要（作者 2019-02-16 开发，源自 2005 年 sig.d）。排查"应用意外终止"（可能收到了某个信号）：

```
# signals.bt
@[SIGNAL, PID, COMM] = COUNT
@[SIGKILL, 3022, sleep]: 1        ← sleep 被 kill，一次就够
@[SIGINT, 2997, signals.bt]: 1
@[SIGCHLD, 21086, bash]: 1
@[SIGSYS, 3014, ServiceWorker t]: 4
@[SIGALRM, 2903, mpstat]: 6
@[SIGALRM, 1882, Xorg]: 87
```

- 实现：**tracepoint:signal:signal_generate**，`@[@sig[args->sig], args->pid, args->comm] = count()`
- 源码含手写 **@sig[0..31] 查询表**（SIGHUP…SIGSYS，来自 /usr/include/asm-generic/signal.h）
- 内核中 **0 号信号没有名字**——它用于**健康检查**（确认目标 PID 还活着），不算真信号
- 信号发送不频繁 → 开销可忽略

## 13.2.13 killsnoop

通过 **kill(2) 系统调用**跟踪信号（BCC/bpftrace 双版本；作者 2004 年首版就是为了调试神秘的应用终止）。与 signals 不同：**只能看到经 kill(2) 发送的**，不是全部信号：

```
# killsnoop
TIME      PID    COMM  SIG  TPID   RESULT
00:28:00  21086  bash  9    3593   0      ← bash 给 PID 3593 发了 SIGKILL
```

- 实现：**syscalls:sys_enter_kill** 存 @tpid[tid]/@tsig[tid]，**sys_exit_kill** 打印并清理（入口/出口配对）
- BCC 选项：`-x` 只显示失败的 kill 调用；`-P PID` 只跟踪该进程
- 可像 signals 那样加信号名查询表增强

## 13.2.16 其他工具：deadlock

BCC 的 **deadlock(8)**（Kenny Yu 2017-02-01 开发）：用**互斥量锁定顺序倒置**的形式检测**潜在死锁**，建立表示互斥量使用的**有向图**。开销可能很高，但有助于调试难题。

## HFT 关联

- naptime 是"启动慢/风控扫描慢"这类问题的第一枪：交易系统里 30 秒 sleep = 30 秒不可交易；warmup 期的 init 脚本 sleep 用它一抓一个准
- signals 里 **SIGSYS 值得盯**（seccomp 拒绝，见第 11 章）；策略进程收到 SIGKILL/SIGTERM 的来源用 killsnoop 追（谁 kill 的——supervisor 还是 OOM）
- OOM killer 杀策略进程不走 kill(2)，killsnoop 看不到——要查内核日志/第 7 章 oomkill 工具

<details>
<summary>自测题</summary>

1. naptime 的哪一行代码被作者明确标注"不可靠"？
   <details><summary>答</summary>从 curtask->real_parent 读父进程 PID/comm——依赖 task_struct 布局，内核一变就需更新。</details>

2. 0 号信号是什么用途？
   <details><summary>答</summary>健康检查：确认目标 PID 是否还在运行，内核源码中没有名字。</details>

3. signals 和 killsnoop 的覆盖范围差异？
   <details><summary>答</summary>signals 跟踪 signal:signal_generate 跟踪点，覆盖所有信号；killsnoop 只跟踪经 kill(2) 系统调用发送的信号。</details>

4. deadlock(8) 用什么原理检测死锁？
   <details><summary>答</summary>建立互斥量锁定顺序的有向图，检测锁定顺序倒置（lock order inversion）。</details>
</details>
