## 1. 本章定位

> **《从零自制操作系统》Ch 27 应用的内存管理**

---

### 一、从 Ch19/24 到「高级 VM」

| Ch 19–24 | **Ch 27** |
|----------|-----------|
| **SetupPageMaps** — 加载时 **全映射 LOAD 段** | **Demand Paging** — 访问时才 **分配物理帧** |
| 每应用 **独立 PML4** | **CoW** — 多实例 **共享只读帧** |
| **sbrk** 简化（Ch25） | **sbrk + DemandPages** — **malloc 大堆** |
| 无 **mmap** | **MapFile** — 文件 **映 VA** |

**问题：** 64 位 **VA 巨大** — 若 **预分配全部** · **多开同 ELF** — **物理内存浪费**。

---

### 二、本章讲什么

| 主题 | 要点 |
|------|------|
| **HandlePageFault** | **#PF → 分配 · 填表** |
| **DemandPages()** | syscall **预留 VA** |
| **MapFile()** | **mmap 式** 读文件 |
| **memstat** | **free** 同类 |
| **CoW + invlpg** | 共享 **.text/.rodata** · 写时 **复制** |

---

### 三、核心思想

```
Page Fault 不是仅致命错误
    → 也可以是 OS 的「分配/加载/复制」触发器
```

→ [Ch24 KillApp vs PF](../chapter-24-multi-terminal/notes/section-5-用户态异常与KillApp.md) · [Ch20 #PF](../chapter-20-syscall/notes/section-4-异常处理与调试.md)

---

← [Ch 27 导读](../README.md) · 下一节 [2. Demand Paging](./section-2-按需分页与DemandPages.md)
