## 1. 本章定位

> **ULK Ch 2 Memory Addressing** · 80x86 内存寻址 + Linux 的软件模型

---

### 一、本章讲什么

- **硬件侧：** 80x86 上逻辑地址 → 线性地址 → 物理地址 怎么转（分段、分页、MMU、TLB）
- **软件侧：** Linux 2.6 如何**简化分段**、用**通用四级分页**适配 32/64 位，以及启动时物理内存怎么布局

Ch 1 只提到「虚拟内存、按需分页」；**本章是地址翻译的物理基础**。

---

### 二、小节导航

| 节 | 主题 |
|----|------|
| [2](./section-2-three-address-types.md) | 三种地址、MMU 流水线 |
| [3](./section-3-segmentation.md) | GDT/LDT、Linux 四段策略 |
| [4](./section-4-hardware-paging.md) | 页目录/页表、PAE、TLB |
| [5](./section-5-linux-four-level-paging.md) | PGD/PUD/PMD/PT、折叠 |
| [6](./section-6-memory-layout-and-tlb.md) | 启动布局、用户/内核空间、TLB 刷新 |

---

### 三、在 Linux 链上的位置

```
Ch 2  内存寻址（本章）— 地址怎么翻译
Ch 8  内存管理       — 物理页框怎么分配
Ch 9  进程地址空间   — 每个进程怎么有自己的页表
Ch 17 页回收         — 内存不够怎么办
07 Gorman            — VM 专著（在 Ch 8–9 之后）
```

交叉：[01 CSAPP](../../../01-CSAPP-3rd/) Ch 9 · [09 MikanOS](../../../09-system-low-level-hands-on/02-mikan-os/) 分页实验

---

← [Ch 2 导读](../README.md) · 下一节 [2. 三种内存地址](./section-2-three-address-types.md)
