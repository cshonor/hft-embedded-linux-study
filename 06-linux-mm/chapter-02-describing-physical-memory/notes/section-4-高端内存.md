# Ch 2 §4 高端内存 (High Memory)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**

### 4. 高端内存 (High Memory)

**32 位 x86** 上，内核 **直接映射** 的虚拟地址窗口有限（`ZONE_NORMAL` 能覆盖的物理范围有顶）。

| 问题 | 内核做法 |
|------|----------|
| 物理内存在 **1GiB–4GiB**（甚至 **PAE 下更大**，如 64GiB） | 落在 **`ZONE_HIGHMEM`** |
| 内核不能随时用「线性偏移」访问 | 需 **`kmap()`** 等 **临时映射** 到 `ZONE_NORMAL` 可访问的虚拟地址，用完 **`kunmap()`** |

**64 位** 桌面/服务器上 **HIGHMEM 常不存在或为空** — 但 **「并非所有物理页都能零成本直接 touch」** 的思想仍在（IO 映射、特殊区域等）。

→ 与 [Ch 9 高端内存管理](../../chapter-09-high-memory-management/)（原书专章，现代 x86_64 可读作背景）。

---
