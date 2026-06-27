# Ch 10 页框回收 · Page Frame Reclamation

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读**（HFT：**kswapd / direct reclaim / 脏页回写** 是 **latency 尖刺** 主要来源之一 — 与 **`mlock`、足够 RAM、监控 vmstat** 强相关）

运行一段时间后，物理页框会被 **页缓存、slab、dentry/inode、进程映射页** 等占满。内核必须在 **彻底耗尽前** **挑选旧页回收**，腾出空间给 **新分配**（Ch 6 Buddy）。

> **时代说明：** 原书函数名 **`shrink_cache()`、`refill_inactive()`、`swap_out()`** 等多属 **2.4/2.6**。现代主线为 **`shrink_page_list()` / `shrink_lruvec()`、`balance_pgdat()`** 等（[`mm/vmscan.c`](https://elixir.bootlin.com/linux/latest/source/mm/vmscan.c)）— **active/inactive LRU、kswapd、zone 局部 LRU** 思想不变。

→ 缺页与 swap-in：[Ch 4](./chapter-04-进程地址空间.md#4-异常处理与缺页异常-page-faulting) · rmap 加速解映射：[Ch 3 §7](./chapter-03-页表管理.md#反向映射-reverse-mapping--rmap--重点)

---

## 本章在 VM 子系统中的位置

```
Ch 6 alloc_pages 需要 free 页
        ↑
Ch 10 回收：LRU 选 victim → 丢弃 / 写回 / swap out
        ↑
Ch 2 zone 水位 pages_low/min → 唤醒 kswapd 或 direct reclaim
Ch 11 swap：匿名页换出细节
```

**HFT 核心句：** **`GFP_KERNEL` 分配 + 内存紧** → 调用方可能 **同步进 shrink** — **毫秒~秒级 stall**；**`mlock`/`mlockall`** 把 **进程页** 移出 **可换出候选**，但不免疫 **全局 reclaim 对 page cache 的压力**（通常影响较小若 RSS 已钉住）。

---

## 1. 页面替换策略 (Page Replacement Policy)

Linux **不用纯 LRU**，而用 **双链表** 近似 **LRU 2Q + clock**：

| 链表 | 角色 |
|------|------|
| **`active_list`** | **工作集** — 近期频繁访问的页 |
| **`inactive_list`** | **回收候选** — 适合被踢出 |

**目标：** active 容纳 **各进程工作集**；inactive 供 **扫描回收** — 在 **近似 LRU** 与 **实现开销** 之间折中。

→ [Ch 2 §2.6 LRU 本地化](./chapter-02-描述物理内存.md#lru-链表本地化) — **2.6 起 per-zone active/inactive**（非全局一条链）。

**`PG_active` / `PG_referenced` / accessed 位**（Ch 2 page flags · Ch 3 PTE young）— 驱动 **在 active ↔ inactive 间迁移**。

---

## 2. 页缓存 (Page Cache)

**目的：** 避免 **重复读盘** — 磁盘数据 **缓存于 RAM**。

| 页缓存中的页类型（原书） | 来源 |
|--------------------------|------|
| **文件 mmap 缺页** | 映射文件读入 |
| **块设备缓冲** | `read()` / buffer head 路径 |
| **Swap cache 匿名页** | 已换出或换出过程中的匿名页 |
| **共享内存页** | shm / mmap shared |

**索引：** **`struct address_space` + 文件内 offset** → **哈希表** 快速定位 **同一文件同一偏移** 的 `struct page`。

**HFT：** 大 **内存映射行情文件** 会占 **page cache** — 与 **进程 RSS** 分开统计；**drop_caches** 可回收 **clean page cache**，**不** 换出 **mlock 页**。

---

## 3. LRU 链表管理

除 **Slab 占用的页** 外，**在用页** 多挂在 **LRU** 上供扫描。

### `refill_inactive()`

定期把页从 **active 尾部** 移到 **inactive** — 保持 **active ≈ 总页缓存的 ~2/3**（原书比例），**inactive** 供 **shrink** 扫描。

### `shrink_cache()`（现代 `shrink_page_list` 等）

从 **inactive 尾部** 取 victim，按状态分支：

| 页状态 | 典型动作 |
|--------|----------|
| **干净 + 无映射 / 可丢** | 直接 **free** 回 Buddy |
| **脏页 (PG_dirty)** | **写回** 磁盘再释放 |
| **仍被映射** | 需 **解 PTE** — 文件页可丢映射；**匿名页** → **swap**（§5） |
| **仍被 pin / 锁定** | **跳过** |

---

## 4. 收缩各类缓存 (`shrink_caches`)

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

## 5. 换出进程页面 (Swapping Out)

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

→ 详 [Ch 11 交换管理](./chapter-11-交换管理.md)

---

## 6. 页面换出守护进程 (`kswapd`)

| 属性 | 说明 |
|------|------|
| **创建时机** | 系统启动 |
| **平时** | **睡眠** |
| **唤醒** | 某 zone 空闲页 **< `pages_low`**（Ch 2 水位） |
| **工作** | 回收直到 free **回到 ~`pages_high`** |
| **同步路径** | 极端压力下，**普通分配路径**（`GFP_KERNEL`）**同步执行部分 reclaim** — **direct reclaim** |

```
free pages
    high ─── 正常
     low ─── wake kswapd（异步）
     min ─── alloc 路径同步 reclaim（调用方阻塞）
```

**HFT：** 监控 **`allocstall`、`pgscan_*`、`kswapd_*`**（`/proc/vmstat`）；latency 尖刺常与 **direct reclaim + 写盘** 同现。

→ [Ch 6 GFP_KERNEL](./chapter-06-物理页分配.md#4-gfp-标志与进程标志-gfp--process-flags) · [Ch 13 OOM](./chapter-13-内存耗尽管理.md)

---

## 7. 2.6 内核的新变化

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

## 相关章节

- 上一章：[chapter-09-高端内存管理.md](./chapter-09-高端内存管理.md)
- 下一章：[chapter-11-交换管理.md](./chapter-11-交换管理.md)
- 附录 J：[appendix-J-页框回收.md](./appendix-J-页框回收.md)
- Zone 水位：[chapter-02-描述物理内存.md#区域水位线-zone-watermarks)

---
