# 内核架构理论 · 分讲目录

> **Linux Internals & Architecture: The Complete Kernel Guide** · B站 **中英字幕视频教程** · [返回 README](./README.md)

| 标签 | 说明 |
|------|------|
| 🔴 | 基础必看 — 后续章节依赖 |
| 🟡 | 核心架构 |
| ⚪ | 进阶 · 有余力 |

## Part I · 基础与设计基因

| 讲 | 主题 | 笔记 | 标签 |
|----|------|------|------|
| a01 | Unix 设计基因 | [episode-a01](./episode-a01-Unix设计基因.md) | 🔴 |
| a02 | 宏内核 vs 微内核 | [episode-a02](./episode-a02-宏内核与微内核.md) | 🔴 |
| a03 | 内核架构总览 · **Kernel/Userland 分离** | [episode-a03](./episode-a03-内核架构总览.md) | 🔴 |
| a04 | 固件与引导流程 | [episode-a04](./episode-a04-固件与引导流程.md) | 🟡 |

## Part II · 现代硬件与执行模型

| 讲 | 主题 | 笔记 | 标签 |
|----|------|------|------|
| a05 | SMP 与 NUMA | [episode-a05](./episode-a05-SMP与NUMA.md) | 🔴 |
| a06 | 抢占模型 | [episode-a06](./episode-a06-抢占模型.md) | 🔴 |
| a07 | 同步策略 | [episode-a07](./episode-a07-同步策略.md) | 🔴 |

## Part III · 核心子系统（HFT 重点）

| 讲 | 主题 | 笔记 | 标签 | 延伸 |
|----|------|------|------|------|
| a08 | 虚拟内存架构 | [episode-a08](./episode-a08-虚拟内存架构.md) | 🔴 | [03-Gorman](../../06-Linux-Virtual-Memory-Manager/) |
| a09 | 进程与调度 | [episode-a09](./episode-a09-进程与调度.md) | 🔴 | LKD Ch 4 |
| a10 | 网络栈架构 | [episode-a10](./episode-a10-网络栈架构.md) | 🔴 | [06-Rosen](../../09-Linux-Kernel-Networking/) |

## 学习路径

```
a01–a03  设计基因 + 宏内核 + 总览（先搞懂「为什么」）
a04      引导（衔接 LFS p0/p13）
a05–a07  SMP/NUMA、抢占、同步
a08–a10  VM / 调度 / 网络 → 直接对接 HFT
```

## 与另两门课的分工

| | LFS | 内核编程 6 集 | **本课** |
|---|-----|--------------|----------|
| 导向 | 从 0 构建 | 写/调试代码 | **理论架构** |
| 典型问题 | rootfs 怎么装 | insmod 怎么用 | **为何用宏内核** |

→ 对照表：[CROSS-COURSE.md](./CROSS-COURSE.md)

## 前置

- 已完成或并行：[01 LFS](../01_Course_LFS/) · [02 内核编程](../02_Course_Kernel_7Lectures/)
- 之后进入：[00 LKD 第三版](../00_Book_3rd_Notes/)
