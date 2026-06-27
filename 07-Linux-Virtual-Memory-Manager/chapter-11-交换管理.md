# Ch 11 交换管理 · Swap Management

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过**（HFT：**热路径应尽量避免 swap** — **`mlock` / 足够 RAM / `swapoff`**；理解本章可知 **swap fault 为何是毫秒~秒级**）

物理内存用尽时，Linux 把 **进程私有页 / 匿名页** **复制到后备存储 (backing storage)** — **交换区 (swap area)** — 腾出 **物理页框**（Ch 10 回收链的末端）。

**Swap 的两面作用（原书）：**

| 作用 | 说明 |
|------|------|
| **扩展有效虚拟内存** | 按需 **swap in** — 进程可用 VA 可大于物理 RAM |
| **腾出 RAM 给更热数据** | 换出 **冷匿名页**，把物理内存留给 **页缓存 / 磁盘缓冲** 等 |

**HFT：** 延迟敏感进程 **不应依赖 swap 扩容量** — swap I/O = **不可接受的 tail latency**。

→ 回收触发 swap：[Ch 10 §5](./chapter-10-页框回收.md#5-换出进程页面-swapping-out-process-pages) · swap-in fault：[Ch 4 §4.2](./chapter-04-进程地址空间.md#42-请求调页-demand-paging--从-swap-换回)

> **时代说明：** 原书 **`swap_info_struct`、`rw_swap_page()`** 等属 2.4/2.6 语境；现代主线 [`mm/swap*.c`](https://elixir.bootlin.com/linux/latest/source/mm/swap_state.c) · [`mm/page_io.c`](https://elixir.bootlin.com/linux/latest/source/mm/page_io.c) 等 — **PTE 存 swap entry、swap cache、cluster 分配** 思想不变。

---

## 本章在 VM 子系统中的位置

```
Ch 10 shrink 选中匿名页 victim
        ↓
Ch 11 分配 swap slot → 写盘 → PTE 改为 swap entry（not present）
        ↓
进程再访问 → Ch 4 page fault → swap in 读盘 → PTE present
```

---

## 1. 描述交换区 (Describing the Swap Area)

每个 **已激活** 的 swap 区（**分区或文件**）对应一个 **`swap_info_struct`**：

| 字段级信息 | 记录内容 |
|------------|----------|
| 状态、尺寸 | 区有多大、是否可用 |
| 优先级 | 多 swap 区时的 **使用顺序** |

**全局管理：** 静态数组 **`swap_info[MAX_SWAPFILES]`** — 原书 **`MAX_SWAPFILES = 32`**（地址空间 / 编码位数限制）。

**用户态：** **`swapon` / `swapoff`** · **`/proc/swaps`** 查看激活列表。

---

## 2. 映射 PTE 到交换项 (PTE ↔ Swap Entry)

页 **换出后** 物理页框归还 Buddy — **不能再靠 PFN 找页**。Linux **复用 PTE 位域** 存 **磁盘位置**：

| 概念 | 说明 |
|------|------|
| **`swp_entry_t`** | 编码 **type**（`swap_info` 数组 **索引** = 哪个 swap 区）+ **offset**（该区内 **slot 编号**） |
| **PTE 状态** | **not present** + **swap entry** — MMU fault → 内核知 **页在 swap 不在 RAM** |

```
换出前：PTE → present → PFN → struct page
换出后：PTE → !present → swp_entry(type, offset)
再访问：fault → read swap slot → 新 physical page → PTE present
```

→ [Ch 3 PTE present / young](./chapter-03-页表管理.md#页表项保护位与状态位-示例)

---

## 3. 分配交换槽 (Allocating Swap Slots)

在 swap 区内找 **空闲的一页大小 slot**。

### 簇 (Cluster · `SWAPFILE_CLUSTER`)

| 动机 | 做法 |
|------|------|
| 磁盘 **随机 seek 慢** | 尽量 **连续分配** 多个 slot（一 **cluster**） |
| 假设 | **同时换出的页** 很可能 **一起换入** — **相邻磁盘块** → **顺序 I/O** |

**HFT：** 若不幸发生 swap，**cluster** 只减轻 **磁盘** 侧 — 仍 **远慢于 RAM**。

---

## 4. 交换缓存 (Swap Cache) — **核心**

**问题：** **共享匿名页** — 多进程 PTE 指向 **同一物理页**；换出时 **不能** 低成本更新 **所有 PTE**（2.4 尤甚；2.6 **rmap** 改善 — Ch 3）。

**Swap Cache 角色：**

| 要点 | 说明 |
|------|------|
| **本质** | **页缓存特例** — `address_space` 为 **`swapper_space`** |
| **换出进行中** | 页 **仍在 swap cache** — 防 **写盘期间被修改**（**更新丢失**） |
| **释放条件** | **所有映射该页的 PTE** 已解绑 / 已改为 swap entry → 页才 **真正丢弃** |

```
共享匿名页换出
    rmap 解绑 / 更新 PTE → swap entry
    页在 swap cache 中完成 write
    引用计数归零 → free physical page
```

→ [Ch 10 swap cache 类型](./chapter-10-页框回收.md#2-页缓存-page-cache) · [Ch 3 rmap](./chapter-03-页表管理.md#反向映射-reverse-mapping--rmap--重点)

---

## 5. 交换区读写与块 I/O

| 方向 | 触发 | 路径 |
|------|------|------|
| **读 (swap in)** | **缺页 fault** — PTE 含 swap entry | 异步/同步读 swap → 填物理页 → 设 PTE present |
| **写 (swap out)** | **shrink** 选中脏/匿名 victim | 写 swap slot → PTE → swap entry |

**统一入口（原书）：** **`rw_swap_page()`** — 底层 **块 I/O** 栈。

**HFT：** **swap in fault** = **磁盘延迟 + 锁 + 页表更新** — **`/proc/vmstat` 的 `pgmajfault`** 上升时查 swap 活动。

---

## 6. 激活与停用交换区

### 激活 `sys_swapon()`

相对 **简单**：

```
打开 swap 文件/分区
    读磁盘 superblock / 元数据
    填充 swap_info_struct
    按优先级加入激活列表
```

### 停用 `sys_swapoff()` — **极昂贵**

```
不能丢弃 swap 里仍被 PTE 引用的数据
    → try_to_unuse()
        扫描 **所有进程** 页表
        找引用该 swap 区的 PTE
        **强制 swap in** 回物理内存
    物理内存不够 → swapoff **失败**
```

**HFT：** 生产 **latency 机器** 常 **`swapoff -a`** 或 **不配置 swap** — 避免 **silent swap** 与 **误操作 swapoff 风暴**。

---

## 7. 2.6 内核的新变化：`swap_extent`

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

## 相关章节

- 上一章：[chapter-10-页框回收.md](./chapter-10-页框回收.md)
- 下一章：[chapter-12-共享内存虚拟文件系统.md](./chapter-12-共享内存虚拟文件系统.md)
- 附录 K：[appendix-K-交换管理.md](./appendix-K-交换管理.md)

---
