# 三门前置课 · 知识对照

> [03_Course_Kernel_Architecture](./README.md) · 打通 **LFS / 内核编程 / 架构理论** 三条线

---

## 定位对照

| 课程 | 文件夹 | 回答的问题 | 导向 |
|------|--------|-----------|------|
| LFS | [01_Course_LFS](../01_Course_LFS/) | 系统**从 0 怎么拼出来** | 动手 · 整机构建 |
| 内核编程 | [02_Course_Kernel_7Lectures](../02_Course_Kernel_7Lectures/) | **怎么改、调试**内核 | 动手 · LKM/驱动/GDB |
| **架构指南** | **本文件夹** | **为什么这样设计** | 理论 · 设计思想 |

推荐顺序：`01 LFS → 02 编程 → 03 架构 → 00 LKD 书本`

---

## Unix DNA 与另两门课的关联

### vs LFS（加加zero）

| Unix/Linux 原则 | LFS 中的「亲手看见」 |
|-----------------|---------------------|
| 可移植 / 模块化 | 自编译工具链、分阶段装包，每步只做一件事 |
| 一切皆文件 | `/dev` 节点、`proc`/`sys` 挂载（chroot 后创建） |
| 多用户/多任务 | 装完系统后多用户配置（p10 系统配置） |
| 内核 + 用户态分界 | p11–p12 编译内核 vs p6–p9 用户态 GNU 工具 |

→ LFS 让你**摸得到** Unix 哲学落地的产物；架构课解释**为何**要这样拆。

### vs 内核编程（码农的洋墨水）

| Unix/Linux 原则 | 内核编程课中的体现 |
|-----------------|-------------------|
| 模块化 | e2 独立 LKM：加载/卸载而不改 monolith 主镜像 |
| 简单接口 | e2 `module_init`/`module_exit`；e4 字符设备 `file_operations` |
| 多任务 | e1 编译支持 SMP 的内核；进程/线程由调度器管理（架构课 a09 深讲） |
| 调试文化 | e6 GDB/QEMU — 开源工具链排障 |

→ 编程课是**改内核**；Unix DNA 解释**进程/文件/模块**这些 API 的历史由来。

### vs LKD 第三版（书本主线）

| 架构课概念 | 书本章节 |
|-----------|----------|
| Unix 进程模型 | Ch 3 进程管理、Ch 5 系统调用 |
| 宏内核（a02） | Ch 1 简介 |
| SMP/NUMA（a05） | Ch 4 调度、Ch 12 内存 |
| 抢占/同步（a06–a07） | Ch 4、Ch 9–10 |
| VM（a08） | Ch 12、Ch 15 |
| 网络栈（a10） | 概述 + [06-Rosen](../../06-Linux-Kernel-Networking/) |

---

## HFT 视角：三条线汇到热路径

```
Unix DNA（模块化、少抽象）
    ↓
LFS：最小系统 + 内核编译直觉
    ↓
内核编程：LKM/调试手感
    ↓
架构课：NUMA / 抢占 / NAPI / RCU 为何存在
    ↓
LKD + Gorman + Rosen：精读 + 绑核/排抖动
```

| HFT 问题 | 先哪门课建立直觉 |
|----------|-----------------|
| 为什么要 isolcpus / RT | 03 调度 + 抢占 |
| 为什么 softirq 抖 | 03 网络栈 + 02 中断上下文 |
| 为什么 NUMA 绑内存 | 03 VM + 01 LFS 装系统时见过 numactl |
| 为什么 DPDK 旁路内核 | 03 宏内核路径 vs [12-DPDK](../../12-DPDK-Low-Latency-Network/) |

→ [CROSS-MODULE-GUIDE.md](../../CROSS-MODULE-GUIDE.md)
