## 6. 与调度相关的系统调用

---

### 一、普通进程优先级

| 调用 | 作用 |
|------|------|
| **`nice()`** | 修改静态优先级（nice 值） |
| **`getpriority()`** | 获取优先级 |
| **`setpriority()`** | 设置进程/进程组优先级 |

→ 实现路径：[Ch 10](../chapter-10-system-calls.md) · 用户态：[08 TLPI](../../../06-The-Linux-Programming-Interface/)

---

### 二、CPU 亲和性

| 调用 | 作用 |
|------|------|
| **`sched_getaffinity()`** | 获取 **CPU affinity mask** |
| **`sched_setaffinity()`** | 限制进程 **只在指定 CPU** 上运行 |

HFT 标配：**交易线程绑 isolated CPU**，与 housekeeping 核分离。

→ [15 HFT 内核调优](../../../15-HFT-Low-Latency-Practice/)

---

### 三、实时调度

| 调用 | 作用 |
|------|------|
| **`sched_setscheduler()`** | 改为 **FIFO / RR** 等策略 |
| **`sched_setparam()`** | 设置 **实时优先级** |

需要特权：**`CAP_SYS_NICE`**（或 root）。

---

### 四、后续章节索引

| Ch 7 主题 | 继续读 |
|-----------|--------|
| 进程、切换 | [Ch 3 进程](../chapter-03-processes/) 🔴 |
| tick、时间片 | [Ch 6 定时测量](../chapter-06-timing/) 🟡 |
| 抢占、锁 | [Ch 5 内核同步](../chapter-05-kernel-synchronization/) 🔴 |
| syscall 路径 | [Ch 10 系统调用](../chapter-10-system-calls.md) 🔴 |
| 内存、COW | [Ch 8 内存管理](../chapter-08-memory-management.md) 🔴 |
| Modern CFS | [05 LKD Ch 4](../../../03-Linux-Kernel-Development/) |
| HFT 绑核/FIFO | [15 HFT 工程](../../../15-HFT-Low-Latency-Practice/) |

---

← [5. SMP 平衡](./section-5-SMP运行队列平衡.md) · 下一章 [Ch 8 内存管理](../chapter-08-memory-management.md)
