# Ch 10 §4 收缩各类缓存 (`shrink_caches`)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

### 4. 收缩各类缓存 (`shrink_caches`)

内存告急时 **不只缩 page cache**：

```
shrink_caches (概念)
    ├─ Slab shrinker（Ch 8 set_shrinker）
    ├─ dcache（目录项）
    ├─ icache（inode）
    └─ dqcache（磁盘配额）
```

**级联效应：** dentry/inode **对象本身小**，但释放后 **关联的 buffer / page cache 页** 可 **大量回落** — **间接腾页**。

---
