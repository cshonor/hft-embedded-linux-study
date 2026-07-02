# Ch 4 §3 内存区域 (Memory Regions · VMA)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

### 3. 内存区域 (Memory Regions · VMA)

进程 **很少用满** 整个用户地址空间，而是 **稀疏** 使用若干 **段** — 每段一个 **`vm_area_struct` (VMA)**。

### VMA 是什么

| 属性 | 含义 |
|------|------|
| **一段连续 VA 范围** `[vm_start, vm_end)` | 不与其它 VMA **重叠** |
| **统一属性** | 相同 **权限**（读/写/执行）、相同 **用途**（堆、栈、mmap 文件、匿名映射、共享库…） |
| **操作函数** | `vm_ops` — 文件映射的 **fault/read** 等回调 |

**典型 VMA：** 可执行文件 **text**、**heap**、**stack**、**mmap** 的订单簿/共享内存、**vdso/vsyscall** 页（2.6+）。

### 相关系统调用（内核入口在 `mm/mmap.c` 等）

| 调用 | 作用 | HFT |
|------|------|-----|
| **`mmap`** | **创建** 新 VMA / 映射文件或匿名内存 | 订单簿 **预分配 arena**、**共享内存** |
| **`mremap`** | **移动/扩大** 已映射区域 | 动态扩容堆式缓冲区 |
| **`mlock` / `mlockall`** | **锁定** 页面于物理内存 — **禁止换出** | **延迟敏感进程标配** |
| **`munmap`** | **删除** 映射、释放 VMA |  teardown 时释放 |

**注意：** `mmap` 成功 **≠** 立刻占用等量 **物理 RAM** — 常只是 **VMA + 页表「洞」**；物理页在 **fault** 时落下。

---
