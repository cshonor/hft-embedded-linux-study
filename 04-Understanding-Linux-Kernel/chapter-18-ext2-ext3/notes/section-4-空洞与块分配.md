## 4. 文件空洞与块分配 (Holes & Allocation)

---

### 一、文件空洞 (File Holes)

文件含大量 **连续空字符 (NUL)** 时：

| 字段 | 行为 |
|------|------|
| **`i_size`** | **逻辑大小** — 含空洞 |
| **`i_blocks`** | **实际分配的 512B 扇区数** — **不含** 未分配的空洞 |

**不** 为全零区间分配磁盘块 — 节省空间。

→ 缓冲页 / 块不连续：[Ch 15 section-4](../chapter-15-page-cache/notes/section-4-缓冲页与buffer_head.md) · [Ch 16 mmap](../chapter-16-file-access/notes/section-4-内存映射.md)

---

### 二、减碎片启发式

Ext2 分配数据块时：

| 策略 | 目的 |
|------|------|
| **靠近上一块** | 顺序文件 **物理相邻** |
| **同块组优先** | 减少 **跨组寻道** |
| **块预分配 (Preallocation)** | 追加写时除请求块外 **再连续预分配最多 8 块** — 减 **外碎片** |

> **深潜可选：** `ext2_new_block()` 目标块组选择、`ext2_prealloc_block` — `fs/ext2/balloc.c`。

---

### 三、与 Ch 14 块层的关系

分配得到 **逻辑块号** → 经块层 **`bio`** 读写 — [Ch 14](../chapter-14-block-devices/)。

---

← [3. 寻址](./section-3-数据块寻址.md) · 下一节 [5. 内存结构](./section-5-Ext2内存数据结构.md)
