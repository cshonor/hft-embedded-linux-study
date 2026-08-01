## ⑤ 操作内存区域

内核频繁回答：**「虚拟地址 `addr` 属于哪一段 VMA？」** — **`find_vma` 族** 是 **缺页、系统调用、gup** 的入口。

#### `find_vma(struct mm_struct *mm, unsigned long addr)`

| 语义 | 返回 **第一个** 满足 **`vm_end > addr`** 的 VMA |
|------|--------------------------------------------------|
| 若 `vm_start <= addr < vm_end` | 该 VMA **覆盖** addr |
| 若 `vm_start > addr` | addr 在 **空洞** — **major fault 或 SIGSEGV** 前兆 |
| 无此类 VMA | **NULL** — 非法访问 |

```
addr ──► mm_rb 查找
            │
            ├─ 命中 [vm_start, vm_end)  → 返回该 VMA
            └─ 落在 gap                 → 返回 next VMA 或 NULL
```

#### 相关 API

| 函数 | 作用 |
|------|------|
| **`find_vma_prev(mm, addr, &pprev)`** | 找 **前驱 VMA** — **`mprotect` 拆分** 时用 |
| **`find_vma_intersection(mm, start, end)`** | 找与 **[start, end)** **相交** 的 VMA — **`mmap` 冲突** |
| **`vma_link` / `vma_merge`** | 插入 / **合并相邻同属性 VMA** |
| **`get_user_pages`（GUP）** | 用户 VA → **pin 物理页** — **RDMA/DPDK** 方向 |

#### `mmap_cache`

| 字段 | **`mm->mmap_cache`** 指向最近使用的 VMA |
|------|------------------------------------------|
| 命中 | **跳过 rbtree** — O(1) |
| 失效 | **munmap**、**mmap 新段** 时更新 |

#### 典型调用链

| 调用者 | 场景 |
|--------|------|
| **`handle_mm_fault`** | **缺页** — 先 `find_vma` |
| **`access_process_vm`** | **ptrace / /proc mem** 读写 |
| **`do_mprotect`** | 改权限 — 可能 **split VMA** |
| **`copy_from_user`** | 先 **access_ok + 找 VMA** 验证可写 |

#### 锁：`mmap_lock`

| 事实 | 说明 |
|------|------|
| **读多写少** | 缺页路径 **读锁**；`mmap`/`munmap` **写锁** |
| **现代内核** | **`mmap_lock`** 替代旧 **`mmap_sem`** — 可 **乐观 spin** |
| **HFT** | 盘中 **无 mmap** → **无写锁竞争** |

**HFT：** **`/proc/self/maps` 解析** 等价用户态 **audit** — 确认 **ring VMA 起止、 huge、 locked**。内核 **`find_vma` 成本** 在 **缺页路径** — **`MAP_POPULATE` + `mlock`** 让 **热路径无 fault** → **少碰 mm_rb**。

→ [Ch 15.8 缺页路径](./section-15.8-从访问到缺页概念.md) · [Ch 5 copy_from_user](../../chapter-05-system-calls/) · [06 Gorman 异常处理](../../../../09-linux-mm/chapter-04-process-address-space/notes/section-4-异常处理与缺页异常.md)

---
