# TLPI 第 21 章 — Signals: Signal Handlers

**优先级**：🔴（崩溃/死锁/EINTR/僵尸回收）
**前置**：[Ch20 信号基础](../chapter-20-signals-fundamentals/README.md)
**后置**：[Ch22 pause / sigsuspend](../chapter-22-signals-advanced/README.md) · [Ch24 进程创建 / wait](../chapter-24-process-creation/README.md)

---

## 小节目录

- [21.1 设计信号处理器](notes/21.1-designing-signal-handlers.md)
- [21.2 从信号处理器终止进程](notes/21.2-other-methods-of-terminating-a-signal-ha.md)
- [21.3 在备用栈上处理信号](notes/21.3-handling-a-signal-on-an-alternate-stack-.md)
- [21.4 `SA_SIGINFO` 标志](notes/21.4-the-sa-siginfo-flag.md)
- [21.5 系统调用的中断与重启](notes/21.5-interruption-and-restarting-of-system-ca.md)
- [21.6 本章小结](notes/21.6-summary.md)
- [21.7 练习题](notes/21.7-exercise.md)

---

## 章节目标

掌握 handler 设计的两条黄金法则（只设标志 + 只调异步信号安全函数）；理解 `volatile sig_atomic_t`
两个限定符的各自作用；能用 `sigaltstack` + `SA_ONSTACK` 处理栈溢出崩溃；理解 `SA_SIGINFO` 与
`siginfo_t` 的 union 语义；正确处理 `EINTR` 与 `SA_RESTART` 的边界。

---

## 易错清单

1. `sa_mask` 叠加，不替换；退出即撤（由 `sigreturn` 自动恢复）
2. `SA_NODEFER` → 允许自我重入，易递归
3. `EINTR` 合法，别当致命；`SA_RESTART` 非万能（带超时调用仍返回 `EINTR`）
4. 标志用 `volatile sig_atomic_t`——两个限定符缺一不可
5. handler 禁 `printf`/`malloc`/`exit`
6. 实时信号 + `sigqueue` 需 `SA_SIGINFO` 才拿得到 `si_value`
7. 多线程：信号交给**未阻塞该信号**的某线程执行，不可预测
8. `siginfo_t` 是 union——只读当前信号匹配的有效字段
9. `close()` 返回 `EINTR` 时 **fd 已关闭，不要重试**
10. handler 里要保护并恢复 `errno`

---

## 练习


1. `sigaction` + `sa_mask`
2. 开关 `SA_RESTART` 看 `read`/`EINTR`
3. `SA_SIGINFO` 打印发送者 pid
4. `sig_atomic_t` 主循环范式
5. `SIGCHLD` 循环 `waitpid`

---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 用 `sigaction`，不用 `signal` |
| 2 | handler 期间：自动阻本信号 + `sa_mask` |
| 3 | 只设 `volatile sig_atomic_t`；禁非 async-safe |
| 4 | 处理 `EINTR`；`SA_RESTART` 非万能 |
| 5 | `SIGCHLD`：循环 `WNOHANG` 或 `SA_NOCLDWAIT` |
| 6 | `SA_SIGINFO` 才拿得到 `siginfo_t` / 队列数据 |

---

## 参考


- Kerrisk · TLPI Ch21
- `man 2 sigaction` · `man 7 signal-safety` · `man 2 waitpid`

---

## 实测硬结论（macOS 26.6.2 / arm64 真机，详见 [code/README.md](code/README.md)）

本章 3 个原书 Listing + 4 个自编 demo + 1 个习题实现全部真实编译运行（`nonreentrant` 除外，见下）：

1. **longjmp 的掩码语义是平台相关的，不是理论题**：同一份 `sigmask_longjmp` 代码，macOS 上跳回后掩码恢复空集；POSIX 对 setjmp 版明说"未定义"，Linux 上常表现为 SIGINT 保持阻塞——所以必须用 `sigsetjmp(env, 1)`
2. **sigaltstack 真的救命**：递归 83 层撑爆 8MB 主栈，SIGSEGV handler 稳稳落在 `sigaltstack` 区间（`0x150037b58` vs 主栈 `0x16a9...`）
3. **EINTR 实测复现**：无 `SA_RESTART` 的 sigaction 下，SIGINT 打断阻塞 `read` → `-1/EINTR`，handler 置旗的值同时可见
4. **SIGCHLD 循环 waitpid 不是风格问题**：3 个子进程同亡只投递一次 SIGCHLD（不排队），`while(waitpid(-1,...,WNOHANG))>0` 是唯一正确写法——实测无僵尸退出
5. **自实现 abort() 全场景过**（习题 21-1）：阻塞拦不住（SUSv3 override）、handler 返回则复位 SIG_DFL 必死、终止前 flush stdio（缓冲内容能打出来正是 flush 语义）
6. `nonreentrant`（21-1）macOS 无 `<crypt.h>` 编译不过——Linux 专有，Pi5 复测项

---

## 本仓库代码

| 类型 | 文件 |
|------|------|
| Listing 镜像 | 21-1 `nonreentrant.c` · 21-2 `sigmask_longjmp.c` · 21-3 `t_sigaltstack.c` · 补充 `nonatomic_uint64.c` |
| 自编 demo | `flag_handler.c`（21.1）· `eintr_read.c`（21.5）· `siginfo_demo.c`（21.4）· `sigchld_reap.c`（21.1） |
| 习题实现 | 21-1 `ex21_1_abort.c`（自实现 abort） |
| 支撑 | `tlpi_hdr.h`（macOS 替身）· `get_num.{c,h}`（dist 原版）· `signal_functions.{c,h}`（跨章依赖 Listing 20-4） |

> 编译命令、完整实测输出、macOS↔Linux 差异、Pi5 复测清单全部在
> **[code/README.md](code/README.md)**。dist `signals/` 里 Ch21/22 混装，
> 本章只镜像 Ch21 自己的 4 个文件。
