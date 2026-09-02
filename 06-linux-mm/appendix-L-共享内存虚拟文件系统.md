# 附录 L 共享内存虚拟文件系统 · Shared Memory Virtual Filesystem

> **Code Commentary** · Mel Gorman · **选读** · 源码核验：Linux v6.6（`mm/shmem.c`，4932 行）

概念总览 → [./chapter-12-shared-memory-virtual-filesystem/](./chapter-12-shared-memory-virtual-filesystem/)（**`mm/shmem.c`**）

---

## 本节走读什么

原书附录 L 走读 **shmem 虚拟文件系统**。shmem 是 **tmpfs** 的底层实现，同时也是 **System V 共享内存（`shmget`）与 POSIX 共享内存（`shm_open`）的后端**。它的核心特征是「**页缓存 + swap 备份**」的混合：常驻内存时是普通页缓存，内存不足时换出到 swap。本附录走读它的**缺页处理**、**换出路径**与**三种创建入口**。

---

## 1. 核心：`shmem_get_folio_gfp`（:1927）

shmem 的所有页操作都汇聚到这一个函数——**查找 → 命中 / 换入 / 分配**三态：

```
shmem_get_folio_gfp(inode, index, &folio, sgp, gfp, ...)   // :1927
   ├─ folio = filemap_get_entry(mapping, index)            // xarray 查找
   ├─ xa_is_value(folio)?                                  // 是个 swap entry？
   │     └─ shmem_swapin_folio()                           // :1813 从 swap 换入
   ├─ folio 命中且 uptodate?                               // 直接返回
   ├─ 否则（SGP_READ 空洞 → 返回 NULL 让调用方清零）
   └─ shmem_alloc_folio(gfp, ...)                          // :1651 分配新页
         └─ shmem_add_to_page_cache(folio, ...)            // :761  挂进 xarray
```

**`enum sgp_type`** 控制行为：`SGP_CACHE`（页缓存读写）、`SGP_READ`（只读，空洞返回 NULL）、`SGP_WRITE`（写，空洞分配）、`SGP_NOALLOC`（不分配）。

**走读要点**：shmem 的「未命中但 swap 有条目」是 `xa_is_value` 判断的——xarray 里存的是 `swp_entry_t`（值类型）而非 folio 指针，这就是「页被换出」的状态。换入走 `shmem_swapin_folio`（:1813）。

---

## 2. 缺页处理：`shmem_fault`（:2159）

```
do_anonymous_page 之外的共享匿名页缺页 → shmem_fault(vmf)   // :2159
   └─ shmem_get_folio_gfp(..., SGP_CACHE, ...)              // :2227
        └─ 命中 → vmf->page = folio_file_page(folio, pgoff) // :2232
```

**走读要点**：`shmem_fault` 是 `vm_ops->fault` 回调，在进程访问共享内存页时被 `__handle_mm_fault` 调用。它是「把共享内存页映射进 VMA」的入口，与匿名页的 `do_anonymous_page` 平级。**关键差异**：匿名页换出后 PTE 直接存 `swp_entry_t`；而 shmem 页换出后，**PTE 是空，条目存在 inode 的 xarray 里**——所以换入要先查 xarray 而非 PTE。

---

## 3. 换出路径：`shmem_writepage`（:1422）

shmem 的写回**只在内存回收时发生**：

```c
shmem_writepage(page, wbc)                     // :1422
   if (!wbc->for_reclaim)  goto redirty;       // 非回收场景直接 re-dirty
   if (VM_LOCKED || noswap) goto redirty;      // mlock 或禁 swap → 不换
   if (!total_swap_pages)   goto redirty;      // 无 swap 空间 → 不换
   // 大页需先 split（:1453）
   → 分配 swap entry，写回 swap（复用 page_io 路径）
```

**走读要点**：`shmem_writepage` 只在 `wbc->for_reclaim`（回收触发）时执行，**不会被 writeback 线程或 sync 触发**（tmpfs 没有真实磁盘文件）。这就是 shmem「内存里是页缓存，压力大了就换 swap」的双面性。`VM_LOCKED` 的共享内存页绝不换出——对应 `mlock` 语义。

---

## 4. 三种创建入口

| 入口 | 落点 | 场景 |
|------|------|------|
| tmpfs mount | `shmem_get_inode`(:2514) | `mount -t tmpfs`，普通文件语义 |
| System V shm | `shmem_file_setup`(:4828) | `shmget()` 返回匿名 shmem 文件 |
| POSIX shm | 同上（`shm_open` → tmpfs 挂载点） | `/dev/shm` |

`shmem_file_setup`（:4828）是**内核内部创建匿名 shmem 文件**的入口——`ipc/shm.c` 的 `shmget` 最终调它。`shmem_fallocate`（:3026）支持 hole punch / 预分配，`shmem_truncate_range`（:1112）截断并回收页。

---

## 与正文对应

| 附录内容 | 正文落点 |
|----------|----------|
| `shmem_get_folio_gfp` 三态 | Ch12 §2（页缓存 + swap 备份） |
| `shmem_fault` 缺页 | Ch12 §3（共享页映射） |
| `shmem_writepage` 换出 | Ch12 §4（回收时换 swap） |
| 三种创建入口 | Ch12 §5（tmpfs / SysV / POSIX） |

---

## HFT / 嵌入式关联

| 手段 | 落点 |
|------|------|
| 共享内存通信 | HFT 进程间共享行情/订单簿用 `shmget`/`shm_open`——本质是 shmem，**零拷贝**（只映射不拷贝） |
| `mlock` 共享内存 | 换出会引入缺页延迟，关键共享段 `mlock` 防 `shmem_writepage` 换出 |
| 内存开销 | shmem 页计入进程 RSS 也计入 tmpfs，`/proc/meminfo` 的 `Shmem` 行可观察总量 |

---

## 相关章节

- 上一章：[appendix-K-交换管理.md](./appendix-K-交换管理.md)
- 下一章：[appendix-M-内存耗尽管理.md](./appendix-M-内存耗尽管理.md)

---

<details>
<summary>自测 5 问（点开看答案）</summary>

**Q1：shmem 是什么的双重身份？**

既是 tmpfs 的底层实现，也是 System V 共享内存（`shmget`）和 POSIX 共享内存（`shm_open`）的后端——本质是「页缓存 + swap 备份」的混合体。

**Q2：`shmem_get_folio_gfp` 的三态？**

xarray 命中 → 返回 folio；xarray 里是 swap entry（`xa_is_value`）→ `shmem_swapin_folio` 换入；空洞 → 依 `sgp_type` 决定分配新页还是返回 NULL。

**Q3：shmem 页换出后，PTE 里是什么？**

PTE 是空的（present=0 且无 swp_entry），条目存在 inode 的 xarray 里。所以换入要先查 xarray（`filemap_get_entry`），这与匿名页换出后 PTE 直接存 `swp_entry_t` 不同。

**Q4：`shmem_writepage` 什么时候触发？**

只在 `wbc->for_reclaim`（内存回收）时，且满足「非 VM_LOCKED、非 noswap、有 swap 空间」才真正换出。writeback 线程和 sync 都不会触发它。

**Q5：`shmem_file_setup`（:4828）被谁调用？**

`ipc/shm.c` 的 `shmget` 最终调用它，创建匿名 shmem 文件作为 System V 共享内存的后端。

</details>
