# Ch 13 §1 检查可用内存（`__vm_enough_memory` → OOM 触发链）

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪** · 源码核验：Linux v6.6

---

## 本节讲什么

原书把「检查可用内存」当作 OOM 的第一道防线：在 `brk()`、`mremap()` 这类**可能大幅消耗 VA/RSS** 的操作**之前**主动调用 `vm_enough_memory()`，尽量让系统**滑不进 OOM killer**。

到 v6.6，这道防线依然存在，但**角色已经分化**成两件完全独立的事：

1. **`__vm_enough_memory()`（mm/util.c:931）** —— 只在 **overcommit 记账**层面做检查，失败直接返回 `-ENOMEM` 让系统调用**提前失败**，**根本不触发 OOM killer**。
2. **真正的 OOM killer 入口**是 `__alloc_pages_may_oom()`（page_alloc.c:3273）→ `out_of_memory()`（oom_kill.c:1103）—— 在**页分配器**已经走完所有回收手段仍失败时才调用。

理解这两条链的分野，是读懂 Ch13 的前提：**前者是「记账前移、提前拒绝」，后者是「用尽之后、暴力止损」。**

---

## 1. overcommit 三策略（`sysctl_overcommit_memory`）

`__vm_enough_memory` 的行为由 `vm.overcommit_memory` 决定，三种取值（mm/util.c:803 起）：

| 取值 | 宏 | 语义 | `__vm_enough_memory` 的处理（:931） |
|------|-----|------|-------------------------------------|
| `0` | `OVERCOMMIT_GUESS`（**默认**） | 启发式：单次请求「别太离谱」就放行 | `pages > totalram_pages() + total_swap_pages` 才拒绝（:943） |
| `1` | `OVERCOMMIT_ALWAYS` | 永不拒绝 | 直接 `return 0`（:940） |
| `2` | `OVERCOMMIT_NEVER` | 严格按 `vm_commit_limit()` 记账 | `vm_committed_as < allowed` 才放行（:987） |

**默认是 GUESS（:803）**，这是关键认知——绝大多数 Linux 机器跑在「半开」的 overcommit 上，所以**内存真的会被承诺过头**，OOM killer 不是「理论上不该发生」而是「设计上就允许发生的兜底」。

```c
int sysctl_overcommit_memory __read_mostly = OVERCOMMIT_GUESS;   // mm/util.c:803
int sysctl_overcommit_ratio  __read_mostly = 50;                 // :804  默认 50%
unsigned long sysctl_overcommit_kbytes __read_mostly;            // :805  0=用 ratio 算
unsigned long sysctl_user_reserve_kbytes  __read_mostly = 1UL << 17; // :807  128MB
unsigned long sysctl_admin_reserve_kbytes __read_mostly = 1UL << 13; // :808  8MB
```

## 2. `vm_commit_limit()`：NEVER 模式的上限（mm/util.c:875）

```c
unsigned long vm_commit_limit(void)
{
    unsigned long allowed;
    if (sysctl_overcommit_kbytes)                              // 绝对 KB 优先
        allowed = sysctl_overcommit_kbytes >> (PAGE_SHIFT - 10);
    else
        allowed = ((totalram_pages() - hugetlb_total_pages())  // 扣掉大页
                   * sysctl_overcommit_ratio / 100);
    allowed += total_swap_pages;                               // 加上 swap
    return allowed;
}
```

公式（`overcommit_kbytes=0` 时）：

```
commit_limit = (totalram - hugetlb) × ratio% + total_swap
```

默认 `ratio=50` → 物理内存的一半 + 全部 swap。这就是 HFT 里常说的「`overcommit_memory=2` + `overcommit_ratio` 精确控额」的来源。

## 3. `__vm_enough_memory()` 完整判断（mm/util.c:931）

```c
int __vm_enough_memory(struct mm_struct *mm, long pages, int cap_sys_admin)
{
    long allowed;
    vm_acct_memory(pages);                       // ① 先记账

    if (sysctl_overcommit_memory == OVERCOMMIT_ALWAYS)   // ② ALWAYS 永不拒绝
        return 0;
    if (sysctl_overcommit_memory == OVERCOMMIT_GUESS) {  // ③ GUESS 只看单次
        if (pages > totalram_pages() + total_swap_pages)
            goto error;
        return 0;
    }
    allowed = vm_commit_limit();                 // ④ NEVER 严格记账
    if (!cap_sys_admin)
        allowed -= sysctl_admin_reserve_kbytes >> (PAGE_SHIFT - 10); // 给 root 留 8MB
    if (mm) {
        long reserve = sysctl_user_reserve_kbytes >> (PAGE_SHIFT - 10);
        allowed -= min_t(long, mm->total_vm / 32, reserve);  // 防单进程撑爆（≤128MB）
    }
    if (percpu_counter_read_positive(&vm_committed_as) < allowed)   // ⑤ 已承诺 < 上限
        return 0;
error:
    pr_warn_ratelimited(... "not enough memory for the allocation\n");
    vm_unacct_memory(pages);                     // 回滚记账
    return -ENOMEM;
}
```

要点：

