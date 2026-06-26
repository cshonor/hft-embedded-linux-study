# ULK · 学习计划

## 为何增补 ULK

| 书 | 回答的问题 |
|----|------------|
| **05 LKD** | 调度器、中断、同步 **负责什么**？现代内核 **怎么用**？ |
| **06 ULK** | `task_struct`、runqueue、页表、syscall **在代码里长什么样**？ |
| **07 Gorman** | VM **为什么这样设计**？NUMA/slab/回收 **专深** |

ULK 基于 **2.6** — 读时 **概念优先**，结构体名字以你机器上的 `include/linux/*.h` 为准对照。

---

## 推荐精读顺序（HFT）

1. **Ch 2** 内存寻址 · 分段/分页硬件  
2. **Ch 4–5** 中断 + 同步（spinlock 路径）  
3. **Ch 7** 调度 — 与 LKD Ch 4 / `SCHED_FIFO` 对照  
4. **Ch 8–9** 内核 MM + 进程地址空间 → 接 Gorman  
5. **Ch 10** 系统调用路径 → 接 TLPI  
6. **Ch 19** IPC 概要 → 接远期 IPC 实战模块  

**跳过：** Ch 12–18 文件系统栈（除非做持久化日志架构）。

---

## 与实操的配合

| 实操 | 配合 ULK 章 |
|------|-------------|
| [05 LFS](../05-Linux-Kernel-Development/01_Course_LFS/) | 附录 A 启动 |
| [LKD Ch 7–8](../05-Linux-Kernel-Development/00_Book_3rd_Notes/chapter-07-interrupts/) | Ch 4–5 中断/同步 |
| [02 MikanOS](../09-system-low-level-hands-on/02-mikan-os/) Ch 19–20 | Ch 2/9/10 分页与 syscall |
| [04 BPF](../04-BPF-Performance-Tools/) | Ch 4/7 内核路径理解 |

---

## 版本对照提示

| ULK 2.6 概念 | Modern 5.x/6.x |
|--------------|----------------|
| O(1) 调度器 | **CFS** + 类 deadline |
| 大内核锁 | **细粒度锁**、RCU 更广泛 |
| 部分 syscall 表 | **可能增删** — 以 ftrace/tracepoint 验证 |

笔记中标注 **「2.6 → modern」** 差异即可，不必全书更新到 6.x。
