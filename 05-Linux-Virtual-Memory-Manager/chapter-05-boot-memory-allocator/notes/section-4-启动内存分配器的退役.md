# Ch 5 §4 启动内存分配器的退役 (Retiring)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

### 4. 启动内存分配器的退役 (Retiring)

当 **`start_kernel()` 路径后期** 可以安全运行 **常规分配器** 时，**bootmem 退役**：

| 步骤 | 说明 |
|------|------|
| 各 arch 提供 **`mem_init()`** | 架构相关的收尾 + 调用通用逻辑 |
| **遍历 bootmem 位图** | 找出 **仍标记为空闲** 的物理页 |
| **清除保留标记** | 含 **保存位图本身** 的页 — 也还给运行时 |
| **全部交给 Buddy** | **`page_alloc.c`** 构建 **zone freelist** — Ch 6 正式接管 |

```
bootmem 位图:  used used free free used …
                    └────────┘
                         ↓ mem_init()
              Buddy freelist (ZONE_NORMAL, …)
```

**之后：** `kmalloc`、`page_alloc`、`mmap` fault 等 **不再走 bootmem** — 用户态进程 **永远不会** 触达此路径。

→ **下一章精读：** [../../chapter-06-physical-page-allocation/](../../chapter-06-physical-page-allocation/)

---
