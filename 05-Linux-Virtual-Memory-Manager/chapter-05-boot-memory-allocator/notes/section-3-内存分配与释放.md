# Ch 5 §3 内存分配与释放 (Alloc / Free)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

### 3. 内存分配与释放 (Alloc / Free)

### 分配 API（UMA / NUMA 两套，语义相近）

| 宏 / 函数（原书） | 典型用途 |
|-------------------|----------|
| **`alloc_bootmem()`** | 分配 **boot 生命周期内** 的内核数据结构 |
| **`alloc_bootmem_low()`** | 倾向从 **低端 / DMA 可达** 物理区分配 |
| **`alloc_bootmem_pages()`** | 按 **整页** 粒度分配 |

NUMA 变体带 **node 参数** — 在 **指定 node** 上分配。

### 释放的限制

**`free_bootmem()`** 重要约束：

| 规则 | 后果 |
|------|------|
| **只能释放完整页** | 页内 **部分占用** 时，bootmem **不跟踪** 页内碎片 |
| 若只「部分释放」 | 整页仍视为 **保留** |

**为何可接受：** boot 阶段分配的内存 **几乎伴随系统一生**（页表、mem_map、伙伴系统元数据…）— **很少真正 free**，退役时 **整页移交 Buddy** 即可。

---
