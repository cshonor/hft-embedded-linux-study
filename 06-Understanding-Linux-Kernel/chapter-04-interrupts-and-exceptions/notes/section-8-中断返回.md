## 8. 从中断与异常中返回

> 返回用户态前，内核要做一系列 **关键检查**

---

### 一、返回路径

处理完毕后经：

- **`ret_from_intr()`** — 从中断返回  
- **`ret_from_exception()`** — 从异常返回  

---

### 二、返回前检查什么

| 检查项 | 后果 |
|--------|------|
| **Pending reschedule** | 是否需要 **进程调度**（`need_resched`） |
| **挂起的信号** | 是否投递给当前进程 |
| **单步调试** | 调试器相关状态 |

这就是为什么 **定时器中断** 能驱动调度 — tick 里可能置 `need_resched`，返回时切进程。

→ 深潜：[Ch 7 进程调度](../chapter-07-process-scheduling.md) · [Ch 11 信号](../chapter-11-signals.md)

---

### 三、后续章节索引

| Ch 4 主题 | 继续读 |
|-----------|--------|
| 锁、临界区、SMP | [Ch 5 内核同步](../chapter-05-kernel-synchronization.md) 🔴 |
| 调度、tick | [Ch 7 进程调度](../chapter-07-process-scheduling.md) 🔴 |
| `int 0x80` / syscall | [Ch 10 系统调用](../chapter-10-system-calls.md) 🔴 |
| 信号投递 | [Ch 11 信号](../chapter-11-signals.md) 🟡 |
| 设备驱动、IRQ 注册 | [Ch 13 I/O 架构](../chapter-13-io-architecture.md) ⚪ |
| 内核路径 profiling | [04 BPF](../../../04-BPF-Performance-Tools/) |
| 用户态绕过中断 | [14 DPDK](../../../14-DPDK-Low-Latency-Network/) |

---

← [7. 可延迟函数](./section-7-可延迟函数与工作队列.md) · 下一章 [Ch 5 内核同步](../chapter-05-kernel-synchronization.md)
