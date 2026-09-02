# 附录 I 高端内存管理 · High Memory Management

> **Code Commentary** · Mel Gorman · **选读** · 源码核验：Linux v6.6（`mm/highmem.c`）

概念总览 → [./chapter-09-high-memory-management/](./chapter-09-high-memory-management/)（x86_64 多为背景）

---

## 本节走读什么

原书附录 I 走读 **`kmap` / `bounce buffer` / 历史 emergency pools**。但 v6.6 已发生两次大换代：**bounce buffer 被彻底删除**（由 `swiotlb` 接管 DMA 无法触达的内存），**`kmap_atomic` 被 `kmap_local_page` 取代**。本附录走读 v6.6 `mm/highmem.c`（816 行）中**幸存下来的两套映射机制**：老式 `kmap_high`（PKMAP 固定映射）与现代 `kmap_local`（per-CPU fixmap 临时映射）。

> **关键前提**：高端内存是 **32 位系统**的产物。x86_64 上 `CONFIG_HIGHMEM=n`，整个 `#ifdef CONFIG_HIGHMEM` 块（highmem.c:52–448）被编译器整段删掉，只剩 `kmap_local` 机制（:450 起）。所以本节对 x86_64 用户主要是「历史机制 + 为什么现代内核不再需要它」的理解；对 **32 位嵌入式（ARM32）仍是活跃代码**。

---

## 1. 为什么会有「高端内存」

32 位内核虚拟地址 4GB 按 **3:1 划分**：低 3GB 给用户进程，高 1GB（从 `PAGE_OFFSET` 起）给内核。内核这 1GB 里，用「直接映射（direct map）」线性覆盖物理内存——但只能覆盖 ≤896MB（`high_memory` 上限）。**物理内存超过 896MB 的部分就是「high memory」**，内核无法用固定虚拟地址直接访问，必须**临时建立映射**。

```
32 位虚拟地址空间（4GB）
┌─────────────────────────────┐ 0xFFFFFFFF
│  内核空间 (1GB)              │
│  ┌───────────────────────┐  │
│  │ fixmap / PKMAP 临时映射 │  │ ← 访问 highmem 的唯一入口
│  │ vmalloc 区域            │  │
│  ├───────────────────────┤  │  high_memory (896MB 边界)
│  │ direct map (≤896MB)    │  │ ← 线性映射，只能到这
│  └───────────────────────┘  │
├─────────────────────────────┤ PAGE_OFFSET (0xC0000000)
│  用户空间 (3GB)              │
└─────────────────────────────┘ 0x00000000
```

**判断入口**：`is_highmem(zone)` 判断一个 zone 是否属于高端区；`__nr_free_highpages`（highmem.c:117）统计高端空闲页；`_totalhigh_pages`（:114）记录高端页总数。

---

## 2. 老式映射：`kmap_high` / `kunmap_high`（PKMAP）

```
kmap(page)  →  kmap_high(page)              // highmem.c:296
    │
    ├─ lock_kmap()                            // 全局 kmap_lock spinlock (:131)
    ├─ vaddr = page_address(page)             // 已映射？直接返回
    │    └─ 未映射 → map_new_virtual(page)    // 在 PKMAP 区抢一个 slot
    ├─ pkmap_count[PKMAP_NR(vaddr)]++         // 引用计数 +1 (:308)
    └─ unlock_kmap()
```

**核心数据结构**：

| 符号 | 行号 | 作用 |
|------|------|------|
| `pkmap_count[LAST_PKMAP]` | :130 | 每个 PKMAP slot 的引用计数（含虚拟别名颜色） |
| `kmap_lock` | :131 | 全局自旋锁，`__cacheline_aligned_in_smp` |
| `pkmap_page_table` | :133 | PKMAP 区对应的页表 |

**走读要点**：

1. **会 sleep 的 kmap**。`map_new_virtual` 在 PKMAP 区（`PKMAP_ADDR(0)~PKMAP_ADDR(LAST_PKMAP)`）没有空闲 slot 时，会**挂到等待队列 sleep**，直到有人 `kunmap` 腾出 slot（`get_pkmap_wait_queue_head` :106 的 `pkmap_map_wait`）。所以 `kmap()` **只能在进程上下文调用，不能在原子/中断上下文用**。
2. **引用计数防止「过早 flush」**。`kmap_high` 对同一 slot 反复 `kmap` 会让 `pkmap_count` 递增（:308，并 `BUG_ON(count < 2)` 断言）；`kunmap_high`（:348）递减，减到 1 时若有等待者才 `wake_up`（:387）。**计数不为 0 就不会 TLB flush**——保证映射被释放干净才可能回收。
3. **耗尽时刷新**：`flush_all_zero_pkmaps`（:185）把 `pkmap_count==0` 的 slot 全部做 `flush_tlb_kernel_range`，腾出空间给新映射。

---

## 3. 现代映射：`kmap_local_page` / `kunmap_local`（fixmap）

`kmap_local_page()` 是 v6.6 的**推荐临时映射**，替代了旧的 `kmap_atomic()`。核心是 **per-task 的映射栈** `current->kmap_ctrl`，用 `fixmap` 区的一段地址做临时窗口：

