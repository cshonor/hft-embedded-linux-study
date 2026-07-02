## 1. 本章定位

> **《从零自制操作系统》Ch 8 内存管理**

---

### 一、本章核心目标

| 之前 | **Ch 8 之后** |
|------|---------------|
| 内存由 **UEFI Boot Services** 临时分配 | OS **自己管理物理页** |
| `AllocatePages(gBS->…)` | **`BitmapMemoryManager::Allocate()`** |
| 栈/GDT/页表可能在 **UEFI 保留区** | 迁到 **OS 可控 RAM** |

**本章问题：** 如何 **安全脱离 UEFI 内存环境**，并 **按页动态分配**？

---

### 二、本章讲什么

| 主题 | 要点 |
|------|------|
| **Memory Map** | 解析 Ch2 **memmap** / Runtime 传递的映射 |
| **迁移** | 栈 · **GDT** · **页表** |
| **GDT** | **`lgdt`** · **CS/SS** |
| **分页** | **4 级** · **Identity Map** · **CR3** |
| **位图分配器** | **4KiB 页帧** · **first fit** |

---

### 三、在全书中的位置

```
Ch2  导出 memmap CSV
Ch7  中断 + FIFO（仍用 UEFI 栈/内存）
    ↓
Ch8  物理页分配 + 身份映射  ← 本章（🔴）
    ↓
Ch9  图层（可 Allocate 缓冲）
Ch19 进程虚拟地址空间（分页深化）
Ch27 应用堆管理
```

→ [Ch2 GetMemoryMap](../chapter-02-edk2-memmap/notes/section-4-GetMemoryMap与导出memmap.md)

---

← [Ch 8 导读](../README.md) · 下一节 [2. Memory Map](./section-2-解析UEFI内存映射.md)
