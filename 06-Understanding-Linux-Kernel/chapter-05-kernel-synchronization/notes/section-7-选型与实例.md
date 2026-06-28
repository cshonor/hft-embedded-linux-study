## 7. 同步选型原则与内核实例

---

### 一、基本原则

**尽可能保持高并发** — 锁粒度越细、读写分离越好，但复杂度越高。

按 **谁访问数据结构** 选型：

| 访问者 | 典型保护 |
|--------|----------|
| **仅异常路径**（如 syscall） | **信号量**（可睡眠） |
| **异常 + 中断** | **local_irq_disable + 自旋锁** |
| **异常 + 中断 + 其他 CPU** | **自旋锁**（+ 关中断若 ISR 也访问） |
| **读多写少** | RW 锁、seqlock、**RCU** |
| **每 CPU 私有** | **per-CPU 变量**，无锁 |

还要区分访问来自：[Ch 4 异常 / 中断 / 可延迟函数](../chapter-04-interrupts-and-exceptions/)

---

### 二、内核中的实际实例

| 实例 | 机制 | 保护什么 |
|------|------|----------|
| **引用计数器** | **原子操作** | 资源 alloc/free 竞态 |
| **大内核锁 (BKL)** | 粗粒度信号量/自旋锁 | 2.6 早期 **整个内核**（已废弃） |
| | 特殊：`schedule()` 时 **自动释放**，返回时再取 | |
| **内存描述符** | **读/写信号量** | 进程间 `mm_struct` 并发 |
| **Slab 缓存列表** | 信号量 | 分配器链表 |
| **索引节点 (Inode)** | 信号量 | 目录/文件元数据并发 |

Modern 内核：**BKL 已移除**，锁更细粒度；读 ULK 时理解 **设计思想**，不必照搬 BKL。

---

### 三、后续章节索引

| Ch 5 主题 | 继续读 |
|-----------|--------|
| 调度、抢占点 | [Ch 7 进程调度](../chapter-07-process-scheduling.md) 🔴 |
| 控制路径、ISR | [Ch 4 中断与异常](../chapter-04-interrupts-and-exceptions/) 🔴 |
| 睡眠、唤醒 | [Ch 3 等待队列](../chapter-03-processes/notes/section-4-组织与查找.md) |
| Slab 分配 | [Ch 8 内存管理](../chapter-08-memory-management.md) 🔴 |
| VFS inode | [Ch 12 VFS](../chapter-12-VFS.md) ⚪ |
| LKD 现代锁 API | [05 LKD Ch 9–10](../../../05-Linux-Kernel-Development/) |

---

← [6. 信号量与完成变量](./section-6-信号量与完成变量.md) · 下一章 [Ch 6 计时](../chapter-06-timing.md)
