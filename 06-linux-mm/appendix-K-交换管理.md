# 附录 K 交换管理 · Swap Management

> **Code Commentary** · Mel Gorman · **选读** · 源码核验：Linux v6.6（`mm/swapfile.c` + `mm/swap_state.c` + `mm/page_io.c`）

概念总览 → [./chapter-11-swap-management/](./chapter-11-swap-management/)

---

## 本节走读什么

原书附录 K 走读 **swap 区激活 / slot 分配 / swap cache / 换入换出 I/O**。本附录按「**换出路径**」和「**换入路径**」两条主线，走读 v6.6 的三个文件：`swapfile.c`（slot 管理）、`swap_state.c`（swap cache）、`page_io.c`（实际 I/O）。

---

## 1. 换出路径：匿名页 → swap 设备

```
shrink_folio_list()                          // vmscan.c:1705（附录 J）
   └─ add_to_swap(folio)                     // swap_state.c:86 写入 swap cache
        └─ get_swap_pages(n_goal, entries)   // swapfile.c:1047  分配 slot
             └─ scan_swap_map_slots()        // :799  扫描 slot 位图
                  └─ swap_alloc_cluster()    // :1002 按簇分配
   └─ swap_writepage(page)                   // page_io.c:179
        └─ __swap_writepage(page)            // :371  ← 三路分叉（见 §3）
```

**核心数据 `swp_entry_t`**：换出后 PTE 的 `present=0`，剩余 64 位被复用为 `{type 高5位, offset 低59位}`（`SWP_TYPE_SHIFT = BITS_PER_XA_VALUE - MAX_SWAPFILES_SHIFT`，swapops.h:27）——**offset 指向 swap 区内的第几个 slot**。

**核心数据 `struct swap_info_struct`**（swap.h:282）：

| 字段 | 作用 |
|------|------|
| `swap_map[]` | 每个 slot 的引用计数（0=空闲，>0=被引用，`SWAP_MAP_MAX`=换出共享页） |
| `cluster_info` | 簇位图（HDD 连续分配优化） |
| `percpu_cluster` | per-CPU 当前簇（SSD 随机写优化） |
| `swap_extent_root` | 红黑树（swap 区在磁盘上可能非连续） |
| `bdev` / `flags` | 块设备 / `SWP_*` 标志 |

**slot 分配按介质分叉**：`scan_swap_map_slots`（:799）里，HDD 用 `SWAPFILE_CLUSTER`（默认 256 页，:277；开 THP_SWAP 时为 `HPAGE_PMD_NR`，:273）把连续 slot 聚成簇以减少磁头寻道；SSD 则设 `SWP_SOLIDSTATE`（:3081），用 **per-CPU 簇**（`percpu_cluster`，:617）让每个 CPU 在各自簇内分配，避免跨 CPU 抢全局锁。

---

## 2. swap cache 与换入路径

```
do_swap_page()                               // 缺页时读到 swp_entry_t（mm/memory.c）
   └─ lookup_swap_cache(entry)               // swap_state.c 命中？直接返回
        └─ 未命中 → __read_swap_cache_async() // :412 预读 + 读入 swap cache
   └─ swap_readpage(page, sync, plug)        // page_io.c:493 实际读盘
```

**swap cache 结构**：`swapper_spaces[MAX_SWAPFILES]`（swap_state.c:39）是**每个 swap 区一个 `struct address_space`**。`add_to_swap_cache`（:86）把 folio 挂进 xarray（`i_pages`），`delete_from_swap_cache`（:233）换入完成后摘除。

**走读要点**：swap cache 的存在是为了**并发换入**——多个进程换入同一 swap 页时，只有一个读盘，其余等 cache 命中。`swap_cluster_readahead`（:620）按簇预读、`swap_vma_readahead`（:780）按 VMA 顺序预读，都是降低缺页延迟的手段。

---

## 3. 换出 I/O 三分支：`__swap_writepage`（page_io.c:371）

```c
__swap_writepage(page, wbc)
   if (sis->flags & SWP_FS_OPS)          // swap 文件（而非块设备）
        → 走文件系统写回路径
   else if (sis->flags & SWP_SYNCHRONOUS_IO)  // zram / 同步 swap
        → 同步写，不排队
   else                                   // 普通块设备 swap
        → 异步 bio 提交，bio->bi_opf |= REQ_SWAP
```

