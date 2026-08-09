# 31.5 `ioctl` 函数

> [Ch 17 ioctl](../../3_DeepMaster/Chapter17_Ioctl_Operate/study.md) · [Ch 25 信号驱动 I/O](../../3_DeepMaster/Chapter25_SignalDriveIO/study.md)

---

## 流的动态装配

STREAMS 下 **`ioctl`** 任务远比普通套接字繁重 — **组装/拆卸** 流栈。

| 命令 | 作用 |
|------|------|
| **`I_PUSH`** | 将模块压入流头**下方**（如 `bufmod` 缓冲） |
| **`I_POP`** | 弹出栈顶模块 |
| **`I_SETSIG`** | 流事件（可读、可写、高优先级消息等）→ **`SIGPOLL`**（同 **`SIGIO`**） |

---

## 与 Ch 25

**`I_SETSIG` + SIGPOLL** = System V 上**信号驱动 I/O** 的核心（Berkeley 用 `fcntl` + `SIGIO`）。

---

## 个人学习总结

（待填）
