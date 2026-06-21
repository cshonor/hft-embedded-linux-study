# 内核原理与架构 · 理论课笔记

**03_Course_Kernel_Architecture** · [返回 02 总目录](../README.md)

> **来源：** B站 **中英字幕视频教程** · *Linux Internals & Architecture: The Complete Kernel Guide*  
> **定位：** 理论前置 — 回答 **「为什么 Linux 这样设计」**，不是教编译/写模块（那是 01 LFS + 02 编程课）

📋 **分讲目录** → [OUTLINE.md](./OUTLINE.md) · **学习清单** → [CHECKLIST.md](./CHECKLIST.md) · **三门课对照** → [CROSS-COURSE.md](./CROSS-COURSE.md)

---

## 学习顺序

```
01 LFS → 02 内核编程 6 集 → 03 本课程（a01–a10）→ 00 LKD 第三版
```

| 课程 | 回答 |
|------|------|
| LFS | 系统从 0 怎么拼 |
| 02 编程 | 怎么改/调试内核 |
| **本课** | **为什么这样设计** |

---

## 分讲速查

| Part | 讲 | 主题 |
|------|-----|------|
| I | a01–a04 | Unix DNA、宏/微内核、总览、引导 |
| II | a05–a07 | SMP/NUMA、抢占、同步 |
| III | a08–a10 | 虚拟内存、调度、网络栈 |

完整链接 → [OUTLINE.md](./OUTLINE.md)

**首讲已笔记：** [a01 Unix 设计基因](./episode-a01-Unix设计基因.md)（Introduction to Unix DNA）

---

## 与 LKD / HFT

| 本课 | 书本 | HFT |
|------|------|-----|
| a08 VM | LKD Ch 12/15 · [03-Gorman](../../06-Linux-Virtual-Memory-Manager/) | NUMA/大页 |
| a09 调度 | LKD Ch 4 | 绑核/RT |
| a07 同步 | LKD Ch 9–10 | RCU/无锁 |
| a10 网络 | [06-Rosen](../../09-Linux-Kernel-Networking/) | NAPI/softirq |

→ [10-HFT ch05–08](../../11-HFT-Low-Latency-Practice/)

---

## 前置

- Linux 命令行 + 基础 C
- 建议已完成 [02 e1–e2](../02_Course_Kernel_7Lectures/)（至少编译过内核、加载过模块）

## 配套

- 书本主线：[00_Book_3rd_Notes](../00_Book_3rd_Notes/)
- 三门课打通：[CROSS-COURSE.md](./CROSS-COURSE.md)
