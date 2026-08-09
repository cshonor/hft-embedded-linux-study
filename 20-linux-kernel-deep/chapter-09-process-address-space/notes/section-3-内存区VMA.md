## 3. 内存区 (Memory Regions)

> 一段 **权限相同** 的连续线性地址 — `vm_area_struct`

---

### 一、`vm_area_struct` 是什么

每个 VMA 描述进程地址空间中的一段区间，例如：

- 可执行 **代码段**（只读 + 可执行）  
- **数据段 / BSS**（可读写）  
- **`mmap` 文件映射**  
- **匿名堆 / 栈**  

同一 VMA 内：**起始地址、长度、访问权限、 backing 对象** 一致。

---

### 二、红黑树 + 链表双重管理

| 结构 | 用途 |
|------|------|
| **单向链表** | 遍历所有 VMA（如 `/proc/pid/maps`） |
| **红黑树** | 按地址 **O(log n)** 查找「包含某线性地址的 VMA」 |

大型应用可有 **成百上千** VMA — 树查找对缺页热路径至关重要。

> **深潜可选：** 红黑树插入/旋转平衡 — 见内核 `mm/mmap.c`；ULK 讲 **为何用树**，不必死记旋转细节。

---

### 三、分配与释放

| 函数 | 作用 |
|------|------|
| **`do_mmap()` / `do_mmap_pgoff()`** | 创建新 VMA、建立页表项骨架 |
| **`do_munmap()`** | 释放 VMA、解除映射 |

系统调用 **`mmap` / `munmap` / `brk`** 最终落到这些内核例程（入口见 [Ch 10](../../chapter-10-system-calls.md)）。

---

### 四、与 Ch 8 的分工

| 层 | 负责 |
|----|------|
| **Ch 9 VMA** | **虚拟** 地址区间、权限、映射关系 |
| **Ch 8 伙伴系统** | **物理页框** 的实际分配 |

VMA 创建时通常 **还不分配物理页** — 等缺页再调 Ch 8 路径。

### 常见陷阱

1. 把 ULK 的 VMA 红黑树当现代版——6.1+ 用 maple tree，VMA 结构体也有变化
2. 混淆 VMA 的 `vm_start`/`vm_end` 和实际物理页——VMA 描述虚拟地址范围，物理页在访问时才分配（demand paging）
3. 以为所有 mmap 都分配物理内存——匿名 mmap 只分配 VMA，物理页在首次访问时通过 page fault 分配

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** `vm_area_struct`（VMA）的核心字段有哪些？

<details><summary>答案</summary>

`vm_start`/`vm_end`（虚拟地址范围）、`vm_flags`（权限：`VM_READ`/`VM_WRITE`/`VM_EXEC`/`VM_MAY*`）、`vm_page_prot`（页表权限）、`vm_ops`（VMA 操作函数表：`fault`/`open`/`close`）、`vm_file`（文件映射时指向 file）、`vm_pgoff`（文件内偏移）、`anon_vma`（匿名映射的反向映射）。ULK 时代还有 `vm_rb`（红黑树节点），6.1+ 改为 maple tree 节点。

</details>

**Q2.** 匿名 mmap 和文件 mmap 的 VMA 有什么区别？

<details><summary>答案</summary>

匿名 mmap：`vm_file = NULL`，`vm_ops = NULL`（或匿名 `vm_ops`），物理页在首次写时分配（`do_anonymous_page()`）。文件 mmap：`vm_file != NULL`，`vm_ops = file->f_op->vm_ops`（如 `ext4_file_vm_ops`），物理页从 page cache 读取（`vm_ops->fault()` → `filemap_fault()`）。匿名 mmap 用于堆/栈/`malloc` 大块；文件 mmap 用于共享库/内存映射 I/O。

</details>

**Q3.** HFT 如何用 `mmap` 建立零拷贝数据通道？

<details><summary>答案</summary>

① 共享内存：`mmap(MAP_SHARED | MAP_ANONYMOUS, ...)` 在父子进程间共享。② 文件映射：`mmap(MAP_SHARED, fd, ...)` 映射文件，多进程共享同一物理页。③ `memfd_create()` + `mmap`：无文件支持的共享内存（`/dev/shm`）。④ huge page 共享：`mmap(MAP_SHARED | MAP_HUGETLB, ...)`。关键：共享内存 + 内存屏障（`std::atomic`）实现无锁 IPC，延迟 <100ns（vs pipe/socket 的 us 级）。

</details>

</details>

---

← [2. 内存描述符](./section-2-内存描述符.md) · 下一节 [4. 缺页异常](./section-4-缺页异常.md)
> ↔ [LKD Ch15 §15.3 虚拟内存区域](../../../05-linux-kernel/00_Book_3rd_Notes/chapter-15-process-address-space/notes/section-15.3-虚拟内存区域.md)
