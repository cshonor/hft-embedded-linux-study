# Ch 11 §6 激活与停用交换区 (swapon / swapoff)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**
> 源码核验：Linux **v6.6**（`mm/swapfile.c`）

---

## 本节讲什么

本节回答：**`swapon` / `swapoff` 这两个系统调用分别做了什么？为什么 `swapoff` 极昂贵？**

原书已经点出了核心：swapon 简单、swapoff 要扫全表强制换入。v6.6 里这两个系统调用的**现代实现**更精确了——swapon 要校验 superblock magic、建 extent；swapoff 有"内存预检查 + OOM 保护 + 失败回滚"。本节落到源码。

---

## 1. 激活：`sys_swapon`（`mm/swapfile.c:2978`）

```c
SYSCALL_DEFINE2(swapon, const char __user *, specialfile, int, swap_flags)
{
    ...
    if (!capable(CAP_SYS_ADMIN))          /* 需要 CAP_SYS_ADMIN */
        return -EPERM;

    /* ① 打开文件/分区 */
    swap_file = ... file_open_name(pathname, O_RDWR|O_LARGEFILE, 0);

    /* ② 读 superblock：校验 magic */
    read_swap_header(p, swap_header, swap_file);   /* :2822 */
    /*   memcmp("SWAPSPACE2", swap_header->magic.magic, 10)  :2831 */

    /* ③ 建 swap_map + swap_extent */
    nr_extents = setup_swap_map_and_extents(p, swap_header, swap_map, ...);  /* :3126 */

    /* ④ swap_activate：文件系统 hook（文件 swap 走 bmap/直接 IO） */
    if (mapping->a_ops->swap_activate)
        mapping->a_ops->swap_activate(sis, swap_file, span);   /* :2264 */

    /* ⑤ 加入可用链表 */
    add_to_avail_list(p);                 /* 按 prio 插入 avail_lists[node] */
}
```

| 步骤 | 说明 |
|------|------|
| **① 打开** | swap 可以是**块设备**或**普通文件**（`SWP_BLKDEV` vs `SWP_FS_OPS`） |
| **② 读 superblock** | swap 区**第一页**存 `swap_header`，开头 10 字节是 magic `"SWAPSPACE2"`——`read_swap_header`（`:2831`）校验它，坏区直接拒 |
| **③ 建元数据** | `swap_map[]`（每 slot 计数）+ `swap_extent` 红黑树（文件 swap 的块映射） |
| **④ `swap_activate`** | 文件系统 hook：文件 swap 要锁定文件、预分配 extents、走直接 I/O |
| **⑤ 入链表** | 按 `prio` 插入 `swap_active_head` 和各节点的 `avail_lists[node]` |

---

## 2. 停用：`sys_swapoff`（`mm/swapfile.c:2388`）

swapoff 是 **swap 的逆向**，但难得多——因为它要**把换出去的数据全部换回来**：

```c
SYSCALL_DEFINE1(swapoff, const char __user *, specialfile)
{
    if (!capable(CAP_SYS_ADMIN))
        return -EPERM;

    /* ① 预检查：物理内存够不够装回所有换出的页？ */
    if (!security_vm_enough_memory_mm(current->mm, p->pages))
        vm_unacct_memory(p->pages);
    else {
        err = -ENOMEM;                   /* 不够 → 直接失败 */
        goto out_dput;
    }

    /* ② 从可用链表摘除，禁止再分配 */
    del_from_avail_list(p);
    p->flags &= ~SWP_WRITEOK;            /* 停止换出写入 */

    /* ③ 核心：强制换入所有仍被引用的页 */
    set_current_oom_origin();            /* OOM 保护：先杀这个进程 */
    err = try_to_unuse(p->type);         /* 扫所有进程 + 扫 swap_map */
    clear_current_oom_origin();

    if (err) {                           /* 失败 → 回滚，重新插回 */
        reinsert_swap_info(p);
        goto out_dput;
    }

    /* ④ 成功 → 释放 swap_map / extent / 关闭文件 */
}
```

### 关键：`try_to_unuse`（`:2038`）—— 双重扫描

```c
static int try_to_unuse(unsigned int type)
{
    if (!READ_ONCE(si->inuse_pages))
        return 0;                        /* 没人用，直接成功 */

retry:
    retval = shmem_unuse(type);          /* ① 先处理 shmem/tmpfs 的引用 */

    /* ② 遍历系统所有 mm（init_mm.mmlist），强制 swap in */
    p = &init_mm.mmlist;
    while (READ_ONCE(si->inuse_pages) && !signal_pending(current) &&
           (p = p->next) != &init_mm.mmlist) {
        mm = list_entry(p, struct mm_struct, mmlist);
        retval = unuse_mm(mm, type);     /* 扫该 mm 的页表，换回引用此区的页 */
        cond_resched();                  /* 别拖死交互 */
    }

    /* ③ 再扫 swap_map，清残留（如 swap cache 里的孤儿页） */
    i = 0;
    while (READ_ONCE(si->inuse_pages) &&
           (i = find_next_to_unuse(si, i)) != 0) {
        entry = swp_entry(type, i);
        folio = filemap_get_folio(swap_address_space(entry), i);
        folio_free_swap(folio);          /* 清 swap cache 残留 */
    }

    if (READ_ONCE(si->inuse_pages)) {    /* 还有 → 重试 */
        if (!signal_pending(current))
            goto retry;
    }
}
```

