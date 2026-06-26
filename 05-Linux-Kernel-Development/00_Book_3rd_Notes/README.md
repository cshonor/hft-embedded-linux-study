# Linux Kernel Development 3rd — Robert Love

**00_Book_3rd_Notes** · 全书 **20 章** · [返回 05 总目录](../README.md) · [返回总清单](../../READING-LIST.md#2-linux-kernel-development--robert-love)

> **前置建议：** 先完成 [01 LFS](../01_Course_LFS/) → [02 内核编程 6 集](../02_Course_Kernel_7Lectures/) → [03 架构理论](../03_Course_Kernel_Architecture/)，再通读本书。见 [LEARNING-PATH.md](../LEARNING-PATH.md)。

📋 **完整目录与 HFT 读/跳标注** → [OUTLINE.md](./OUTLINE.md)

---

## 目录结构

```
chapter-XX-english-slug/
├── README.md      ← 章导读（本节结构、小结、Checklist、HFT 标签）
└── notes/         ← 按原书小节拆分的笔记
```

每章 `README.md` 含 **小节笔记索引表**；正文在 `notes/section-*.md`。

**全书 20 章笔记 ✓**（2025 迁移为与 [03-SysPerf](../../03-Systems-Performance-2nd/) 同构目录）

---

## 全书结构（20 章）

| 章 | 导读 |
|----|------|
| 1 Linux 内核简介 | [chapter-01-intro](./chapter-01-intro/) |
| 2 内核入门 | [chapter-02-getting-started](./chapter-02-getting-started/) |
| 3 进程管理 | [chapter-03-process-management](./chapter-03-process-management/) |
| 4 进程调度 | [chapter-04-process-scheduling](./chapter-04-process-scheduling/) |
| 5 系统调用 | [chapter-05-system-calls](./chapter-05-system-calls/) |
| 6 内核数据结构 | [chapter-06-kernel-data-structures](./chapter-06-kernel-data-structures/) |
| 7 中断和中断处理程序 | [chapter-07-interrupts](./chapter-07-interrupts/) |
| 8 下半部和推后执行的工作 | [chapter-08-bottom-halves](./chapter-08-bottom-halves/) |
| 9 内核同步介绍 | [chapter-09-kernel-sync-intro](./chapter-09-kernel-sync-intro/) |
| 10 内核同步方法 | [chapter-10-sync-methods](./chapter-10-sync-methods/) |
| 11 定时器和时间管理 | [chapter-11-timers](./chapter-11-timers/) |
| 12 内存管理 | [chapter-12-memory-management](./chapter-12-memory-management/) |
| 13 虚拟文件系统 | [chapter-13-vfs](./chapter-13-vfs/) |
| 14 块 I/O 层 | [chapter-14-block-io](./chapter-14-block-io/) |
| 15 进程地址空间 | [chapter-15-process-address-space](./chapter-15-process-address-space/) |
| 16 页高速缓存和页回写 | [chapter-16-page-cache](./chapter-16-page-cache/) |
| 17 设备与模块 | [chapter-17-devices-modules](./chapter-17-devices-modules/) |
| 18 调试 | [chapter-18-debugging](./chapter-18-debugging/) |
| 19 可移植性 | [chapter-19-portability](./chapter-19-portability/) |
| 20 补丁、开发和社区 | [chapter-20-patches-community](./chapter-20-patches-community/) |

---

## HFT 精读捷径

```
Ch 4 → Ch 7–8 → Ch 9–10 → Ch 11
```

选读补上下文：**Ch 3、12、15** · 内存深读 → [07-Linux-Virtual-Memory-Manager](../../07-Linux-Virtual-Memory-Manager/)

对照性能观测 → [03-Systems-Performance-2nd](../../03-Systems-Performance-2nd/) Ch 3 / 6–10
