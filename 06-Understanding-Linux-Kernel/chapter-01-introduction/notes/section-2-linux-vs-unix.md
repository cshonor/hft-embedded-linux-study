## 2. Linux 与其他类 Unix 内核的比较

---

### 一、Linux 在 Unix 家族中的位置

- Linux 是 **Unix 类操作系统**大家族的一员。
- 最显著优势：**GPL 开源** — 任何人可获取、阅读、修改源代码（ULK 即基于此）。
- 从**实现**角度，ULK 以 **Linux 2.6** 为蓝本展开全书。

---

### 二、Linux 2.6 的两个技术亮点（Ch 1 强调）

| 特性 | 含义 | 后续深潜 |
|------|------|----------|
| **可抢占内核（Preemptive）** | 内核态也可被更高优先级任务打断，降低调度延迟 | [Ch 7 进程调度](../../chapter-07-process-scheduling.md) |
| **SMP 支持** | 对称多处理：多 CPU，每颗核可处理任意任务 | [Ch 5 内核同步](../../chapter-05-kernel-synchronization.md) · [Ch 7](../../chapter-07-process-scheduling.md) |

> **Modern 对照：** 5.x/6.x 调度器已演进为 CFS 等，**抢占与 SMP 概念不变**，实现细节需对照现网内核。

---

### 三、与其他 Unix 变体

Ch 1 会对比 Linux 与 BSD、System V 等在**内核设计取舍**上的差异（调度、内存、VFS 等）。读后续章时留意：

- **Unix 通用机制** — 进程、inode、syscall 等各家类似
- **Linux 特有实现** — 如 2.6 时代的 O(1) 调度、具体锁实现等

---

← [1. 系统全景图](./section-1-overview-and-architecture.md) · 下一节 [3. 基本操作系统概念](./section-3-basic-os-concepts.md)
