# Linux Kernel Development — 内核开发总目录

**文件夹 07** · LKD 第三版笔记 · [返回总清单](../READING-LIST.md#2-linux-kernel-development--robert-love)

> **书本是主线核心。** 体系化梳理内核机制，查漏补缺。  
> 内核编译/rootfs 实操 → [P3.5 BusyBox 极简 Linux](../projects/P3.5-busybox-minimal-linux/)

---

## 子目录

| 序号 | 文件夹 | 内容 |
|------|--------|------|
| 00 | [00_Book_3rd_Notes](./00_Book_3rd_Notes/) | Love · LKD 第三版 · 20 章 — **主线** |

🗺️ 源码顶层目录 ↔ LKD 章节 → [Ch2 §2.2](./00_Book_3rd_Notes/chapter-02-getting-started/notes/section-2.2-内核源码树.md)

---

## 推荐学习顺序

```
P3.5 BusyBox 极简 Linux（编译内核 + rootfs + 启动链实操）
  ↓
通读 LKD 第三版（在 00_Book_3rd_Notes 补笔记）
```

**正确节奏：** P3.5 建立感性认知 → 书本系统化串联。

---

## HFT 精读捷径（读书阶段）

```
Ch 4 调度 → Ch 7–8 中断/下半部 → Ch 9–10 同步 → Ch 11 定时器
```

内存深读 → [03-Gorman](../09-linux-mm/) · 网络栈 → [06-Rosen](../17-kernel-networking/)

完整 HFT 路线 → [HFT-READING-ROADMAP.md](../HFT-READING-ROADMAP.md)
