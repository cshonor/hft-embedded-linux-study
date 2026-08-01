# Ch 7 §3 释放非连续区域 (Freeing)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

### 3. 释放非连续区域 (Freeing)

**`vfree(addr)`** 大致步骤：

```
vfree
    → 扫描 vm_struct 链表，定位包含 addr 的 vm_struct
    → vmfree_area_pages()
           反向 walk 页表
           清除 PTE 映射
           free 每个物理页（回 Buddy）
           释放页表页（若不再使用）
    → 从链表移除 vm_struct，释放 guard 与描述符
```

**与 vmalloc 对称** — unmap + **`free_pages`** 逐页归还。

---
