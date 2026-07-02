# Ch 2 §2 内存区域 (Zones)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**

### 2. 内存区域 (Zones)

每个 **Node** 再划分为多个 **区域 (Zone)**。典型 x86 32 位划分：

| Zone | 用途（简） |
|------|------------|
| **`ZONE_DMA`** | 低端物理地址，供 **ISA DMA** 等必须落在有限物理范围的设备 |
| **`ZONE_NORMAL`** | 内核 **直接线性映射** 可常态访问的「普通」物理页 |
| **`ZONE_HIGHMEM`** | **32 位** 上超出内核直接映射窗口的物理内存（见 §4） |

每个 Zone 由 **`zone_t`**（2.6 起 **`struct zone`**）描述。

### 区域水位线 (Zone Watermarks)

页面 **分配** 与 **回收** 的触发器 — 每个 Zone 有三个关键水位：

| 水位 | 含义 | 典型后果 |
|------|------|----------|
| **`pages_high`** | 空闲页较多 | 正常分配 |
| **`pages_low`** | 空闲页偏少 | 唤醒 **`kswapd`** 后台回收 |
| **`pages_min`** | 空闲页紧张 | 分配路径上 **同步回收**（**direct reclaim**）— 调用方可能被拖慢 |

```
空闲页数量
    high ─────────────────  正常
     low ─────────────────  kswapd 开始干活
     min ─────────────────  分配器同步回收（延迟尖刺）
```

**HFT 关联：** 即使用 `mlock`，系统其他部分仍可能因 **全局内存压力** 触发回收路径；绑 NUMA + 预分配 + 监控 **direct reclaim**（`/proc/vmstat` 等）是常见调优项。

### 等待队列表 (Wait Queue Table)

页在进行 **I/O**（换入/换出、块设备读写）时会被 **锁定**（`PG_locked`）。等该页的进程需 **睡眠**。

- **不为每个 page 单独建等待队列**（太费内存）
- 在 **Zone** 里维护 **哈希形式的 wait queue table** — 多个 page 共享桶式等待结构

→ 读回收 / swap 时会反复碰到：**页 locked → 在 zone 等待队列上 sleep → I/O 完成唤醒**。

---
