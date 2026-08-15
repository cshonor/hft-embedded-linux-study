## ⑥ 创建与删除地址区间

用户态 **`mmap` / `munmap`** 最终进入内核 **`do_mmap` / `do_munmap`** — 操纵 **VMA 链表/树** 与 **页表**（PTE 可延迟建立）。

#### 创建 · `do_mmap()`

| 路径 | 说明 |
|------|------|
| **用户** | **`mmap()` / `mmap2()`** syscall（Ch 5） |
| **内核** | **`do_mmap(file, addr, len, prot, flags, pgoff)`** |

| 步骤（概念） | 行为 |
|--------------|------|
| 1 | **`find_vma_intersection`** — 检查 **重叠** |
| 2 | 选 **未占用 VA**（`get_unmapped_area`）或 **`MAP_FIXED`** |
| 3 | **`vm_area_alloc`** + 填 **flags/ops** |
| 4 | **插入 rbtree + 链表** |
| 5 | **合并** 相邻 **同属性 VMA**（`vma_merge`） |
| 6 | PTE | **未必立刻分配** — **fault-on-first-touch** |

```c
/* 用户态：匿名私有 RW 映射 */
void *p = mmap(NULL, size,
               PROT_READ | PROT_WRITE,
               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

/* HFT 常用组合 */
void *ring = mmap(addr_hint, size,
                  PROT_READ | PROT_WRITE,
                  MAP_SHARED | MAP_ANONYMOUS | MAP_HUGETLB | MAP_POPULATE,
                  hugetlb_fd, 0);
mlock(ring, size);
```

#### `mmap` flags 与 HFT

| flag | 效果 |
|------|------|
| **`MAP_PRIVATE`** | **COW** — 默认 heap 式 |
| **`MAP_SHARED`** | 跨进程 **可见** — **IPC ring** |
| **`MAP_FIXED`** | **强制 VA** — 与 **硬件寄存器约定地址** 对齐时用 |
| **`MAP_POPULATE`** | **同步 fault** 填页 — **启动尖刺换盘中无 fault** |
| **`MAP_LOCKED` / `mlock()`** | **VM_LOCKED** — **禁止 swap** |
| **`MAP_HUGETLB`** | **预预留 huge 页** — 须 **hugetlb pool** |
| **`MAP_NORESERVE`** | 不预留 swap/account — **慎用** |

#### 删除 · `do_munmap()`

| 用户 | **`munmap(start, len)`** |
|------|--------------------------|
| 内核 | **`do_munmap`** — **拆分/删除 VMA**、**Tear down PTE**、**减 map_count** |

| 副作用 | 说明 |
|--------|------|
| **文件映射** | **`fput`**、**页缓存 unmap** |
| **共享匿名** | 其他映射 **仍存活** 直至 **各自 munmap** |
| **TLB** | **`flush_tlb_range`** |

#### 与 Ch 16 页缓存

| 映射类型 | 后备 |
|----------|------|
| **文件 `MAP_SHARED`** | **address_space → 页缓存** |
| **匿名** | **swap / 零页** |
| **hugetlb** | **独立 huge page pool** |

**HFT：** **启动脚本清单**：`sysctl vm.nr_hugepages` → **`mmap MAP_HUGETLB`** → **`mlock`** → **`madvise(MADV_DONTFORK)`** 防 **fork COW**。盘中 **零 munmap** — **VMA 树只读路径**。

→ [Ch 5 syscall](../../chapter-05-system-calls/) · [Ch 16 页缓存](../../chapter-16-the-page-cache-and-page-writeback/) · [01 CSAPP mmap](../../../../02-computer-systems/chapter-09-virtual-memory/) · [17 HFT Practice](../../../../17-hft-engineering/)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, -1, 0) 做了什么？

<details><summary>答案</summary>

1) do_mmap 在 mm 中创建新 VMA（VM_READ|VM_WRITE|VM_SHARED|VM_ANONYMOUS）；2) 不分配物理页（延迟到首次访问）；3) 返回 VMA 起始地址。首次写 → page fault → 分配物理页 → 建 PTE。MAP_SHARED 的页在 fork 后父子共享（可用于 IPC）。HFT 用 MAP_SHARED|MAP_LOCKED 共享行情数据并锁在物理内存。

</details>

**Q2.** mlock() 对 HFT 有什么意义？

<details><summary>答案</summary>

mlock 锁定内存页在物理 RAM 中，禁止换出到 swap。HFT 交易数据如果被换出，访问时需要磁盘 IO → 毫秒级延迟 → 灾难。mlockall(MCL_CURRENT|MCL_FUTURE) 锁定当前和未来所有页。HFT 进程启动时 mlockall 防止任何页被换出。需要 CAP_IPC_LOCK 或 root 权限。

</details>

</details>
---
