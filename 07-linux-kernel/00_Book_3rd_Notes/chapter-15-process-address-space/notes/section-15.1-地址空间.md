## ① 地址空间 · Address Spaces

Linux 是 **虚拟内存 OS** — 每个进程看到 **独立的线性 VA 空间**；物理页通过 **页表** 映射，**MMU** 在访问时完成 **VA→PA**。

#### 核心概念

| 概念 | 说明 |
|------|------|
| **进程地址空间** | 该进程可寻址的 **全部 VA** + 实际映射的 **权限/ backing** |
| **平坦模型** | 32/64 位 **单一连续 VA** — 非分段时代 Intel 分段 |
| **隔离** | 进程 A **不能** 直接读写进程 B 的 VA（除共享映射） |

#### 典型内存布局（x86-64 用户部分，简化）

| 区域 | 内容 | 增长方向 |
|------|------|----------|
| **text** | 可执行 **代码** — 常 **R-X** | 固定 |
| **data** | **已初始化** 全局/静态 | 固定 |
| **bss** | **未初始化** 全局 — 常映射 **零页** | 固定 |
| **heap** | `brk` / `malloc` | **向上** |
| **mmap 区** | 共享库、文件映射、匿名大块 | **向 heap 方向** |
| **栈** | 局部变量、调用链 | **向下** |
| **vDSO / vvar** | 内核辅助 **快速 syscall / time** | 固定 |

```
高地址  0x7fff...
  ┌─────────────┐
  │ 栈           │  ↓ 增长
  ├─────────────┤
  │             │
  │ mmap 区      │  ← MAP_FIXED 策略缓冲常放此带
  │ 共享库 .so   │
  ├─────────────┤
  │ heap        │  ↑ brk/malloc
  ├─────────────┤
  │ bss / data  │
  ├─────────────┤
  │ text (ELF)  │
低地址  0x400000 附近
```

#### 内核如何描述

| 结构 | 作用 |
|------|------|
| **`mm_struct`** | 整个地址空间的 **头** |
| **`vm_area_struct`（VMA）** | 每一段 **[start, end)** 的 **属性 + 操作** |
| **页表** | **VA → PA** + 权限位 |

#### 与用户态 API 对应

| 用户操作 | 内核效果 |
|----------|----------|
| **`execve`** | 新建 text/data/bss/heap 布局 |
| **`malloc`** | 可能 **`brk`** 或 **匿名 mmap** |
| **`mmap` / `munmap`** | 增删 **VMA** + 建/拆 PTE |
| **`mprotect`** | 改 **VMA flags + PTE 权限** |

**HFT：** 低延迟栈 **标配**：

| 技术 | 目的 |
|------|------|
| **`mmap` 环形缓冲** | 订单/行情 **零拷贝** 跨线程 |
| **`MAP_SHARED`** | 多进程 **共享 state** |
| **`mlock` / `MAP_LOCKED`** | **禁止 swap** — 避免 **缺页尖刺** |
| **`MAP_HUGETLB` / `hugetlbfs`** | **2MB/1GB 大页** — **TLB miss ↓** |
| **`MAP_POPULATE`** | 启动时 **fault 完** — 盘中 **无 demand paging** |

→ [01 CSAPP Ch9 VM](../../../../02-computer-systems/chapter-09-virtual-memory/) · [06 Gorman Ch4 进程地址空间](../../../../09-linux-mm/chapter-04-process-address-space/) · [Ch 3 fork/COW](../../chapter-03-process-management/)

---
