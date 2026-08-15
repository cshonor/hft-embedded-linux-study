## ③ 虚拟内存区域 · VMA · `vm_area_struct`

地址空间不是 **均匀** 的 — 内核按 **段** 管理，每段是一个 **VMA**，带 **独立权限、后备存储、操作回调**。

#### 核心字段

| 字段 | 说明 |
|------|------|
| **`vm_start` / `vm_end`** | **半开区间** [start, end) |
| **`vm_flags`** | **VM_READ/WRITE/EXEC**、**VM_SHARED**、**VM_LOCKED**、**VM_HUGETLB** 等 |
| **`vm_page_prot`** | 写入 **PTE** 的 **保护属性** |
| **`vm_ops`** | **`fault` / `open` / `close` / `nopage`** 等回调表 |
| **`vm_file` / `vm_pgoff`** | 文件映射时 **指向 struct file** + **文件内偏移** |
| **`vm_private_data`** | 驱动/fs 私有 |

```
mm_struct
  ├── VMA: [0x400000, 0x401000)  text     R-X   anon
  ├── VMA: [0x600000, 0x602000)  data     RW-   anon
  ├── VMA: [0x7f..000, 0x7f..)   heap     RW-   anon
  ├── VMA: [0x7f..800, 0x7f..)   lib.so   R-X   file
  └── VMA: [0x7f..0000000, ...)  ringbuf  RW-   anon MAP_SHARED|HUGETLB
```

#### `vm_flags` 与 HFT 相关位

| 标志 | 含义 |
|------|------|
| **`VM_SHARED`** | **`MAP_SHARED`** — 修改 **可见于其他映射者** |
| **`VM_PRIVATE`** | **`MAP_PRIVATE`** — **COW** 写时复制 |
| **`VM_LOCKED`** | **`mlock` 范围** — 计入 **`locked_vm`**，**不换出** |
| **`VM_HUGETLB`** | **hugetlbfs / MAP_HUGETLB** |
| **`VM_DONTEXPAND`** | 禁止 **`mremap` 扩展** |
| **`VM_IO`** | **MMIO** 映射 — **非 RAM** |

#### `vm_ops->fault` — 缺页入口

| 场景 | fault 行为 |
|------|------------|
| **匿名首次写** | 分配 **物理页** + 填 PTE |
| **文件映射** | 从 **页缓存** 找页或 **读盘** |
| **COW** | 复制页、改 PTE **writable** |
| **设备 mmap** | 驱动 **自定义 fault** |

#### 每 VMA = 一个「内存对象」

| 视角 | 好处 |
|------|------|
| **统一抽象** | text、heap、**mmap 文件**、**共享内存** 同一套结构 |
| **不同 vm_ops** | **tmpfs**、**hugetlb**、**DRM** 各自 **fault 逻辑** |

**HFT：** 每个 **策略缓冲** 应对应 **独立 VMA** — 便于 **`/proc/maps` 审计** 与 **`mlock` 精确范围**。`MAP_SHARED` **行情 ring** 与 **私有 stack/heap** 分离 — **权限最小化**（ring **RW- 无 X**）。

→ [Ch 15.6 mmap 创建](./section-15.6-创建与删除地址区间.md) · [Ch 15.8 缺页](./section-15.8-从访问到缺页概念.md) · [06 Gorman VMA](../../../../06-linux-mm/chapter-04-process-address-space/notes/section-3-内存区域.md)


> ↔ [ULK Ch9 §3 内存区VMA](../../../../19-linux-kernel-deep/chapter-09-process-address-space/notes/section-3-内存区VMA.md)


<details>
<summary>自测题（点击展开）</summary>

**Q1.** VMA 的作用是什么？text/data/heap/stack 各是什么 VMA？

<details><summary>答案</summary>

VMA（vm_area_struct）描述一段连续虚拟地址区间的属性（起止地址/权限/映射方式/后备存储）。text=只读可执行、data=读写、heap=读写可扩展（brk）、stack=读写向下扩展。mmap 创建新 VMA。`cat /proc/pid/maps` 可看到进程所有 VMA。HFT mmap 共享内存会在 maps 中显示为独立 VMA。

</details>

**Q2.** VMA 的权限如何影响内存访问？

<details><summary>答案</summary>

VMA 权限位 VM_READ/VM_WRITE/VM_EXEC 控制用户态访问权限。写只读 VMA → page fault → SIGSEGV。内核态不受 VMA 权限限制（可写任何物理页）。VMA 权限 + PTE 权限双重检查：VMA 是粗粒度（段级），PTE 是细粒度（页级，如 COW 页标为只读）。

</details>

</details>
---
