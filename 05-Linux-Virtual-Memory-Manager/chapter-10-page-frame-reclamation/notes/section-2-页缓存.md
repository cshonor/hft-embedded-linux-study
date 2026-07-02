# Ch 10 §2 页缓存 (Page Cache)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

### 2. 页缓存 (Page Cache)

**目的：** 避免 **重复读盘** — 磁盘数据 **缓存于 RAM**。

| 页缓存中的页类型（原书） | 来源 |
|--------------------------|------|
| **文件 mmap 缺页** | 映射文件读入 |
| **块设备缓冲** | `read()` / buffer head 路径 |
| **Swap cache 匿名页** | 已换出或换出过程中的匿名页 |
| **共享内存页** | shm / mmap shared |

**索引：** **`struct address_space` + 文件内 offset** → **哈希表** 快速定位 **同一文件同一偏移** 的 `struct page`。

**HFT：** 大 **内存映射行情文件** 会占 **page cache** — 与 **进程 RSS** 分开统计；**drop_caches** 可回收 **clean page cache**，**不** 换出 **mlock 页**。

---
