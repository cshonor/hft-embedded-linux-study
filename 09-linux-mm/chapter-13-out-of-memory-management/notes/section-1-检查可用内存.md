# Ch 13 §1 检查可用内存 (`vm_enough_memory`)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

### 1. 检查可用内存 (`vm_enough_memory`)

在 **可能大幅消耗 VA/RSS** 的操作 **之前** **主动检查** — 尽量避免滑入 OOM：

| 触发场景（例） | 系统调用 / 路径 |
|----------------|-----------------|
| 扩展堆 | **`brk()`** |
| 扩大映射 | **`mremap()`** |

**`vm_enough_memory()`** 估算 **潜在可用内存** 是否 ≥ 本次请求：

| 计入（原书） | 说明 |
|--------------|------|
| **页缓存** 中可回收部分 | clean cache 等 |
| **空闲物理页** | Buddy free |
| **空闲 swap 槽** | 尚未承诺的 swap |
| **未用 dcache / inode cache** | 可 shrink（Ch 10 §4） |

**Overcommit：** 若管理员允许 **内存超额分配**（**`vm.overcommit_memory`** 等），检查 **更松** — **承诺的 VA** 可大于 **物理+swap**，**OOM 风险上升**。

**HFT：** **`overcommit_memory=2`** + 合理 **`overcommit_ratio`** 更 **可预测** — 仍 **不能替代** 物理 RAM + **`mlock`**。

---
