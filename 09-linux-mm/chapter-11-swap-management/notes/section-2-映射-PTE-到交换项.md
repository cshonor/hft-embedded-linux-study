# Ch 11 §2 映射 PTE 到交换项 (PTE ↔ Swap Entry)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

### 2. 映射 PTE 到交换项 (PTE ↔ Swap Entry)

页 **换出后** 物理页框归还 Buddy — **不能再靠 PFN 找页**。Linux **复用 PTE 位域** 存 **磁盘位置**：

| 概念 | 说明 |
|------|------|
| **`swp_entry_t`** | 编码 **type**（`swap_info` 数组 **索引** = 哪个 swap 区）+ **offset**（该区内 **slot 编号**） |
| **PTE 状态** | **not present** + **swap entry** — MMU fault → 内核知 **页在 swap 不在 RAM** |

```
换出前：PTE → present → PFN → struct page
换出后：PTE → !present → swp_entry(type, offset)
再访问：fault → read swap slot → 新 physical page → PTE present
```

→ [Ch 3 PTE present / young](../../chapter-03-page-table-management/)

---