| 要点 | 说明 |
|------|------|
| **双重扫描** | 先扫**所有进程的页表**（`unuse_mm`→`unuse_pte`），再扫 **`swap_map[]`** 清 cache 残留 |
| **可中断** | `signal_pending(current)` 检查——swapoff 可被信号打断 |
| **无限重试** | 注释 `:2118-2123`：内存压力下换出页可能被**重新插回**，所以简单粗暴地 `goto retry` 一直试 |

---

## 3. 为什么 swapoff 极昂贵？

```
代价链：
  扫所有进程页表（mmlist 全遍历）
    → 每个引用该区的 PTE 都要 swap in（磁盘读！）
      → 换入需要物理内存
        → 内存不够 → 触发回收/换出 → 死循环风险
```

原书说"物理内存不够 → swapoff 失败"，v6.6 的实现是**前置预检查**（`security_vm_enough_memory_mm`，`:2429`）：先算"装回 `p->pages` 页需要多少内存"，不够直接 `-ENOMEM`，**不进入昂贵的 try_to_unuse**。

另外 `set_current_oom_origin()`（`:2461`）是**OOM 保护**：swapoff 过程中如果系统内存耗尽，OOM killer 会**优先杀 swapoff 这个进程**（它标记为 oom_origin），而不是乱杀业务进程——避免 swapoff 风暴把系统拖垮。

---

## 4. HFT / 嵌入式关联

| 场景 | 关联 |
|------|------|
| **`swapoff -a` 与 `mlock`** | 生产延迟机器常 `swapoff -a` 或干脆不配 swap；但要**先 `mlock` 再 `swapoff`**，否则 swapoff 本身会触发换入风暴 |
| **`try_to_unuse` 的在线风险** | 它是**全系统页表扫描 + 强制换入**，在线执行会导致毫秒级延迟尖刺——所以"避免 swapoff 在线"是铁律 |
| **预检查 vs 事后失败** | `security_vm_enough_memory_mm` 先算账再动手——HFT 里"先预检资源再提交"比"失败回滚"更稳 |

---

## 5. 衔接

- 上节 [§5 交换区读写与块 I/O](./section-5-交换区读写与块-I-O.md)
- 下节 [§7 swap_extent](./section-7-2.6-内核的新变化：swap_extent.md)：文件 swap 的块映射

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：swapon 校验的 `"SWAPSPACE2"` 是什么？为什么需要它？**
A：它是 swap 区 superblock 的 magic 值（`read_swap_header`，`swapfile.c:2831` `memcmp("SWAPSPACE2", ...)`），存在 swap 区第一页。swapon 用它**确认"这确实是个 swap 区"**，防止把普通分区/文件误当 swap 区激活（那会毁掉数据）。`mkswap` 工具负责写这个 magic。

**Q2：swapoff 的预检查 `security_vm_enough_memory_mm` 解决了什么问题？**
A：它提前判断"物理内存够不够装回所有换出的页（`p->pages`）"。原书只说"内存不够则失败"，v6.6 是**在进入昂贵的 `try_to_unuse` 之前**就检查，不够直接 `-ENOMEM`，避免扫完所有页表才发现装不回、白费大量 I/O。

**Q3：`try_to_unuse` 的"双重扫描"是哪两重？为什么需要两重？**
A：① 扫**所有进程的页表**（`unuse_mm`→`unuse_pte`），把引用该 swap 区的 PTE 强制换回；② 扫 **`swap_map[]`**，清掉 swap cache 里的孤儿页（有 cache 但已无 PTE 引用）。两重是因为"进程页表"和"swap cache"是两处独立记录引用的地方，都得清干净才能 `swapoff` 成功。

**Q4：`set_current_oom_origin()` 在 swapoff 里起什么作用？**
A：它把 swapoff 进程标记为 **OOM 优先被杀目标**。swapoff 过程会消耗大量内存（强制换入），若系统因此内存耗尽，OOM killer 应该先杀这个"始作俑者"（swapoff 进程），而不是误杀正常业务进程。这是对 swapoff 风暴的一种系统级保护。

**Q5：swapoff 失败后会怎样？swap 区会丢吗？**
A：不会。`try_to_unuse` 失败（或被信号打断）时，代码走 `reinsert_swap_info(p)` 把 swap 区**重新插回可用链表**，`goto out_dput` 返回错误。区还在，之前换出的数据也还在——swapoff 是原子的（要么全成、要么回滚），不会半途丢掉数据。

</details>
