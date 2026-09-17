# TLPI 第 20 章 — Signals: Fundamental Concepts

**优先级**：🔴（进程控制、daemon、可靠异步事件的理论地基）
**前置**：[Ch19 inotify](../chapter-19-monitoring-file-events/README.md)（同属异步事件，机制不同）
**后置**：[Ch21 Signal Handlers](../chapter-21-signal-handlers/README.md) · [Ch22 高级信号](../chapter-22-signals-advanced/README.md)

---

## 小节目录

- [20.1 概念与概述](notes/20.1-concepts-and-overview.md)
- [20.2 信号类型与默认动作](notes/20.2-signal-types-and-default-actions.md)
- [20.3 改变信号处置：`signal()`](notes/20.3-changing-signal-dispositions-signal.md)
- [20.4 信号处理器简介](notes/20.4-introduction-to-signal-handlers.md)
- [20.5 发送信号：`kill()`](notes/20.5-sending-signals-kill.md)
- [20.6 检查进程是否存在](notes/20.6-checking-for-the-existence-of-a-process.md)
- [20.7 其他发送信号的方式：`raise()` 与 `killpg()`](notes/20.7-other-ways-of-sending-signals-raise-and-.md)
- [20.8 显示信号描述](notes/20.8-displaying-signal-descriptions.md)
- [20.9 信号集（Signal Sets）](notes/20.9-signal-sets.md)
- [20.10 信号掩码（阻塞信号传递）](notes/20.10-the-signal-mask-blocking-signal-delivery.md)
- [20.11 待处理信号（Pending Signals）](notes/20.11-pending-signals.md)
- [20.12 信号不排队](notes/20.12-signals-are-not-queued.md)
- [20.13 改变信号处置：`sigaction()`](notes/20.13-changing-signal-dispositions-sigaction.md)
- [20.14 等待信号：`pause()`](notes/20.14-waiting-for-a-signal-pause.md)
- [20.15 本章小结](notes/20.15-summary.md)
- [20.16 练习题](notes/20.16-exercises.md)

---

## 章节目标


建立信号模型与生命周期；区分标准信号 vs 实时信号；掌握信号集与掩码、未决集；理解递送时机；认清 `signal()` 缺陷与 `kill`/`raise` 语义。

---

## 20.8 易错清单


1. `SIGKILL`/`SIGSTOP` 不可捕/忽/阻
2. 标准信号不排队；实时信号可排队
3. 掩码是**线程**属性
4. 阻塞 ≠ 忽略（pending 待递送 vs 直接丢）
5. `kill(pid,0)` 探测存活
6. 标准信号 pending 看不出“来了几次”
7. handler 内只调异步信号安全函数（Ch21 展开）

---

## 速查：标准 vs 实时 · 阻塞 vs 忽略


| | 阻塞 | 忽略 |
|--|------|------|
| 抵达后 | 进 pending | 丢弃 |
| 解除后 | 会递送 | — |

| 信号类 | 排队 | 数据 |
|--------|------|------|
| 标准 | 否 | 否 |
| 实时 | 是 | 可 |

---

## 练习


1. `sigprocmask` 阻塞 `SIGINT`，Ctrl+C 暂无效
2. `sigpending` 再解除，观察递送
3. （选）对比 `signal()` 局限
4. `kill(pid,0)` 探测

---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 产生 → 未决 → 递送；递送在返用户态前 |
| 2 | KILL/STOP 三不能 |
| 3 | 标准不排队；实时可排队 |
| 4 | 掩码是线程的；`sigprocmask` how 三选一 |
| 5 | 阻塞留 pending；忽略直接丢 |
| 6 | 弃用 `signal()` → `sigaction` |

---

## 参考


- Kerrisk · TLPI Ch20
- `man 7 signal` · `man 2 sigprocmask` · `man 2 kill` · `man 2 sigpending`

---

## 实测硬结论（macOS 26.6.2 / arm64 真机，详见 [code/README.md](code/README.md)）

本章 6 个原书程序 + 2 个自编 demo 全部真实编译运行，实测输出已钉进 code/README.md：

1. **标准信号不排队，亲眼可见**：`sig_sender` 连发 3×信号10，`sig_receiver` 解除屏蔽后 `caught 1 time`——pending 位图只有一个比特，后两次全部合并
2. **SIG_IGN 会抹掉 pending**（习题 20-2 官方解实测）：处置改成 `SIG_IGN` 的瞬间 pending 位图清空，解除阻塞后什么也不会发生——「变更处置 → 丢弃」不用背
3. **跨平台信号编号完全不同**：macOS 是 BSD 系（`SIGUSR1=30/USR2=31`、`SIGBUS=10`、`SIGSYS=12`），Linux 是 System V 系（USR1=10/USR2=12）——`strsignal(10)` 在两边打印完全不同的文字，**编号只能用宏，不能硬编码**
4. **`si_code` 语义有平台差**：Linux `SI_USER==0`，macOS `SI_USER==0x10001`（kill 发的信号 si_code=0）；macOS 的 `si_pid/si_uid` 填 0，追不了发送者
5. **macOS 无实时信号段**：NSIG=32，没有 34–64 的 RT 区间——队列化信号实验只能去 Linux/Pi5
6. （Ch21/22 预演）**ARM64 macOS 整数除零不触发 SIGFPE**：`1/0` 直接出结果继续跑，handler 根本不进——ARM64 `sdiv` 无除零陷阱，x86 演示在 Apple Silicon 上必然落空

---

## 代码示例

| 类型 | 文件 |
|------|------|
| 自编 demo | `kill_probe.c`（20.6）、`block_pending.c`（20.10/20.11） |
| Listing 镜像 | 20-1 `ouch.c` · 20-2 `intquit.c` · 20-3 `t_kill.c` · 20-4 `signal_functions.{c,h}` · 20-6 `sig_sender.c` · 20-7 `sig_receiver.c` |
| 官方习题解镜像 | 习题 20-2 `ignore_pending_sig.c` · 习题 20-4 `siginterrupt.c` |
| 支撑 | `tlpi_hdr.h`（macOS 最小替身）· `get_num.{c,h}`（官方 dist 原版） |

> 编译命令、完整实测输出、macOS↔Linux 差异表、Ch21/22 程序预演结论、Pi5 复测清单
> 全部在 **[code/README.md](code/README.md)**。dist 的 `signals/` 目录混装三章程序，
> 本章只镜像 Ch20 自己的 8 个文件（+2 自编），Ch21/22 的 12 个留到对应章升级时再镜像。
