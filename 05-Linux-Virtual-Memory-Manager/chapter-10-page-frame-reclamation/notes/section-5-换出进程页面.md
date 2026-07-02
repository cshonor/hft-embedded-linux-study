# Ch 10 §5 换出进程页面 (Swapping Out)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

### 5. 换出进程页面 (Swapping Out)

| 页类型 | 回收方式 |
|--------|----------|
| **文件页 / 块缓冲** | 干净则 **直接丢**（仍可从文件再读）；脏则 **writeback** |
| **匿名页（进程堆栈堆 mmap 私有）** | 必须 **写入 swap** 才能腾物理页 |

### 2.4 痛点：`swap_out()` 扫全局页表

**无法** 从 **`struct page` 快速找所有 PTE** → **线性扫描各进程页表** 解绑 — **极慢**。

### 2.6+ rmap（Ch 3）

**`struct page` → PTE 链表** — **直接 unmap 所有映射** → 页进 **Swap Cache**，引用归零后 **真正 free**。

```
匿名页 swap out
    unmap PTEs (rmap)
    → swap cache
    → 写 swap 设备
    → 物理页归还 Buddy
```

→ 详 [Ch 11 交换管理](../../chapter-11-swap-management/)

---
