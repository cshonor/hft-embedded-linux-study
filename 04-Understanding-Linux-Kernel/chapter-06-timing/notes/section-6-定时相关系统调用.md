## 6. 定时测量相关的系统调用

---

### 一、获取与设置时间

| 调用 | 说明 |
|------|------|
| **`gettimeofday()`** | 读当前 wall-clock 时间（2.6 经典） |
| **`settimeofday()`** | 设置系统时间（需权限） |
| **`time()`** | 秒级时间戳 |
| **`stime()`** | 已废弃的设时接口 |

→ 内核路径：[Ch 10](../chapter-10-system-calls.md) · 用户态：[08 TLPI](../../../06-The-Linux-Programming-Interface/)

---

### 二、间隔定时器

| 调用 | 说明 |
|------|------|
| **`setitimer()`** | 设置 **间隔定时器**，到期发信号（如 **`SIGALRM`**） |
| **`alarm()`** | 简化版 — N 秒后 `SIGALRM` |

→ 信号：[Ch 11](../chapter-11-signals.md)

---

### 三、POSIX 定时器（更高级）

面向多线程 / 实时应用：

- **`clock_gettime()`** — 指定时钟源（CLOCK_REALTIME / MONOTONIC 等）  
- **`timer_create()`** / **`timer_settime()`** — 线程级定时器  

Modern 用户态代码应优先 POSIX API，而非仅 `gettimeofday`。

---

### 四、后续章节索引

| Ch 6 主题 | 继续读 |
|-----------|--------|
| tick 驱动调度 | [Ch 7 进程调度](../chapter-07-process-scheduling.md) 🔴 |
| 定时器中断 | [Ch 4 中断与异常](../chapter-04-interrupts-and-exceptions/) 🔴 |
| syscall 实现 | [Ch 10 系统调用](../chapter-10-system-calls.md) 🔴 |
| 信号 SIGALRM | [Ch 11 信号](../chapter-11-signals.md) 🟡 |
| 用户态时间 API | [08 TLPI](../../../06-The-Linux-Programming-Interface/) |

---

← [5. 软件定时器](./section-5-软件定时器与延迟函数.md) · 下一章 [Ch 7 进程调度](../chapter-07-process-scheduling.md)
