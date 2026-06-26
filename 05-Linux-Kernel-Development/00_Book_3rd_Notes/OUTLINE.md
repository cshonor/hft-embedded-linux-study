# LKD 3rd — 全书目录（20 章）

> **Linux Kernel Development 3rd** · Robert Love

| 标签 | HFT 读法 |
|------|----------|
| 🔴 | 精读 |
| 🟡 | 选读 |
| ⚪ | 跳过 |

| 章 | 英文 | 笔记 | HFT |
|----|------|------|-----|
| 1 | Introduction to the Linux Kernel | [chapter-01-intro](./chapter-01-intro/) | 🟡 |
| 2 | Getting Started with the Kernel | [chapter-02-getting-started](./chapter-02-getting-started/) | ⚪ |
| 3 | Process Management | [chapter-03-process-management](./chapter-03-process-management/) | 🟡 |
| 4 | Process Scheduling | [chapter-04-process-scheduling](./chapter-04-process-scheduling/) | 🔴 |
| 5 | System Calls | [chapter-05-system-calls](./chapter-05-system-calls/) | ⚪ |
| 6 | Kernel Data Structures | [chapter-06-kernel-data-structures](./chapter-06-kernel-data-structures/) | 🟡 |
| 7 | Interrupts and Interrupt Handlers | [chapter-07-interrupts](./chapter-07-interrupts/) | 🔴 |
| 8 | Bottom Halves and Deferring Work | [chapter-08-bottom-halves](./chapter-08-bottom-halves/) | 🔴 |
| 9 | An Introduction to Kernel Synchronization | [chapter-09-kernel-sync-intro](./chapter-09-kernel-sync-intro/) | 🔴 |
| 10 | Kernel Synchronization Methods | [chapter-10-sync-methods](./chapter-10-sync-methods/) | 🔴 |
| 11 | Timers and Time Management | [chapter-11-timers](./chapter-11-timers/) | 🔴 |
| 12 | Memory Management | [chapter-12-memory-management](./chapter-12-memory-management/) | 🟡 |
| 13 | The Virtual Filesystem | [chapter-13-vfs](./chapter-13-vfs/) | ⚪ |
| 16 | The Block I/O Layer | [chapter-14-block-io](./chapter-14-block-io/) | ⚪ |
| 16 | The Process Address Space | [chapter-15-process-address-space](./chapter-15-process-address-space/) | 🟡 |
| 16 | The Page Cache and Page Writeback | [chapter-16-page-cache](./chapter-16-page-cache/) | ⚪ |
| 17 | Devices and Modules | [chapter-17-devices-modules](./chapter-17-devices-modules/) | ⚪ |
| 18 | Debugging | [chapter-18-debugging](./chapter-18-debugging/) | 🟡 |
| 19 | Portability | [chapter-19-portability](./chapter-19-portability/) | ⚪ |
| 20 | Patches, Hacking, and the Community | [chapter-20-patches-community](./chapter-20-patches-community/) | ⚪ |

---

## HFT 精读顺序

```
Ch 4  进程调度（CFS / RT / affinity）
Ch 7–8 中断 / softirq / workqueue
Ch 9–10 同步（spinlock / mutex / barriers）
Ch 11 定时器 / jiffies / tick
Ch 3、12、15 选读补上下文
```

→ [07-Linux-Virtual-Memory-Manager](../../07-Linux-Virtual-Memory-Manager/) 深读内存

完整路线 → [HFT-READING-ROADMAP.md](../../HFT-READING-ROADMAP.md)
