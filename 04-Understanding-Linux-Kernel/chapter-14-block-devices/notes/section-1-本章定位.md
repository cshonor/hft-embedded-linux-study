## 1. 本章定位

> **ULK Ch 14 Block Device Drivers** · 硬盘等 **块设备** 的 I/O 路径

---

### 一、本章讲什么

块设备（硬盘、软驱、CD-ROM …）与 CPU 速度 **差距极大** — 磁头 **寻道** 极慢。内核在 **多层抽象** 上合并、排序、缓存请求：

| 主题 | 要点 |
|------|------|
| **层次** | VFS → 映射层 → 通用块层 → I/O 调度 → 驱动 |
| **数据单位** | 扇区 / 块 / 段（DMA scatter-gather） |
| **核心结构** | `bio`、`gendisk`、`request_queue` |
| **调度** | Noop、CFQ、Deadline 电梯算法 |
| **完成** | DMA 结束 → 中断 → 释放 `bio`、派发下一请求 |

---

### 二、小节导航

| 节 | 主题 |
|----|------|
| [2](./section-2-块设备层次结构.md) | 五层 I/O 路径 |
| [3](./section-3-扇区块与段.md) | sector / block / segment |
| [4](./section-4-通用块层与bio.md) | `bio`、`gendisk`、分区 |
| [5](./section-5-IO调度程序.md) | `request_queue`、Noop/CFQ/Deadline |
| [6](./section-6-块设备驱动与中断.md) | `block_device`、初始化、IRQ 完成 |

---

### 三、在 Linux 链上的位置

```
Ch 12 VFS read/write
Ch 14 块层 + 调度（本章）
Ch 15 页高速缓存 — 多数读写在 RAM 完成，减少落盘
Ch 16 文件访问 — 完整 read 路径
Ch 13 DMA / 中断基础
```

HFT：交易热路径少碰 **旋转磁盘**；日志/快照落盘仍受 **调度与 fsync** 影响。NVMe 时代 elevator 已大幅简化（modern 对照）。

---

← [Ch 14 导读](../README.md) · 下一节 [2. 层次结构](./section-2-块设备层次结构.md)
