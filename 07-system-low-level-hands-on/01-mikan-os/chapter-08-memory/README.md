# Ch 8 · 内存管理

> **原书第 8 章** · HFT **🔴** · 官方源码标签 `osbook_day08`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **脱离 UEFI 内存：** 迁移 GDT/栈/页表 · **身份映射** · **位图页帧分配器**

---

### 本章结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 映射** | 解析 **UEFI Memory Map** | 找出可用物理 RAM |
| **② 迁移** | 栈 · **GDT** · 页表移出 UEFI 临时区 | 避免被覆盖崩溃 |
| **③ 分页** | **4 级页表** · **Identity Map** · **CR3** | x86-64 强制分页 |
| **④ 分配器** | **BitmapMemoryManager** · **first fit** | `Allocate()` / `Free()` |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. 解析 UEFI 内存映射 | [notes/section-2-解析UEFI内存映射.md](./notes/section-2-解析UEFI内存映射.md) |
| 3. 迁移栈与关键数据结构 | [notes/section-3-迁移栈与关键数据结构.md](./notes/section-3-迁移栈与关键数据结构.md) |
| 4. GDT 与分段 | [notes/section-4-GDT与分段.md](./notes/section-4-GDT与分段.md) |
| 5. 四级分页与身份映射 | [notes/section-5-四级分页与身份映射.md](./notes/section-5-四级分页与身份映射.md) |
| 6. 位图管理器与首次适配 | [notes/section-6-位图管理器与首次适配.md](./notes/section-6-位图管理器与首次适配.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 本章做了什么？ | **OS 自持内存** — 身份映射 + **4KiB 页帧位图分配** |
| 与 02 川合 OS 对照？ | 01 **Day 10–12 分页**；Mikan **Ch8 物理帧 + Ch19 进程页表** 拆分 |
| 与 Linux / CSAPP 对照？ | 物理页分配 ≈ **buddy/位图** 极简版 — [CSAPP Ch9](../../../01-CSAPP-3rd/chapter-09-virtual-memory/) · [07 Gorman](../../../05-Linux-Virtual-Memory-Manager/) |

**本章目的：** 不再依赖 UEFI 分配 — **动态 `Allocate/Free`** 基础（Ch5 **Placement new** 后可演进为堆）。

---

## 本章学习目标 · 自检

- [ ] 从 **Memory Map** 识别 **ConventionalMemory**
- [ ] 说清 **栈/GDT/页表迁移** 与 **`RSP` 重设**
- [ ] 用 **`lgdt`** 加载自管 **GDT**
- [ ] 描述 **PML4→PT** 与 **Identity Mapping**、**CR3**
- [ ] 实现 **位图页帧** + **first fit** `Allocate/Free`

---

## 相关

- 上一章：[../chapter-07-interrupt-fifo/](../chapter-07-interrupt-fifo/)
- 下一章：[../chapter-09-layers/](../chapter-09-layers/)
- 衔接：[../chapter-02-edk2-memmap/](../chapter-02-edk2-memmap/) · [../chapter-19-paging/](../chapter-19-paging/)
- 模块导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md)
