## 6. 系统调用重启与相关系统调用

---

### 一、信号打断可中断睡眠中的 syscall

进程在 **`TASK_INTERRUPTIBLE`** 等待 syscall 完成时若收到信号：

| 结果 | 说明 |
|------|------|
| 被 **唤醒** | 处理信号 |
| syscall **未完成** | 返回 **`ERESTARTSYS`**、**`-EINTR`** 等 |

用户态需检查返回值；libc 可能包装为 `-1` + `errno=EINTR`。

---

### 二、自动重新执行 syscall

若 **`SA_RESTART`** 且内核判定应重启：

- 修改 **寄存器状态**（如调整 EIP）  
- 返回用户态时 **重新触发** `int $0x80` / `sysenter`  
- 对应用层 **透明重启** — 无需手动重调  

未设 `SA_RESTART` 的 handler 返回后，syscall 以 **失败/中断** 结束。

→ syscall 入口：[Ch 10](../../chapter-10-system-calls/)

> **深潜可选：** `handle_signal()` 中 `ERESTARTSYS` 分支 — 见 `kernel/signal.c`。

---

### 三、与信号相关的系统调用

| 系统调用 | 作用 |
|----------|------|
| **`kill()`** | 向进程/线程组发信号 |
| **`sigaction()`** | 改变信号关联动作（handler、flags、`SA_RESTART`） |
| **`sigpending()`** | 查询挂起信号 |
| **`sigprocmask()`** | 修改阻塞信号掩码 |
| **`sigsuspend()`** | 原子地改掩码并挂起等待信号 |
| **`rt_sig*()`** | 实时信号对应接口 |

→ 用户态详述：[08 TLPI Ch 20–21](../../../06-The-Linux-Programming-Interface/) · syscall 路径：[Ch 10](../chapter-10-system-calls/)

---

### 四、本章小结

```
generation → pending 队列 → TIF_SIGPENDING
    ↓ 返回用户态前
do_signal → DFL 或 setup_rt_frame
    ↓ handler 结束
rt_sigreturn → 恢复原上下文
    ↓ 可选
ERESTARTSYS → 重入 syscall
```

---

### 五、后续章节索引

| Ch 11 主题 | 继续读 |
|------------|--------|
| syscall 返回检查 | [Ch 10 进入与退出](../chapter-10-system-calls/notes/section-4-进入与退出.md) 🔴 |
| 线程组 / task_struct | [Ch 3 进程](../chapter-03-processes/) 🔴 |
| 异常 → SIGSEGV | [Ch 4 异常处理](../chapter-04-interrupts-and-exceptions/notes/section-5-异常处理.md) |
| SIGALRM / 定时器 | [Ch 6 定时](../chapter-06-timing/) 🟡 |
| 子进程 SIGCHLD | [Ch 3 创建与销毁](../chapter-03-processes/notes/section-6-创建与销毁.md) |
| 其他 IPC | [Ch 19 进程通信](../chapter-19-ipc.md) ⚪ |
| 用户态编程 | [08 TLPI Ch 20–21](../../../06-The-Linux-Programming-Interface/) |

---

← [5. 用户态 handler](./section-5-捕获与用户态处理.md) · 下一章 [Ch 12 VFS](../chapter-12-VFS/)