```
kmap_local_page(page)
    → __kmap_local_page_prot(page, prot)       // highmem.c:564
         ├─ 非 highmem 页 → 直接 page_address() 返回（:573）
         ├─ 尝试 arch_kmap_local_high_get()（:577，多数架构返回 NULL）
         └─ __kmap_local_pfn_prot(pfn, prot)   // :538
              ├─ migrate_disable(); preempt_disable()   // :548-549
              ├─ idx = kmap_local_idx_push()            // 栈顶入栈 :464
              ├─ vaddr = __fix_to_virt(FIX_KMAP_BEGIN + idx)  // :551
              ├─ 写 PTE 建立映射（:555）
              └─ preempt_enable()  返回 vaddr

kunmap_local(vaddr)
    → kunmap_local_indexed(vaddr)              // :585
         ├─ pte_clear 撤销映射（:615）
         └─ kmap_local_idx_pop()               // 栈顶出栈 :618
```

**核心数据结构 `struct kmap_ctrl`**（per-task，include/linux/sched.h）：`idx`（当前栈深）+ `pteval[]`（已建映射的 PTE 缓存数组）。

**走读要点**：

1. **不可 sleep，但可嵌套**。`kmap_local` 只关抢占（`preempt_disable`），不拿锁、不 sleep，所以**原子/中断上下文都能用**；`kmap_ctrl.idx` 的入栈/出栈（:464/:477）让同一个进程可以**嵌套多层 kmap_local**，每层占用一个递增的 fixmap slot。
2. **进程切换时强制清理**：`__kmap_local_sched_out`（:634）在 `switch_to` 前被调用，把当前进程残留的 kmap_local 映射全部清掉——因为 fixmap 是全局共享窗口，**不能跨进程泄漏**。
3. **非 highmem 页是零开销快路径**：`__kmap_local_page_prot` 对 `!PageHighMem(page)` 直接 `page_address()` 返回（:573），**根本不建立映射**。这也是为什么 x86_64 上 kmap_local 几乎没成本。

---

## 4. 两个版本断崖

| 旧机制（原书附录 I） | v6.6 现状 | 替代者 |
|----------------------|-----------|--------|
| `bounce buffer`（BLK_BOUNCE_HIGH） | **彻底删除** | `swiotlb`（软件 I/O TLB，处理 DMA 无法访问高地址内存） |
| `kmap_atomic()` | **删除**（`CONFIG_KMAP_LOCAL`） | `kmap_local_page()` / `kunmap_local()` |
| `kmap()` / `kunmap()` | 仅 `CONFIG_HIGHMEM`（32 位）存活 | x86_64 上退化为 `page_address()` |
| emergency pools（紧急内存池） | 概念重构 | mempool / 其他 |

**典型消费者**：`zero_user_segments`（:392）用 `kmap_local_page` 逐页把用户页片段清零（`memset`），配合 `flush_dcache_page`（:438）——这是「高端内存页只能临时映射后才能访问」的经典用例。

---

## 与正文对应

| 附录内容 | 正文落点 |
|----------|----------|
| 高端内存成因（3:1 划分 + 896MB 边界） | Ch9（高端内存背景） |
| `kmap_high` / PKMAP | Ch9 §2（固定映射） |
| `kmap_local` / fixmap | Ch9 §3（现代临时映射） |
| bounce buffer → swiotlb | Ch9 §4（版本断崖） |

---

## HFT / 嵌入式关联

| 手段 | 落点 |
|------|------|
| x86_64 无 highmem | 现代 HFT 服务器（x86_64）**不受高端内存限制**，`kmap_local` 对普通页是零开销 |
| 32 位嵌入式（ARM32） | 仍有 `CONFIG_HIGHMEM`，热路径避免对 highmem 页频繁 `kmap`（会 sleep + 抢全局锁） |
| 实时性 | 临时访问页优先用 `kmap_local_page`（不可 sleep、无锁、可嵌套），而非老 `kmap`（可能阻塞） |
| 零页清零 | 大量清零用户页（如 mlock 预置、共享内存 init）走 `zero_user_segments`，注意其 per-page `kmap_local` 成本 |

---

## 相关章节

- 上一章：[appendix-H-Slab分配器.md](./appendix-H-Slab分配器.md)
- 下一章：[appendix-J-页框回收.md](./appendix-J-页框回收.md)

---

<details>
<summary>自测 5 问（点开看答案）</summary>

**Q1：什么是「高端内存」？为什么 32 位系统需要它？**

32 位内核 3:1 划分地址空间，内核只有 1GB 虚拟地址，其中 direct map 只能线性覆盖 ≤896MB 物理内存。**超过 896MB 的物理内存就是 highmem**，内核无法用固定虚拟地址直接访问，必须临时建立映射（kmap / kmap_local）。

**Q2：`kmap()` 和 `kmap_local_page()` 的核心区别？**

`kmap()`（`kmap_high` :296）会 sleep——PKMAP 区无空闲 slot 时挂起等待，只能在进程上下文用；`kmap_local_page()`（`__kmap_local_page_prot` :564）只关抢占、不 sleep，可嵌套、可在原子上下文用，且对非 highmem 页零开销。

**Q3：`kmap_local` 为什么能在进程切换时安全清理？**

fixmap 是全局共享的临时窗口，不能跨进程泄漏。`__kmap_local_sched_out`（:634）在 `switch_to` 前调用，把当前进程 `kmap_ctrl` 里残留的映射全部 `pte_clear`。

**Q4：bounce buffer 在 v6.6 去哪了？**

被彻底删除，由 `swiotlb`（软件 I/O TLB）接管「DMA 设备无法访问的内存」场景。bounce buffer 曾是 highmem 时代为 32 位 DMA 限制打补丁的机制。

**Q5：x86_64 上 `kmap_local_page` 对普通页的开销是多少？**

近乎零。`__kmap_local_page_prot`（:573）对 `!PageHighMem(page)` 直接 `page_address()` 返回，不建立任何映射，因为 x86_64 的 direct map 覆盖了全部物理内存。

</details>
