## 4. 信号的生成与传递

---

### 一、生成信号

| 函数 | 用途 |
|------|------|
| **`specific_send_sig_info()`** | 向 **特定** LWP 发送 |
| **`group_send_sig_info()`** | 向 **整个线程组** 发送 |

流程概要：

1. **权限检查**（能否向目标发信号）  
2. **`send_signal()`** — 加入对应 **挂起队列**  
3. 置位 **`TIF_SIGPENDING`** 等标志  

来源示例：用户 `kill()`、内核 `force_sig()`、缺页转 `SIGSEGV`、定时器 `SIGALRM`。

→ 异常转信号：[Ch 4 section-5](../../chapter-04-interrupts-and-exceptions/notes/section-5-异常处理.md)

---

### 二、传递时机

**典型路径：** 从 **内核态返回用户态之前**

```
中断 / 异常 / syscall 结束
    ↓
检查 TIF_SIGPENDING
    ↓ 是
do_signal() — 处理未阻塞的 pending 信号
    ↓
再 iret / sysexit 回用户态
```

→ 中断返回：[Ch 4 section-8](../../chapter-04-interrupts-and-exceptions/notes/section-8-中断返回.md)

---

### 三、默认动作 (`SIG_DFL`)

| 类别 | 示例信号 | 行为 |
|------|----------|------|
| **Terminate** | SIGTERM, SIGKILL | 终止进程 |
| **Dump** | SIGSEGV, SIGABRT | 终止 + core dump |
| **Ignore** | SIGCHLD（可配置） | 丢弃 |
| **Stop** | SIGSTOP, SIGTSTP | 暂停 |
| **Continue** | SIGCONT | 继续运行 |

`SIGKILL` / `SIGSTOP` **不可捕获、不可忽略**。

---

← [3. 数据结构](./section-3-信号数据结构.md) · 下一节 [5. 用户态 handler](./section-5-捕获与用户态处理.md)
