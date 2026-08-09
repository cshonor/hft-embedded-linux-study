## ⑥ 进程终结 · exit, zombie & wait

#### 触发路径

| 路径 | 说明 |
|------|------|
| **`exit()` / `_exit()` / `return main`** | 主动退出 — 用户库清理后进入内核 `do_exit()` |
| **无法处理的信号** | 默认动作 `SIG_DFL` 为 Term — 如 `SIGSEGV`、`SIGKILL` |
| **最后一个线程退出** | 线程组内无其他线程时整组结束 |
| **`pthread_exit`** | 仅结束 **当前线程**；末线程退出 ≡ 进程退出 |

#### 清理：`do_exit()` 阶段

| 步骤 | 动作 |
|------|------|
| 1 | 设 `PF_EXITING`，从各种队列/定时器摘除 |
| 2 | 释放 **大部分** 内存、`files`、信号等 |
| 3 | 设 **`EXIT_ZOMBIE`** — **保留** `task_struct` + 退出码 |
| 4 | 通知父进程（`SIGCHLD` 等） |
| 5 | 父 **`wait`** 后 → `release_task()` 彻底释放描述符 |

```
子进程 exit ──► ZOMBIE（占 slot，几乎不占内存）
                    │  exit code 在 task_struct
父 wait/waitpid ───► release_task() ──► PID 可回收
```

#### wait 族系统调用

| 调用 | 行为 |
|------|------|
| **`wait(&status)`** | 阻塞等 **任一** 子进程 |
| **`waitpid(pid, &status, opts)`** | 指定 PID；`WNOHANG` 非阻塞 |
| **`waitid`** | 更细粒度（POSIX） |

```c
int status;
pid_t p = waitpid(child, &status, 0);
if (WIFEXITED(status))
    code = WEXITSTATUS(status);
else if (WIFSIGNALED(status))
    sig  = WTERMSIG(status);
```

#### 僵尸 vs 孤儿

| 术语 | 条件 | 内核行为 | 用户可见 |
|------|------|----------|----------|
| **僵尸 ZOMBIE** | 子已退出，父 **未 wait** | 保留 `task_struct` | `ps` 显示 **Z**，占 PID |
| **孤儿 ORPHAN** | 父 **先死**，子仍运行 | **reparent** 给组内线程或 **init (PID 1)** | 父变 init/ppid 变化 |
| **init 收养** | 孤儿最终到 PID 1 | init **定期 wait** 收养子 | 避免永久僵尸 |

#### reparenting 简图

```
父 P 退出（子 S 仍在跑）
  S.ppid ──► 同组其他线程？ ──否──► init (1)
                                      │
                                 init wait 循环回收
```

#### 资源到底何时释放？

| 资源 | 释放时机 |
|------|----------|
| 用户内存、fd、映射 | **`do_exit()`** 内 |
| **`task_struct`、内核栈、PID** | 父 **`wait`** → **`release_task()`** |
| 子线程栈 | 线程 exit 路径；末线程触发组退出 |

#### SIGCHLD 与 SA_NOCLDWAIT

| 机制 | 效果 |
|------|------|
| 默认 | 子僵尸时父可收到 **`SIGCHLD`** |
| **`signal(SIGCHLD, SIG_IGN)`**（Linux） | 自动回收，不产生僵尸（语义依平台） |
| **`SA_NOCLDWAIT`** | 子退出立刻回收，父无需 wait |

**运维 / HFT：** 僵尸泛滥 → 父进程 bug（**未 wait** 或线程组设计错误）。长寿命交易 daemon 应在子进程退出路径 **`waitpid(..., WNOHANG)`** 或专用 sigchld 处理器回收；否则 PID 耗尽会导致 **无法 fork 新 worker**。`init` 会回收其收养子，但 **不能替代** 自己进程的回收责任。

→ [§3.1 进程概念](./section-3.1-进程的概念.md) · [§3.3 EXIT_ZOMBIE 状态](./section-3.3-进程状态.md) · [Ch 4 调度退出路径](../../chapter-04-process-scheduling/notes/section-4.5-抢占与上下文切换.md) · [07 TLPI Ch24/20 进程创建/信号](../../../../03-linux-userspace-api/chapter-24-process-creation/notes.md) · [01 CSAPP Ch8 僵尸/孤儿](../../../../02-computer-systems/chapter-08-exceptional-control-flow/)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 僵尸进程（zombie）是怎么产生的？如何避免？

<details><summary>答案</summary>

子进程 exit() 后内核释放内存/fd 但保留 task_struct（含退出状态）等父进程 wait()。如果父进程不 wait，子进程就变 zombie。避免方法：1) 父进程及时 wait/waitpid；2) 父进程注册 SIGCHLD handler 调 wait；3) 设置 SIGCHLD 为 SIG_IGN（内核自动回收）；4) 父进程忽略后子进程被 init(PID 1) 收养。

</details>

**Q2.** exit() 和 _exit() 的区别？哪个在内核中执行？

<details><summary>答案</summary>

_exit()` 直接执行 sys_exit 系统调用（内核态：释放资源、设置退出码、通知父进程）。`exit()` 是 libc 包装：先执行 atexit 注册函数 + flush stdio buffer + 调 _exit()。HFT 进程退出时如果需要保证日志 flush，用 exit()；如果 crash 路径想立即终止不 flush，用 _exit()。

</details>

</details>
---
