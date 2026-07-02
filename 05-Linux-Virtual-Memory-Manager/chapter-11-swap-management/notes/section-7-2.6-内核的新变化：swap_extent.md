# Ch 11 §7 2.6 内核的新变化：`swap_extent`

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

### 7. 2.6 内核的新变化：`swap_extent`

| 问题 | 2.6 方案 |
|------|----------|
| **swap file** 在磁盘上 **块不连续** | **`swap_extent`** — 记录 **连续 swap 页范围 ↔ 连续磁盘块范围** 的映射 |
| 文件 swap 性能差 | **extent** 使 I/O **更顺序** — 接近 **分区 swap** 效率 |

现代 **swap file** 仍依赖 **extent / 预分配** 等优化 — **文件 swap 可用但 HFT 仍不推荐**。

---

## Swap 全链路简图

```
匿名页 (进程私有)
    │
    ├─ 内存充足 → 常驻 RAM
    │
    └─ Ch 10 回收选中
           ├─ alloc swap slot (cluster)
           ├─ 入 swap cache → write swap
           ├─ PTE := swp_entry(type, offset)
           └─ free physical page

再次访问 VA
    → page fault
    → read swap → new page → PTE present
    → 用户指令重试（毫秒级延迟）
```

---

## HFT 精读 checklist

| 手段 | 目的 |
|------|------|
| **`mlock` / `mlockall(MCL_CURRENT\|MCL_FUTURE)`** | 匿名 RSS **不被换出** |
| **足够 RAM + 监控** | `si/so`（vmstat）、`pswpin/pswpout` |
| **`vm.swappiness=1` 或 0** | 降低 **匿名页** 被 swap 倾向（**不替代 mlock**） |
| **不用 swap 作「额外内存」** | swap 是 **回收手段**，不是 **HFT 堆扩展** |
| **避免 swapoff 在线** | `try_to_unuse` **全表扫描** |

**与 Ch 10 闭合：** **kswapd / direct reclaim** 选中 victim → **本章** 完成 **slot + I/O + PTE 编码** → fault 路径 **读回**。

---
