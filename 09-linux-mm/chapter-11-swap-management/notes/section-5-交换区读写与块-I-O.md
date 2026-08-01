# Ch 11 §5 交换区读写与块 I/O

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

### 5. 交换区读写与块 I/O

| 方向 | 触发 | 路径 |
|------|------|------|
| **读 (swap in)** | **缺页 fault** — PTE 含 swap entry | 异步/同步读 swap → 填物理页 → 设 PTE present |
| **写 (swap out)** | **shrink** 选中脏/匿名 victim | 写 swap slot → PTE → swap entry |

**统一入口（原书）：** **`rw_swap_page()`** — 底层 **块 I/O** 栈。

**HFT：** **swap in fault** = **磁盘延迟 + 锁 + 页表更新** — **`/proc/vmstat` 的 `pgmajfault`** 上升时查 swap 活动。

---
