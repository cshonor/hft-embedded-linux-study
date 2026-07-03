## 3.5 与动态库 mmap 的区别 · 自检

> **§3 子笔记 5/5** · [§3 索引](./section-3-主存储器与内存映射.md)

---

### 和动态库 mmap / 虚拟内存的区别（防混）

| 阶段 | 你在学什么 |
|------|------------|
| **Ch 2（本章）** | **物理 RAM 布局真相** — 固件交底 |
| **Ch 8+** | 在 **可用物理区** 里做 **线性 / 物理页分配** |
| **Ch 19** | **分页** — 虚拟地址 ↔ 物理页（那时才有「进程地址空间」） |
| **Linux mmap** | 用户态 **虚拟内存** 映射 — **远在** 物理摸底之后 |

```
Ch 2  memmap CSV     — 「物理世界真相」快照
  ↓
Ch 8  物理内存管理   — 只在 Conventional 区内分配
  ↓
Ch 19 分页           — 每个进程看到的虚拟地址
```

→ 对照 Linux **`/proc/iomem`**、x86 **e820 表**（Legacy BIOS 传递的同类信息）  
→ 入门澄清：[3.1 内存映射指什么](./section-3-1-内存映射指什么与RAM视图.md)

---

### §3 自检（概念）

- [ ] 能背 **开发铁则**：分配池 **默认只从 Conventional** 划（[3.4](./section-3-4-地址清单与UEFI内存类型.md)）
- [ ] 能说出 **四层 RAM 占用** 各是谁、内核能不能用（[3.2](./section-3-2-RAM四层占用.md)）
- [ ] 能用 **管家 / 工作台 / 办公室** 比喻说清 **固件 vs `.efi` vs 内核**（[3.3](./section-3-3-固件与EFI应用内存隔离.md)）
- [ ] 能解释 **GetMemoryMap ≠ 动态库共享**
- [ ] 能说出 **`GetMemoryMap` 每行** 含地址、长度、**Type**、**Attribute**
- [ ] 知道 **LoaderCode：可执行、不可写** — 别当普通数组乱改（[3.4](./section-3-4-地址清单与UEFI内存类型.md)）
- [ ] 知道 **ACPI Reclaim**：Loader 期勿乱踩，内核后或可回收
- [ ] 知道 **`ExitBootServices` 后** 须 **重读 map** 再看 Boot/Loader 区是否可回收

---

← [3.4 地址清单与 UEFI 类型](./section-3-4-地址清单与UEFI内存类型.md) · [§3 索引](./section-3-主存储器与内存映射.md) · 下一节 [4. GetMemoryMap](./section-4-GetMemoryMap与导出memmap.md)
