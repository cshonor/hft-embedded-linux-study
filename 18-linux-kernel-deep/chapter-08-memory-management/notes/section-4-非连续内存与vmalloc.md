## 4. 非连续内存区管理 · `vmalloc()`

> 物理页 **不连续**，内核线性地址 **连续**

---

### 一、使用场景

需要 **大块** 内存，但物理 RAM 中 **凑不齐连续页框**：

- 加载 **内核模块**  
- 大容量 **I/O 缓冲区**  

伙伴系统会失败或代价高 → 用 **vmalloc**。

---

### 二、`vmalloc()` / `vfree()` 原理

| 步骤 | 说明 |
|------|------|
| 地址区间 | **`VMALLOC_START` – `VMALLOC_END`**（`PAGE_OFFSET` 3GB 之上） |
| 物理页 | **逐个** 分配零散页框 |
| 元数据 | `kmalloc()` 分配 **页描述符指针数组** |
| 映射 | 修改 **主内核页表** → 线性地址连续、物理分散 |

**优点：** 不受 **外碎片** 限制（物理上）。

**代价：**

- 频繁改页表 → **TLB 刷新**  
- 访问可能 **更慢**（TLB miss、非连续物理）  

→ 内核 **默认优先** 伙伴系统 / kmalloc；vmalloc 用于特定场景。

---

### 三、与 Ch 9 的分工

| 机制 | 视角 |
|------|------|
| **Ch 8 vmalloc** | **内核** 线性地址空间中的非连续物理映射 |
| **Ch 9 进程 VMA** | **用户进程** 虚拟地址、缺页、`mmap` |

---

### 四、后续章节索引

| Ch 8 主题 | 继续读 |
|-----------|--------|
| 进程地址空间、缺页 | [Ch 9 进程地址空间](../../chapter-09-process-address-space/) 🔴 |
| 页回收、swap | [Ch 17 页回收](../../chapter-17-page-reclaim.md) 🟡 |
| 页表、高端内存 | [Ch 2 内存寻址](../../chapter-02-memory-addressing/) 🔴 |
| VM 专著 | [07 Gorman](../../../06-linux-mm/) |
| Slab 深潜 | [07 Gorman Ch 8 Slab](../../../06-linux-mm/chapter-08-slab-allocator/) |
| 大页 / NUMA | [16 HFT 工程](../../../16-hft-engineering/) · [03 SysPerf Ch 7](../../../14-systems-performance/chapter-07-memory/) |

### 常见陷阱

1. 以为 `vmalloc()` 和用户态 `malloc()` 类似——`vmalloc()` 建立页表映射非连续物理页，开销远大于 `kmalloc()`
2. 在性能敏感路径用 `vmalloc()`——`vmalloc()` 需要建页表 + 可能触发 TLB shootdown，不适合热路径
3. 混淆 `vmap()` 和 `vmalloc()`——`vmap()` 映射已有 pages，`vmalloc()` 分配新 pages 再映射

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** `vmalloc()` 的工作原理和开销？

<details><summary>答案</summary>

① 在 vmalloc 区预留虚拟地址空间。② 调 `alloc_page()` 分配物理页（可能不连续）。③ 调 `map_vm_area()` 建页表（每 4KB 一个 PTE）。④ TLB 需要 flush（新映射）。开销：比 `kmalloc()` 慢 10-100 倍（页表建立 + TLB flush）。优势：可分配大块（不受 MAX_ORDER 限制）、不要求物理连续。适合：内核模块加载、大缓冲区分配。不适合：热路径。

</details>

**Q2.** 为什么 `vmalloc()` 不适合 HFT 内核模块热路径？

<details><summary>答案</summary>

① 分配时建页表 = 多次原子写 + TLB flush，微秒级开销。② 访问时 TLB miss（页表 walk），纳秒级开销。③ `vfree()` 需要等 RCU grace period + TLB shootdown IPI，可能毫秒级。如果内核模块必须分配大块，应在初始化时 `vmalloc` 一次，之后用内存池管理。

</details>

**Q3.** 现代内核如何优化 `vmalloc()` 的性能？

<details><summary>答案</summary>

① `vmalloc_to_page()` 用 `virt_to_page()` 快速路径（如果地址在直接映射区）。② lazy TLB flush：延迟到下一个 `schedule()` 时统一 flush。③ `vfree_atomic()`：异步释放（不等 grace period）。④ `__vmalloc_node_range()`：NUMA 感知分配。但这些优化仍无法消除基本开销——热路径应避免 `vmalloc`。

</details>

</details>

---

← [3. Slab](./section-3-Slab分配器.md) · 下一章 [Ch 9 进程地址空间](../../chapter-09-process-address-space/)
> ↔ [LKD Ch12 §12.6 vmalloc](../../../05-linux-kernel/chapter-12-memory-management/notes/section-12.6-vmalloc.md)
