# Ch 11 §1 描述交换区 (Describing the Swap Area)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

### 1. 描述交换区 (Describing the Swap Area)

每个 **已激活** 的 swap 区（**分区或文件**）对应一个 **`swap_info_struct`**：

| 字段级信息 | 记录内容 |
|------------|----------|
| 状态、尺寸 | 区有多大、是否可用 |
| 优先级 | 多 swap 区时的 **使用顺序** |

**全局管理：** 静态数组 **`swap_info[MAX_SWAPFILES]`** — 原书 **`MAX_SWAPFILES = 32`**（地址空间 / 编码位数限制）。

**用户态：** **`swapon` / `swapoff`** · **`/proc/swaps`** 查看激活列表。

---
