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
| 进程地址空间、缺页 | [Ch 9 进程地址空间](../chapter-09-process-address-space/) 🔴 |
| 页回收、swap | [Ch 17 页回收](../chapter-17-page-reclaim.md) 🟡 |
| 页表、高端内存 | [Ch 2 内存寻址](../chapter-02-memory-addressing/) 🔴 |
| VM 专著 | [07 Gorman](../../../05-Linux-Virtual-Memory-Manager/) |
| Slab 深潜 | [07 Gorman Ch 8 Slab](../../../05-Linux-Virtual-Memory-Manager/chapter-08-slab-allocator/) |
| 大页 / NUMA | [15 HFT 工程](../../../15-HFT-Low-Latency-Practice/) · [03 SysPerf Ch 7](../../../13-Systems-Performance-2nd/chapter-07-memory/) |

---

← [3. Slab](./section-3-Slab分配器.md) · 下一章 [Ch 9 进程地址空间](../chapter-09-process-address-space/)
