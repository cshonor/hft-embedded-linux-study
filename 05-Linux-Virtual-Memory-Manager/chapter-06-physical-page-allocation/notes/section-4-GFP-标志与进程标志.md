# Ch 6 §4 GFP 标志与进程标志 (GFP & Process Flags)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

### 4. GFP 标志与进程标志 (GFP & Process Flags)

### GFP (Get Free Pages)

调用 **`alloc_pages(gfp_mask, order)`** 时必须传 **`gfp_mask`** — 声明 **允许分配器做什么 / 不能做什么**：

| 标志（示例） | 含义 | 典型场景 |
|--------------|------|----------|
| **`GFP_KERNEL`** | 可 **睡眠** 等待页、可触发 **直接回收** | 进程上下文、多数内核路径 |
| **`GFP_ATOMIC`** | **不可睡眠** — 失败则立即返回 | **中断 / spinlock 持有** 中分配 |
| **`GFP_NOIO`** | 回收路径中 **不发起 I/O** | 避免 **存储栈重入死锁** |
| **`GFP_HIGHUSER` / `GFP_DMA`** 等 | 约束 **物理地址范围**、用户可映射等 | 设备 DMA、用户页 |

**HFT 关联：** 热路径若在 **不当上下文** 触发 **`GFP_KERNEL` 回收** → **毫秒级 stall**；用户态 **mlock** 是为减少 **fault 路径进 allocator + reclaim**。

### 进程标志（突破水位）

内存紧张、低于 **pages_min** 时，普通分配可能被 **同步回收** 拖慢；带特殊标志的进程可 **绕过常规水位限制**：

| 标志 | 谁 |
|------|-----|
| **`PF_MEMALLOC`** | **`kswapd`** 等回收路径 — 需 **继续拿页完成回收** |
| **`PF_MEMDIE`** | 正被 **OOM killer** 处理的进程 |

→ [Ch 13 OOM](../../chapter-13-out-of-memory-management/) · Ch 1 路线 **`oom_kill.c`**

---
