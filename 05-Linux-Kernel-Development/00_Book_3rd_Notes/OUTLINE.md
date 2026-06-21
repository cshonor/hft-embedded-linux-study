# LKD 3rd — 全书目录（20 章）

> **Linux Kernel Development 3rd** · Robert Love

| 标签 | HFT 读法 |
|------|----------|
| 🔴 | 精读 |
| 🟡 | 选读 |
| ⚪ | 跳过 |

| 章 | 英文 | 笔记 | HFT |
|----|------|------|-----|
| 1 | Introduction to the Linux Kernel | [chapter-01](./chapter-01-Linux内核简介.md) | 🟡 |
| 2 | Getting Started with the Kernel | [chapter-02](./chapter-02-内核入门.md) | ⚪ |
| 3 | Process Management | [chapter-03](./chapter-03-进程管理.md) | 🟡 |
| 4 | Process Scheduling | [chapter-04](./chapter-04-进程调度.md) | 🔴 |
| 5 | System Calls | [chapter-05](./chapter-05-系统调用.md) | ⚪ |
| 6 | Kernel Data Structures | [chapter-06](./chapter-06-内核数据结构.md) | 🟡 |
| 7 | Interrupts and Interrupt Handlers | [chapter-07](./chapter-07-中断和中断处理程序.md) | 🔴 |
| 8 | Bottom Halves and Deferring Work | [chapter-08](./chapter-08-下半部和推后执行的工作.md) | 🔴 |
| 9 | An Introduction to Kernel Synchronization | [chapter-09](./chapter-09-内核同步介绍.md) | 🔴 |
| 10 | Kernel Synchronization Methods | [chapter-10](./chapter-10-内核同步方法.md) | 🔴 |
| 11 | Timers and Time Management | [chapter-11](./chapter-11-定时器和时间管理.md) | 🔴 |
| 12 | Memory Management | [chapter-12](./chapter-12-内存管理.md) | 🟡 |
| 13 | The Virtual Filesystem | [chapter-13](./chapter-13-虚拟文件系统.md) | ⚪ |
| 14 | The Block I/O Layer | [chapter-14](./chapter-14-块IO层.md) | ⚪ |
| 15 | The Process Address Space | [chapter-15](./chapter-15-进程地址空间.md) | 🟡 |
| 16 | The Page Cache and Page Writeback | [chapter-16](./chapter-16-页高速缓存和页回写.md) | ⚪ |
| 17 | Devices and Modules | [chapter-17](./chapter-17-设备与模块.md) | ⚪ |
| 18 | Debugging | [chapter-18](./chapter-18-调试.md) | 🟡 |
| 19 | Portability | [chapter-19](./chapter-19-可移植性.md) | ⚪ |
| 20 | Patches, Hacking, and the Community | [chapter-20](./chapter-20-补丁开发和社区.md) | ⚪ |

---

## HFT 精读顺序

```
Ch 4  进程调度（CFS / RT / affinity）
Ch 7–8 中断 / softirq / workqueue
Ch 9–10 同步（spinlock / RCU）
Ch 11 定时器 / hrtimer
Ch 3、12、15 选读补上下文
```

→ [06-Linux-Virtual-Memory-Manager](../../06-Linux-Virtual-Memory-Manager/) 深读内存

完整路线 → [HFT-READING-ROADMAP.md](../../HFT-READING-ROADMAP.md)
