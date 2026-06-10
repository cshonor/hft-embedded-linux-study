# 内核原理与架构 · 理论课笔记

**03_Course_Kernel_Architecture** · [返回 02 总目录](../README.md)

> **定位：** 前置预习 — 虚拟内存、网络栈、调度、并发锁的**理论地图**，对应 LKD 核心五大子系统。

## 学习顺序

建议在 [02 7讲实操](../02_Course_Kernel_7Lectures/) 之后，[00 LKD 第三版](../00_Book_3rd_Notes/) 之前。

→ 与书本对应：[LEARNING-PATH.md § 架构课](../LEARNING-PATH.md#3-内核原理架构课--五大子系统前置)

## 对应 LKD 第三版 / 延伸书目

| 本课主题 | 书本章节 | 延伸 |
|----------|----------|------|
| 虚拟内存、HugePage、NUMA、页表、缺页 | LKD Ch 12、Ch 15 | [03-Gorman](../../03-Linux-Virtual-Memory-Manager/) |
| 网络栈分层、UDP、NAPI/软中断 | LKD 网络概述 | [06-Rosen](../../06-Linux-Kernel-Networking/) |
| 调度器、RT 进程 | LKD Ch 4 | HFT 绑核 |
| spinlock / mutex / RCU | LKD Ch 9–10 | 无锁订单簿 |

## 笔记目录

<!-- 按课程模块补充 -->

| 模块 | 主题 | 笔记 | 状态 |
|------|------|------|------|
| VM | 虚拟内存 / NUMA / 大页 | — | 待补充 |
| Net | 内核网络栈 / NAPI | — | 待补充 |
| Sched | 调度 / RT | — | 待补充 |
| Sync | 锁 / RCU | — | 待补充 |

## HFT 关联

本课覆盖内容与 HFT 低延迟调优**高度重叠** — 是筛出 LKD 精读章节的最好前置。

→ [10-HFT ch05–08](../../10-HFT-Low-Latency-Practice/)
