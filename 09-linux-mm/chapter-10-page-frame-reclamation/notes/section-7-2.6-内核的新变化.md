# Ch 10 §7 2.6 内核的新变化

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

### 7. 2.6 内核的新变化

| 改进 | 说明 |
|------|------|
| **LRU 按 Zone 维护** | 2.4 **全局** active/inactive → 2.6 **每 `struct zone` 一套** — 与 NUMA/多 zone 回收平衡 |
| **Pageout pressure 衰减平均** | 不用简单 **priority 跳变** — **decaying average** 控制 **扫描强度** — 回收 **更平滑**、少 **突发** |
| **`pagevec` 批量 LRU** | 2.4 每次改 LRU **抢全局锁** → 2.6 **局部向量攒批** 再 **一次性** 链入/链出 — **降锁争用** |

**HFT 镜像：** **batch 更新共享结构**（pagevec）≈ 用户态 **批量提交 ring buffer**，少 **跨核锁**。

---

## 回收决策简图

```
shrink 扫描 inactive 尾部 page
        │
        ├─ Slab / 非 LRU 页？ → 跳过（Slab 自有 shrinker）
        ├─ mlocked？ → 跳过
        ├─ 文件页：干净 → free；脏 → writeback → free
        ├─ 匿名页：→ swap out（rmap unmap → swap cache）
        └─ 仍被频繁引用？ → 可能 rotate 回 active
```

---

## HFT 精读 checklist

| 手段 | 目的 |
|------|------|
| **`mlock` / `mlockall`** | 进程 **匿名/文件映射 RSS** 不被 swap、减少 **被 shrink 踢出** |
| **足够物理 RAM** | 避免 **kswapd / direct reclaim** 常转 |
| **`vm.swappiness=0`** 等 | 降低 **匿名页 swap 倾向**（不替代 mlock） |
| **监控 vmstat** | `pgmajfault`、`allocstall`、`pgscan_direct` |
| **避免热路径 `GFP_KERNEL` 大分配** | 减少 **分配触发的同步 reclaim** |
| **理解 page cache** | 行情 **mmap 只读** 可被 drop；**私有 dirty** 要 writeback |

**Gorman HFT 捷径终点：** Ch 2 → 3 → 8 → 4 → **Ch 10** — 「**内存为什么会抖**」的 **内核侧答案** 在本章与 Ch 6 水位、Ch 4 fault **闭合**。

---
