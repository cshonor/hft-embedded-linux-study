## 4. 更新时间、日期与系统统计

> 定时器中断处理程序里的「每 tick 必做之事」

---

### 一、更新系统时间

- **`update_times()`** — 在 tick 中断路径中更新 **系统日期和时间**  
- 与 RTC 校准、NTP（用户态）等配合（本章偏内核 tick 路径）

---

### 二、更新进程 CPU 时间

**`update_process_times()`**：

| 字段 | 含义 |
|------|------|
| **`utime`** | 用户态累计 CPU 时间 |
| **`stime`** | 内核态累计 CPU 时间 |

还检查进程是否 **超出 CPU 时间限制** → 发送 **`SIGXCPU`** / **`SIGKILL`**。

→ `task_struct`：[Ch 3](../chapter-03-processes/notes/section-3-process-descriptor.md) · 信号：[Ch 11](../chapter-11-signals.md)

---

### 三、NMI 看门狗 (NMI Watchdog)

SMP 系统中：

- 利用 APIC 周期性产生 **NMI**（不可屏蔽中断）  
- 检测 CPU 是否 **冻结**（死锁、长时间关中断）  
- 若 CPU 无响应 → 记录错误、导出寄存器、**杀死当前进程**

生产环境 hang 排查时会见到 watchdog 相关日志。

---

← [3. 计时架构](./section-3-timekeeping-architecture.md) · 下一节 [5. 软件定时器与延迟](./section-5-software-timers-and-delays.md)
