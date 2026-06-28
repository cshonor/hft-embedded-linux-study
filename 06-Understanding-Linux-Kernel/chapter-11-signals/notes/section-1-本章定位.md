## 1. 本章定位

> **ULK Ch 11 Signals** · Unix 最古老的 **异步通知** 机制

---

### 一、本章讲什么

信号是发给进程的 **极短消息**（通常仅一个编号），用于：

- **进程间通信**（如 `kill`）  
- **通知系统事件**（如 `SIGSEGV`、`SIGCHLD`、`SIGALRM`）  

内核要在 **内核态 ↔ 用户态** 之间安全完成 **生成 → 挂起 → 传递 → 恢复**，并与 **系统调用返回路径** 紧密耦合。

---

### 二、小节导航

| 节 | 主题 |
|----|------|
| [2](./section-2-信号作用与生命周期.md) | 常规 vs 实时信号、生成/传递、pending |
| [3](./section-3-信号数据结构.md) | `signal_struct`、`sighand_struct`、挂起队列 |
| [4](./section-4-生成与传递.md) | `send_signal`、`TIF_SIGPENDING`、`do_signal` |
| [5](./section-5-捕获与用户态处理.md) | `setup_frame`、`sigreturn` |
| [6](./section-6-系统调用重启与相关syscall.md) | `ERESTARTSYS`、kill/sigaction 等 |

---

### 三、在 Linux 链上的位置

```
Ch 3  线程组 / task_struct
Ch 4  异常（部分转为信号，如 SIGSEGV）
Ch 10 syscall 返回前检查 TIF_SIGPENDING
Ch 11 信号（本章）
Ch 19 IPC（更丰富的进程间通信）
08 TLPI  用户态 signal API
```

HFT：**信号处理引入不可预测延迟** — 热路径常 `sigprocmask` 阻塞或 `SA_RESTART` 谨慎使用。

---

← [Ch 11 导读](../README.md) · 下一节 [2. 生命周期](./section-2-信号作用与生命周期.md)
