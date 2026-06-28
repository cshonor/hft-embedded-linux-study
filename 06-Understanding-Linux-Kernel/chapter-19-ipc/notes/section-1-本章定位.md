## 1. 本章定位

> **ULK Ch 19 Process Communication** · **用户态** 进程如何同步与交换数据

---

### 一、本章讲什么

焦点从 **内核同步**（Ch 5）转到 **用户进程间** IPC：

| 机制 | 典型场景 |
|------|----------|
| **管道 / FIFO** | 生产者-消费者、shell 管道 |
| **SysV 信号量** | 多进程临界区（数组 + 原子 semop） |
| **SysV 消息队列** | 带类型的异步消息 |
| **SysV 共享内存** | **零拷贝** 大数据共享 |
| **POSIX mqueue** | 现代消息队列 + 优先级 |

---

### 二、小节导航

| 节 | 主题 |
|----|------|
| [2](./section-2-管道与FIFO.md) | pipe、16 缓冲环形数组、原子写 |
| [3](./section-3-System-V-IPC基础.md) | IPC key、identifier、kern_ipc_perm |
| [4](./section-4-IPC信号量.md) | semop、SEM_UNDO |
| [5](./section-5-IPC消息队列.md) | msg_msg、类型标签 |
| [6](./section-6-共享内存与POSIX消息队列.md) | shmat、shmem、swap；mqueue FS |

---

### 三、在 Linux 链上的位置

```
Ch 5  内核 spinlock / 内核信号量（≠ 本章 SysV sem）
Ch 9  mmap、缺页、VMA
Ch 11 信号（POSIX mq 异步通知）
Ch 17 swap（IPC 共享内存换出）
Ch 19 IPC（本章）
Ch 20 程序执行
08 TLPI  用户态 IPC API
```

HFT：**共享内存 + 无锁环** 是跨进程/跨组件热路径常见选型；SysV 较老，POSIX shm/mmap 更常见。

---

← [Ch 19 导读](../README.md) · 下一节 [2. 管道](./section-2-管道与FIFO.md)
