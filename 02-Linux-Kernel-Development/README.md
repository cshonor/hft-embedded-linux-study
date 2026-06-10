# Linux Kernel Development — 内核开发总目录

**文件夹 02** · 书本主线 + 三门前置课程 · [返回总清单](../READING-LIST.md#2-linux-kernel-development--robert-love)

> **归类逻辑：** 三门视频课是《Linux Kernel Development 第三版》的**前置预习**；书本是**主线核心**。全部归属「Linux 内核开发」大主题，检索与复习统一入口。

---

## 子目录

| 序号 | 文件夹 | 内容 | 与书本关系 |
|------|--------|------|-----------|
| 00 | [00_Book_3rd_Notes](./00_Book_3rd_Notes/) | Love · LKD 第三版 · 20 章 | **主线** — 体系化梳理、查漏补缺 |
| 01 | [01_Course_LFS](./01_Course_LFS/) | LFS 从零构建系统 | 前置：编译上下文、用户态/内核态边界 |
| 02 | [02_Course_Kernel_7Lectures](./02_Course_Kernel_7Lectures/) | 内核模块/驱动 · 7 讲实操 | 前置：LKM、中断、Oops/KGDB |
| 03 | [03_Course_Kernel_Architecture](./03_Course_Kernel_Architecture/) | 内核原理与架构理论课 | 前置：VM、网络栈、调度、并发锁 |

📋 课程 ↔ 书本阅读关系 → [LEARNING-PATH.md](./LEARNING-PATH.md)

---

## 推荐学习顺序

```
01 LFS
  ↓
02 内核模块 7 讲（实操）
  ↓
03 内核原理架构（理论）
  ↓
00 LKD 第三版（通读 + 补全书本独有细节）
```

**正确节奏：** 三门课建立感性认知 → 书本系统化串联 → 把书本新增内容补回 `00_Book_3rd_Notes`。

---

## HFT 精读捷径（读书阶段）

```
Ch 4 调度 → Ch 7–8 中断/下半部 → Ch 9–10 同步 → Ch 11 定时器
```

内存深读 → [03-Gorman](../03-Linux-Virtual-Memory-Manager/) · 网络栈 → [06-Rosen](../06-Linux-Kernel-Networking/)

完整 HFT 路线 → [HFT-READING-ROADMAP.md](../HFT-READING-ROADMAP.md)
