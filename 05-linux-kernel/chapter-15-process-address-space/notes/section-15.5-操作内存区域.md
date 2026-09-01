## ⑤ 操作内存区域

内核频繁回答：**「虚拟地址 `addr` 属于哪一段 VMA？」** — **`find_vma` 族** 是 **缺页、系统调用、gup** 的入口。

#### `find_vma(struct mm_struct *mm, unsigned long addr)`

| 语义 | 返回 **第一个** 满足 **`vm_end > addr`** 的 VMA |
|------|--------------------------------------------------|
| 若 `vm_start <= addr < vm_end` | 该 VMA **覆盖** addr |
| 若 `vm_start > addr` | addr 在 **空洞** — **major fault 或 SIGSEGV** 前兆 |
| 无此类 VMA | **NULL** — 非法访问 |

```
addr ──► mm_rb 查找          ← LKD3rd（2.6 时代）
addr ──► mm_mt (maple tree)  ← v6.1+，find_vma 即 mt_find
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

#### `mmap_cache`（LKD3rd 时代的缓存）

| 字段 | **`mm->mmap_cache`** 指向最近使用的 VMA |
|------|------------------------------------------|
| 命中 | **跳过 rbtree** — O(1) |
| 失效 | **munmap**、**mmap 新段** 时更新 |

**缓存演化三段**（核对 v6.6 源码）：

| 阶段 | 方案 | 出处 |
|------|------|------|
| 2.6（LKD3rd） | 单条 `mmap_cache`（每 mm 一条） | 已删除 |
| ~3.16 | **vmacache**：每 task_struct 4 条 VMA 缓存（按地址哈希） | 已删除 |
| **v6.1+** | **不再需要缓存**——maple tree 本身足够快，查找即 `mt_find(&mm->mm_mt, &index, ULONG_MAX)`（v6.6 mm/mmap.c:1874） | 现行 |

> 教训：**在错误的数据结构上贴缓存膏药，不如换掉数据结构**。vmacache 维护成本（fork 时继承失效、上下文切换 flush）最终证明不划算。

#### per-VMA lock：缺页路径的锁粒度革命（v6.4+）

v6.6 `mm/memory.c:5431` 的 `lock_vma_under_rcu()` 实现了**缺页不抢全局锁**：

```
传统路径（LKD3rd / 无 CONFIG_PER_VMA_LOCK）:
    page fault → mmap_lock 读锁 → find_vma → handle_mm_fault
                 ↑ 所有线程的缺页串行经过同一把锁

per-VMA lock 路径（v6.6）:
    page fault → RCU 下 mt_find 找到 vma
               → vma_start_read(vma)   ← 只锁这一个 VMA（seqcount 读锁）
               → 校验 vma_start <= addr < vma_end 未变
               → handle_mm_fault       ← 不同 VMA 的缺页完全并行
    失败回退 → lock_mm_and_find_vma()（传统全局读锁路径，memory.c:5369）
```

| 对象 | LKD3rd | v6.6 |
|------|--------|------|
| 缺页持锁 | `mmap_sem` 全局读锁 | **单个 VMA** 的 `vm_lock`（RCU + seqcount） |
| 多线程同时缺页（不同区域） | 串行 | **并行** |
| `mmap`/`munmap`（写） | 写锁阻塞所有缺页 | 仍需全局 `mmap_lock` 写锁 + 目标 VMA 写锁 |

**HFT 直接受益**：多线程策略进程启动期并发 `MAP_POPULATE` 预热时，per-VMA lock 让各线程各自缺页互不踩脚。

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
| **v6.4+** | 缺页快路径可绕过全局锁——**per-VMA lock**（见上节） |
| **HFT** | 盘中 **无 mmap** → **无写锁竞争** |

#### GUP 与 pin：`get_user_pages` vs `pin_user_pages`（v5.6+ 分家）

核对 v6.6 `mm/gup.c`：两族 API 并存，**语义分工**不同——

| API 族 | 语义 | 适用 |
|--------|------|------|
| `get_user_pages`（GUP） | **短暂引用**页（get_page 计数） | 进程自己马上要读写（如 `copy_from_user` 深层路径） |
| **`pin_user_pages`**（gup.c:3370，内部 `FOLL_PIN`） | **长期独占**引用，专门给 **DMA** | **RDMA / DPDK / VFIO** 把用户页交给设备 |

> 为什么要分家：页可能被 `migrate`/`swap` 挪走。普通 GUP 拿到的引用**不阻止迁移**；DMA 引擎正对着那个物理地址写数据时页被搬走 = 数据写进错误的物理页。`FOLL_PIN` 标记让内存管理知道"这页被设备钉住了，别动"。**DPDK/RDMA 用户态零拷贝的根基就是 pin**——所以 HFT 工程师调 `rte_mlock`/`ibv_reg_mr` 失败时，排查方向就是 pin 权限（RLIMIT_MEMLOCK / CAP_IPC_LOCK）与页迁移冲突。

**HFT：** **`/proc/self/maps` 解析** 等价用户态 **audit** — 确认 **ring VMA 起止、 huge、 locked**。内核 **`find_vma` 成本** 在 **缺页路径** — **`MAP_POPULATE` + `mlock`** 让 **热路径无 fault** → **少碰 mm_mt**。

→ [Ch 15.8 缺页路径](./section-15.8-从访问到缺页概念.md) · [Ch 5 copy_from_user](../../chapter-05-system-calls/) · [06 Gorman 异常处理](../../../06-linux-mm/chapter-04-process-address-space/notes/section-4-异常处理与缺页异常.md)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** find_vma() 的工作原理？在什么场景下被调用？

<details><summary>答案</summary>

find_vma(mm, addr) 在红黑树中查找第一个 vma_end > addr 的 VMA。调用场景：1) 缺页处理（page fault handler 找 addr 所属 VMA → 判断是合法访问还是 SIGSEGV）；2) mmap/munmap 合并相邻 VMA；3) /proc/pid/maps 查找。如果 addr 不在任何 VMA 范围内 → 访问未映射地址 → SIGSEGV。

</details>

**Q2.** v6.6 的缺页路径如何避免拿全局 `mmap_lock`？

<details><summary>答案</summary>

`lock_vma_under_rcu()`（mm/memory.c:5431）：RCU 读临界区内用 maple tree 找到候选 VMA → `vma_start_read(vma)` 只对这个 VMA 加读锁（seqcount）→ 复核 addr 仍在 `[vm_start, vm_end)` 内 → 直接 `handle_mm_fault`。不同 VMA 上的缺页完全并行；失败才回退 `lock_mm_and_find_vma()` 走传统全局读锁。前提是 `CONFIG_PER_VMA_LOCK`（v6.4+）。

</details>

**Q3.** DPDK/RDMA 场景为什么要用 `pin_user_pages` 而不是 `get_user_pages`？

<details><summary>答案</summary>

普通 GUP 只拿临时引用，**不阻止页迁移/换出**；DMA 引擎按物理地址直接写内存，期间页被 migrate 挪走 = 数据落进错误的物理页（静默损坏）。`pin_user_pages`（v5.6+，内部置 `FOLL_PIN`）把页**钉住**，内存管理看到 FOLL_PIN 就不迁移/不换出该页。所以 `rte_mlock`/`ibv_reg_mr` 这类"把用户内存交给设备"的操作底层走 pin；排查其失败看 `RLIMIT_MEMLOCK`、`CAP_IPC_LOCK`。

</details>

</details>
---