**走读要点**：`SWP_SYNCHRONOUS_IO`（:3075）是 **zram** 这类内存 swap 的标志——无需排队，同步完成，这也是 zram 延迟远低于磁盘 swap 的机制原因。普通 swap 则走异步 `bio`，标记 `REQ_SWAP`。

---

## 4. swap extent：磁盘非连续映射

swap 区在磁盘上可能**不连续**（swap 文件尤其如此），用**红黑树**组织：

```
offset_to_swap_extent(sis, offset)          // swapfile.c:211  offset→extent 查询
add_swap_extent(sis, start_page, ...)       // :2184  插入一个 extent
destroy_swap_extents(sis)                   // :2157  销毁整棵树
```

`struct swap_extent` 记录「swap 区内 offset 范围 ↔ 磁盘 block 范围」的映射（`swap_extent_root` 红黑树，:2745 初始化）。**走读要点**：这是 swap 区抽象与具体块设备之间的**寻址翻译层**，让上层 slot 分配无需关心磁盘布局。

---

## 5. swapon / swapoff

`__do_sys_swapon` → `alloc_swap_info`（:2701）分配 `swap_info_struct` → `claim_swapfile`（:2762）声明文件 → `setup_swap_info`（:2292）初始化 → `enable_swap_info`（:3184）挂进全局 swap 表。`si_swapinfo`（:3240）供 `/proc/meminfo` 统计 `SwapTotal`/`SwapFree`。

---

## 与正文对应

| 附录内容 | 正文落点 |
|----------|----------|
| `swp_entry_t` 位域 | Ch11 §1（换出 PTE 复用） |
| `swap_info_struct` | Ch11 §2（swap 区描述符） |
| slot 分配（HDD/SSD 分叉） | Ch11 §3（簇分配） |
| swap cache | Ch11 §4（`swapper_spaces`） |
| `__swap_writepage` 三分支 | Ch11 §5（换出 I/O） |

---

## HFT / 嵌入式关联

| 手段 | 落点 |
|------|------|
| 关 swap 或用 zram | HFT 禁用磁盘 swap（换出是缺页延迟的元凶），必要时用 zram（`SWP_SYNCHRONOUS_IO` 同步完成） |
| 嵌入式存储寿命 | 频繁换出会磨损 eMMC/SSD，`SWP_SOLIDSTATE` + per-CPU 簇减少写放大 |
| `vm.swappiness` | 调低减少匿名页换出，与附录 J 联动 |

---

## 相关章节

- 上一章：[appendix-J-页框回收.md](./appendix-J-页框回收.md)
- 下一章：[appendix-L-共享内存虚拟文件系统.md](./appendix-L-共享内存虚拟文件系统.md)

---

<details>
<summary>自测 5 问（点开看答案）</summary>

**Q1：`swp_entry_t` 如何复用 PTE？**

换出后 PTE 的 `present=0`，64 位被复用为 `{type 高5位, offset 低59位}`，offset 指向 swap 区内的 slot 编号（swapops.h:27 定义 `SWP_TYPE_SHIFT`）。

**Q2：HDD 和 SSD 的 slot 分配策略有何不同？**

HDD 用 `SWAPFILE_CLUSTER`（256 页，:277）聚簇分配减少寻道；SSD 设 `SWP_SOLIDSTATE`（:3081），用 per-CPU 簇（`percpu_cluster`）避免跨 CPU 抢锁并减少写放大。

**Q3：swap cache 解决什么问题？**

并发换入。`swapper_spaces[]`（swap_state.c:39）为每个 swap 区维护 `address_space`，多个进程换入同一页时只有一个读盘，其余命中 cache 等待。

**Q4：`__swap_writepage` 的三路分叉？**

`SWP_FS_OPS`（swap 文件）→ 文件系统写回；`SWP_SYNCHRONOUS_IO`（zram 等）→ 同步写不排队；其他（普通块设备）→ 异步 bio（`REQ_SWAP`）。

**Q5：zram 为什么比磁盘 swap 快？**

zram 是内存 swap，设 `SWP_SYNCHRONOUS_IO`（:3075），写回走同步路径无需排队，且无磁盘寻道/块设备延迟。

</details>
