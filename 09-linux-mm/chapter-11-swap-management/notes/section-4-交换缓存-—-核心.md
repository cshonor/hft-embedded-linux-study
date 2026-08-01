# Ch 11 §4 交换缓存 (Swap Cache) — **核心**

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

### 4. 交换缓存 (Swap Cache) — **核心**

**问题：** **共享匿名页** — 多进程 PTE 指向 **同一物理页**；换出时 **不能** 低成本更新 **所有 PTE**（2.4 尤甚；2.6 **rmap** 改善 — Ch 3）。

**Swap Cache 角色：**

| 要点 | 说明 |
|------|------|
| **本质** | **页缓存特例** — `address_space` 为 **`swapper_space`** |
| **换出进行中** | 页 **仍在 swap cache** — 防 **写盘期间被修改**（**更新丢失**） |
| **释放条件** | **所有映射该页的 PTE** 已解绑 / 已改为 swap entry → 页才 **真正丢弃** |

```
共享匿名页换出
    rmap 解绑 / 更新 PTE → swap entry
    页在 swap cache 中完成 write
    引用计数归零 → free physical page
```

→ [Ch 10 swap cache 类型](../../chapter-10-page-frame-reclamation/notes/section-2-页缓存.md#2-页缓存-page-cache) · [Ch 3 rmap](../../chapter-03-page-table-management/)

---