- **记账是 percpu 计数器** `vm_committed_as`（:886，`____cacheline_aligned_in_smp`），读时用 `percpu_counter_read_positive` 拿**近似值**（读不精确，但足够做「是否超额」判断）。
- **两道预留**：`admin_reserve_kbytes`（8MB）保证管理员还能登录救援；`user_reserve_kbytes`（128MB，或单进程 `total_vm/32` 的较小值）防止**单个进程**把系统吃到用户无法回收。
- **`vm_acct_memory` / `vm_unacct_memory`** 成对出现：失败要**回滚**，否则计数器会永久虚高。

## 4. 真正的 OOM 触发链（page_alloc.c）

`__vm_enough_memory` 只管「承诺」，而 **OOM killer 由页分配器触发**：

```
用户态 malloc/brk/mmap
        │
        ▼
__alloc_pages / __alloc_pages_slowpath
        │  依次尝试：fast path → 回收(direct reclaim) → compact → ...
        ▼
__alloc_pages_may_oom(gfp_mask, order, ac, &did_some_progress)   // page_alloc.c:3273
        │  关键：只在高阶分配(order>0)失败、或 __GFP_NOFAIL 等场景才进
        ▼
out_of_memory(&oc)                                               // page_alloc.c:3342 调用
```

```c
// page_alloc.c:3273 附近的注释（原文）
/*
 * We are in an unfortunate situation where out_of_memory cannot
 * be called ... Once filesystems are ready to handle allocation
 * failures ... we will call out_of_memory().
 */
if (out_of_memory(&oc) || ...)   // :3342
```

**为什么不是每次分配失败都杀**：`__alloc_pages_may_oom` 会先判断 `gfp_mask` 是否允许直接回收（`__GFP_DIRECT_RECLAIM`）、是否 `__GFP_NORETRY`，**只有真正走到无路可退才进 `out_of_memory`**。这是对原书「主动检查」叙事的现代修正：**检查（overcommit）与止损（OOM）是两条独立的链，别混为一谈。**

## 5. HFT / 嵌入式关联

| 手段 | 作用 | 边界 |
|------|------|------|
| `overcommit_memory=2` + 精确 `overcommit_ratio` | 把「承诺上限」卡死在物理+swap 内，**消除 overcommit 超额** | 只防「承诺过度」，**防不住真内存耗尽**（OOM killer 仍可能触发） |
| `mlock` 关键映射 | 把热路径页**锁死**在物理内存 | 只锁 RSS，不改变 overcommit 记账 |
| 理解 `vm_committed_as` 是近似值 | 别拿它当精确水位线 | percpu counter 读的是 `read_positive` 快照 |

**结论**：对低延迟交易进程，`overcommit_memory=2` 的意义是**让失败在 `malloc` 那一刻显式暴露**（返回 NULL 可处理），而不是在几十毫秒后的缺页/分配路径上被 OOM killer 突然打断——**可预测性 > 可用性**。

---

## 衔接

§1 讲清了「主动检查」与「OOM 止损」的分野。下一节进 OOM killer 本体：`out_of_memory()` 如何判断「现在是不是真的该杀人了」。

---

<details>
<summary>自测 5 问（点开看答案）</summary>

**Q1：`vm.overcommit_memory` 默认是几？默认策略下单次 `malloc` 多大一定会被 `__vm_enough_memory` 拒绝？**

默认是 `0`（`OVERCOMMIT_GUESS`，mm/util.c:803）。GUESS 策略只在 `pages > totalram_pages() + total_swap_pages` 时拒绝（:943）——即**单次请求超过物理+swap 总量**才失败，否则一律放行，所以「默认是半开的」。

**Q2：`vm_commit_limit()` 在没有 `overcommit_kbytes` 时怎么算？**

`(totalram_pages - hugetlb_total_pages) × overcommit_ratio/100 + total_swap_pages`（mm/util.c:875-883）。默认 ratio=50，即物理内存的一半 + swap。

**Q3：`__vm_enough_memory` 里 `admin_reserve_kbytes` 和 `user_reserve_kbytes` 各起什么作用？**

前者（8MB）保证**管理员**在系统快爆时仍能登录救援（非 root 进程的 allowed 要扣掉它）；后者（128MB，或单进程 `total_vm/32` 的较小值）防止**单个进程**通过反复 mmap 把系统吃到连自己都回收不了。

**Q4：`__vm_enough_memory` 失败返回 `-ENOMEM` 会不会触发 OOM killer？**

不会。它只是让 `brk`/`mmap`/`mremap` 等系统调用**提前失败返回**。真正的 OOM killer 入口是 `__alloc_pages_may_oom`（page_alloc.c:3273）→ `out_of_memory`（:3342），是**页分配器**在回收无果后调用的，两条链独立。

**Q5：为什么说 `vm_committed_as` 只能当近似值看？**

它是 `percpu_counter`（mm/util.c:886，`____cacheline_aligned_in_smp`），各 CPU 本地累加、读时 `percpu_counter_read_positive` 拿快照，有跨 CPU 的偏差。做「是否超额」这种粗粒度判断足够，当精确水位线不可靠。

</details>
