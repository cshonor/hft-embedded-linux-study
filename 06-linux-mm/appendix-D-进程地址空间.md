# 附录 D 进程地址空间 · Process Address Space

> **Code Commentary** · Mel Gorman · **选读** · 源码核验：Linux v6.6

概念总览 → [./chapter-04-process-address-space/](./chapter-04-process-address-space/)

---

## 本节走读什么

正文 Ch4 讲了「VMA、mm_struct、缺页、COW、mlock」。本附录走读**地址空间管理的代码组织**：`mm/mmap.c`（映射的建与拆）、`mm/mlock.c`（锁页）、`mm/gup.c`（用户态页的 pin）。

---

## 1. 映射的建立：`do_mmap`（mm/mmap.c:1203）

`mmap`/`brk`/`mremap` 最终都汇聚到 `do_mmap`：

```
SYSCALL_DEFINE6(mmap_pgoff)     // mmap.c:1427  用户态入口
        │
SYSCALL_DEFINE1(brk)            // mmap.c:178   堆扩展入口
        │
        ▼
do_mmap(file, addr, len, prot, flags, pgoff)     // mmap.c:1203
        │
        ├─ vma_merge(vmi, mm, ...)               // mmap.c:863  尝试合并相邻 VMA
        ├─ vm_area_alloc(mm)                     // 分配 struct vm_area_struct
        ├─ file->f_op->mmap(...)                 // 文件映射回调（如 shmem_mmap）
        └─ vma_link / 插入 maple tree            // 挂进 mm->mm_mt
```

**走读要点**：`vma_merge`（:863）是性能关键——频繁的 mmap/munmap 若每次都新建 VMA，`mm->mm_mt`（maple tree）会膨胀，所以先尝试**与前后 VMA 合并**（`can_vma_merge_before/after`，:773/:796）。这直接决定地址空间管理的开销。

## 2. VMA 的存储：maple tree（v6.1+）

v6.6 的 `struct mm_struct` 里，VMA 用 **maple tree**（`mm->mm_mt`）组织，取代了老内核的红黑树 + 双向链表：

| 版本 | VMA 组织 | 查找复杂度 |
|------|----------|-----------|
| ≤ v6.0 | 红黑树 + 链表 | O(log n) |
| v6.1+ | maple tree（B 树变体） | O(log n)，**内存更省、锁更细** |

`find_vma` / `find_vma_prev`（mmap.c:1708/:1758）都是对 maple tree 的遍历。这是 Ch4 的「版本断崖」点之一。

## 3. 锁页：`mlock` 系列（mm/mlock.c）

```c
SYSCALL_DEFINE2(mlock, ...)     // mlock.c:622  锁一段地址
SYSCALL_DEFINE3(mlock2, ...)    // mlock.c:627  带 flags（MCL_ONFAULT 等）
SYSCALL_DEFINE1(mlockall, ...)  // mlock.c:705  锁整个进程
        │
        ▼
mlock_fixup(vmi, vma, ...)      // mlock.c:412  给 VMA 打 VM_LOCKED
        │
        ▼
mlock_vma_pages_range(vma, ...) // mlock.c:369  把已存在的页 pin 住
```

**走读要点**：`mlock` 分两步——① 给 VMA 设 `VM_LOCKED` 标志（未来的缺页会锁住新页）；② 立即 `mlock_vma_pages_range` 把**当前已在**的页 pin 住。这解释了为什么 `mlock` 对「已经映射但还没 touch 的页」需要 `MCL_ONFAULT` 才延迟锁页。

## 4. 用户态页 pin：`gup.c`

`get_user_pages`（GUP）把用户态虚拟地址**pin 成 struct page**，用于 O_DIRECT、`process_vm_readv`、RDMA 等「绕过缺页、直接操作物理页」的场景：

```
get_user_pages(...)                 // gup.c（公开入口）
        │
follow_page_mask(vma, addr, ...)    // gup.c:809  逐级走页表
        │
follow_page_pte(...)                // gup.c:579  读 PTE 拿到 struct page
```

**走读要点**：GUP 与普通缺页的区别是——GUP 假设页**已经存在**（不做缺页处理），只「查 PTE → 拿 page → 增引用计数」。HFT 的 RDMA/用户态零拷贝路径会用到它，理解「pin 之后页不会被回收」是延迟稳定的关键。

---

## 与正文对应

| 附录内容 | 正文落点 |
|----------|----------|
| `do_mmap` / `vma_merge` | Ch4（VMA 建立/合并） |
| maple tree 存储 | Ch4（版本断崖：红黑树 → maple tree） |
| `mlock_fixup` 两步 | Ch4（mlock 语义）+ Ch13（mlock 不防 OOM） |
| GUP | Ch4（copy_*_user / 零拷贝） |

---

## HFT / 嵌入式关联

| 手段 | 落点 |
|------|------|
| `mlockall(MCL_CURRENT\|MCL_FUTURE)` | 锁死 RSS，防 reclaim 延迟尖刺（Ch10/Ch13 呼应） |
| 理解 `vma_merge` | 频繁 mmap 的碎片化会拖慢地址空间查找，HFT 尽量**一次性大 mmap** |
| GUP pin | RDMA/零拷贝路径 pin 住页，避免「使用时页被换出」 |

---

## 相关章节

- 上一章：[appendix-C-页表管理.md](./appendix-C-页表管理.md)
- 下一章：[appendix-E-启动内存分配器.md](./appendix-E-启动内存分配器.md)

---

<details>
<summary>自测 5 问（点开看答案）</summary>

**Q1：`mmap`/`brk`/`mremap` 最终都汇聚到哪个函数？**

`do_mmap`（mmap.c:1203），它是所有「建映射」路径的汇聚点。

**Q2：`vma_merge` 为什么是性能关键？**

频繁 mmap/munmap 若不合并相邻 VMA，maple tree 会膨胀、查找变慢；`vma_merge`（:863）先尝试与前后 VMA 合并，控制 VMA 数量。

**Q3：v6.1 起 VMA 用什么数据结构组织？取代了什么？**

maple tree（`mm->mm_mt`），取代了老内核的红黑树 + 双向链表，内存更省、锁更细。

**Q4：`mlock` 的两步操作是什么？**

① `mlock_fixup` 给 VMA 设 `VM_LOCKED`（未来缺页会锁新页）；② `mlock_vma_pages_range` 立即 pin 当前已存在的页。

**Q5：GUP（get_user_pages）和普通缺页处理的本质区别？**

GUP 假设页已存在，只「查 PTE → 拿 page → 增引用计数」，不做缺页处理；用于 O_DIRECT/RDMA 等绕过缺页直接操作物理页的场景。

</details>
