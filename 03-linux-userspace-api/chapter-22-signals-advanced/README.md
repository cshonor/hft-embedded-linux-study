# TLPI 第 22 章 — Signals: Advanced Features

**优先级**：🔴（可靠等待、实时信号、崩溃栈、服务进程信号模型）
**前置**：[Ch21 Signal Handlers](../chapter-21-signal-handlers/README.md)
**后置**：[Ch23 Timers and Sleeping](../chapter-23-timers-sleeping/README.md) · [Ch29+ 线程](../chapter-29-threads-intro/README.md) · Ch63 多路 I/O

---

## 小节目录

- [22.1 Core Dump 文件](notes/22.1-core-dump-files.md)
- [22.2 递送、处置与挂起的特殊情况](notes/22.2-special-cases-for-delivery-disposition-a.md)
- [22.3 可中断与不可中断的进程睡眠状态](notes/22.3-interruptible-and-uninterruptible-proces.md)
- [22.4 硬件产生的信号](notes/22.4-hardware-generated-signals.md)
- [22.5 同步与异步的信号生成](notes/22.5-synchronous-and-asynchronous-signal-gene.md)
- [22.6 递送的时序与顺序](notes/22.6-timing-and-order-of-signal-delivery.md)
- [22.7 `signal()` 的实现与可移植性](notes/22.7-implementation-and-portability-of-signal.md)
- [22.8 实时信号](notes/22.8-realtime-signals.md)
- [22.9 使用掩码等待信号：`sigsuspend()`](notes/22.9-waiting-for-a-signal-using-a-mask-sigsus.md)
- [22.10 同步等待信号（`sigwaitinfo`）](notes/22.10-synchronously-waiting-for-a-signal.md)
- [22.11 通过文件描述符获取信号（`signalfd`）](notes/22.11-fetching-signals-via-a-file-descriptor.md)
- [22.12 用信号做进程间通信](notes/22.12-interprocess-communication-with-signals.md)
- [22.13 早期信号 API（System V 和 BSD）](notes/22.13-earlier-signal-apis-system-v-and-bsd.md)
- [22.14 本章小结](notes/22.14-summary.md)
- [22.15 练习题](notes/22.15-exercises.md)

---

## 章节目标

本章回答 Ch20/Ch21 讲完基础后必然冒出的四个问题：

1. **时序**：信号到底什么时候递送？多个 pending 谁先谁后？为什么 `kill -9` 有时杀不掉？（22.2 / 22.3 / 22.6）
2. **接收方式**：除了装 handler，还有 `sigsuspend` / `sigwaitinfo` / `signalfd` 三条路——各自适用什么场景、为什么 `signalfd` 抓不到 `SIGSEGV`。（22.9 / 22.10 / 22.11）
3. **硬件异常**：段错误、除零这类「CPU 自己产生的信号」与 `kill()` 在内核里走同一条路吗？（22.4 / 22.5）
4. **历史包袱**：`signal()` / `sigset()` / `sigpause()` 的来路，以及为什么生产代码一律用 `sigaction`。（22.7 / 22.13）

---

## 三种接收方式速览

| 方式 | 跑 handler？ | 能进 epoll？ | 能抓 `SIGSEGV`？ | 典型用途 |
|------|--------------|--------------|------------------|----------|
| `sigaction` handler | ✅ 是 | ❌ | ✅ **唯一能抓** | 崩溃兜底、栈溢出收尸 |
| `sigsuspend(&mask)` | ✅ 是 | ❌ | ⚠️ 理论可以但不该 | 单线程原子等待 |
| `sigwaitinfo` / `sigtimedwait` | ❌ 否 | ❌ | ❌ | 多线程专用信号线程（推荐） |
| `signalfd` + `read` | ❌ 否 | ✅ **是** | ❌ | 主事件循环统一纳管 |

**关键约束**：`signalfd` / `sigwaitinfo` 之前**必须先 `sigprocmask(SIG_BLOCK, ...)`**，否则信号走默认动作或 handler，fd 上永远没数据。

---

## 易错清单

1. `sigsuspend` 的参数是**睡眠期间的完整掩码**，不是「要等哪些信号」——传「只含目标信号」的集合是典型错误
2. `pause()` 有致命竞态（解除阻塞 → 信号到达 → 永远睡着）；一律用 `sigsuspend`
3. `sigwaitinfo` / `signalfd` 之前**必须先阻塞**目标信号，否则信号被 handler 或默认动作消费掉
4. `signalfd` 的 `read` 缓冲必须 ≥ `sizeof(struct signalfd_siginfo)`，否则返回 **`-EINVAL`**（不是 `ENOBUFS`）
5. `signalfd` / `sigwaitinfo` **抓不到** `SIGSEGV`/`SIGBUS`/`SIGFPE`/`SIGILL`——同步故障信号走 `force_sig_info_to_task():1336` 线程定向直投，只能 `sigaction` + `SA_ONSTACK` 兜底
6. 标准信号**不排队**（`legacy_queue():1079`）：`SIGCHLD` 必须 `while (waitpid(-1, NULL, WNOHANG) > 0);`
7. 多个 pending 信号的递送顺序是**信号编号小的优先**（`next_signal():209`），不是 FIFO
8. `kill -9` 杀不掉 `TASK_UNINTERRUPTIBLE`（`D` 状态）进程——信号挂上了但进程叫不醒
9. `SA_ONSTACK` 是**按信号**注册的，不是全局开关；每个崩溃信号都要单独设，且要先 `sigaltstack`
10. 崩溃 handler 不能正常返回（CPU 会重试肇事指令 → 无限循环），写完现场后 `_exit` 或 re-raise
11. `SIGKILL`/`SIGTERM` **不在** `SIG_KERNEL_COREDUMP_MASK` 里——要留 core 用 `kill -QUIT`
12. `signal()` 在 Linux 内核层（`sys_signal:4622`）和 glibc 层语义**相反**；跨 UNIX 更不可移植。生产一律 `sigaction` 并显式写 `sa_flags`
13. `sigpause` **同名不同义**：BSD 版参数是掩码、SysV 版参数是信号编号，参数都是 `int`，编译器无法区分
14. 从 BSD 老代码迁移 `sigaction` 时最易漏 `SA_RESTART`——两派默认值相反，漏了会偶发 `EINTR`
15. 内核对未识别的 `sa_flags` **静默清除不报错**（`do_sigaction():4153-4156`），唯一可靠验证是装完读回比对

