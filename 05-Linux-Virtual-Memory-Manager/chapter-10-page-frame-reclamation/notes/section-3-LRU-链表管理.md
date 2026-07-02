# Ch 10 §3 LRU 链表管理

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

### 3. LRU 链表管理

除 **Slab 占用的页** 外，**在用页** 多挂在 **LRU** 上供扫描。

### `refill_inactive()`

定期把页从 **active 尾部** 移到 **inactive** — 保持 **active ≈ 总页缓存的 ~2/3**（原书比例），**inactive** 供 **shrink** 扫描。

### `shrink_cache()`（现代 `shrink_page_list` 等）

从 **inactive 尾部** 取 victim，按状态分支：

| 页状态 | 典型动作 |
|--------|----------|
| **干净 + 无映射 / 可丢** | 直接 **free** 回 Buddy |
| **脏页 (PG_dirty)** | **写回** 磁盘再释放 |
| **仍被映射** | 需 **解 PTE** — 文件页可丢映射；**匿名页** → **swap**（§5） |
| **仍被 pin / 锁定** | **跳过** |

---