---

## 练习

1. 复现 `pause` 竞态，用 `sigsuspend` 修复
2. `sigwaitinfo` 循环取信号（注意 `EAGAIN` 超时 vs `EINTR` 打断）
3. `sigqueue` 传 `int`，并测试队列溢出（`EAGAIN`）
4. `signalfd` + `epoll` 统一事件循环
5. （选）`sigaltstack` + `SA_ONSTACK` 处理栈溢出
6. （选）诊断 `D` 状态进程：`ps -eo stat,wchan` + `/proc/PID/status` 的 `ShdPnd`
7. （选）遗留 API 迁移审查：`grep -nE '\b(signal|sigset|sighold|sigpause|sigblock|sigvec)\s*\('`

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | `pause` 有竞态；等待信号用 `sigsuspend`（换掩码 + 睡，原子） |
| 2 | `sigwaitinfo` **取走**信号（不跑 handler）；`sigsuspend` **递送**信号（跑 handler） |
| 3 | 同步等待 / `signalfd` 前**必须阻塞**目标信号 |
| 4 | 多线程标准模型：main 阻塞全部 → 起一个 `sigwaitinfo` 线程 |
| 5 | `SIGSEGV` 等同步故障信号只能 handler 抓，`signalfd` 抓不到 |
| 6 | 标准信号不排队；`SIGCHLD` 要循环 `waitpid` |
| 7 | 递送顺序：号小者优先（同步信号除外，它们最优先） |
| 8 | `D` 状态（`UNINTERRUPTIBLE`）连 `SIGKILL` 也叫不醒 |
| 9 | `sigqueue` + 实时信号可排队可带数据；跨进程**不能**传指针 |
| 10 | 生产代码不用 `signal()`；`sa_flags` 显式写，装完读回验证 |

---

## 参考

- Kerrisk · TLPI Ch22
- `man 2 sigsuspend` · `man 3 sigwaitinfo` · `man 3 sigqueue` · `man 2 signalfd` · `man 2 sigaltstack` · `man 5 core` · `man 7 signal`

---

## 实测硬结论（macOS 26.6.2 / arm64 真机，详见 [code/README.md](code/README.md)）

8 个官方镜像 + 3 个自编中，**5 个在 macOS 真实编译运行、5 个 Linux 专有只做源码核验**：

1. **sigsuspend 原子性实测复现**：临界区（INT/QUIT 全屏蔽）内到达的 SIGINT 变 pending，`Caught signal 2` 精确落在 sigsuspend 点——22.9「先阻塞才不丢」的灵魂输出
2. **标准信号合并 vs RT 排队**：`catch_rtsigs` 实测发 USR1×2 只 caught 1 次；RT 排队验证需 Linux（macOS NSIG=32 无 RT 段）
3. **⚠️ ARM64 整数除零不触发 SIGFPE**：`demo_SIGFPE` 在 Apple Silicon 上 handler 根本不进（`sdiv` 无陷阱，x86 才有）——22.4 的演示在 M 系列 Mac 上必然落空
4. **macOS 阻塞期延迟递送的信号不回填 si_pid/si_uid**（=0）；运行期直收则正常——Linux 会一直填
5. `sig_speed_sigsuspend 2000` 实测 0.04s——sigsuspend 往返 ≈20µs，仍是「廉价 IPC」量级
6. macOS 无 `sigqueue()/sigwaitinfo()/signalfd`——三个自编 RT demo 全部标注 Linux 专有，Pi5 编译复测

---

## 代码示例

| 类型 | 文件 |
|------|------|
| Listing 镜像 | 22-1 `signal.c` · 22-2 `t_sigqueue.c` · 22-3 `catch_rtsigs.c` · 22-5 `t_sigsuspend.c` · 22-6 `t_sigwaitinfo.c` · 22-7 `signalfd_sigval.c` · 补充 `demo_SIGFPE.c` · `sig_speed_sigsuspend.c` |
| 自编 demo | `sigsuspend_wait.c`（22.9）· `sigwaitinfo_loop.c`（22.10）· `sigqueue_rt.c`（22.8） |
| 支撑 | `tlpi_hdr.h`（macOS 替身）· `get_num.{c,h}`（dist 原版）· `signal_functions.{c,h}`（跨章依赖 Listing 20-4） |

> 编译分类表、完整实测输出、Linux 专有语义核验 + Pi5 复测清单全部在
> **[code/README.md](code/README.md)**。dist `signals/` 混装三章程序，
> 本章只镜像 Ch22 自己的 8 个文件。
