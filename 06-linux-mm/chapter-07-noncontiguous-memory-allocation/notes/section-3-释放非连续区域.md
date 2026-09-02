# Ch 7 §3 释放非连续区域 (Freeing)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**
> 源码核验：Linux **v6.6**（`mm/vmalloc.c`）

---

## 本节讲什么

本节回答：**`vfree()` 怎么把 §2 那一整套「虚拟区间 + 物理散页 + 页表映射」对称拆掉？**

原书（2.6）的释放是「扫链表定位 → 反向 walk 页表 → 逐页 `free_pages`」。v6.6 里骨架不变，但有两个关键升级：① 定位从 O(n) 链表扫描变成 O(log n) 红黑树查找；② TLB 刷新从「每次释放都 flush」变成「**懒释放**」——先把区间扔进 purge 树攒着，攒够阈值再一次性 flush。

---

## 1. `vfree` 完整流程（`mm/vmalloc.c:2807`）

```c
void vfree(const void *addr)
{
    struct vm_struct *vm;
    int i;

    if (unlikely(in_interrupt())) {        /* :2812 中断上下文？ */
        vfree_atomic(addr);                /* :2813 走原子延迟释放 */
        return;
    }
    BUG_ON(in_nmi());                      /* :2817 NMI 里禁止 */
    ...
    vm = remove_vm_area(addr);             /* :2824 ① 摘除虚拟区间 */
    if (unlikely(!vm)) { WARN(...); return; }

    if (unlikely(vm->flags & VM_FLUSH_RESET_PERMS))
        vm_reset_perms(vm);                /* :2832 特殊：重置 direct map 权限 */

    for (i = 0; i < vm->nr_pages; i++) {
        struct page *page = vm->pages[i];
        mod_memcg_page_state(page, MEMCG_VMALLOC, -1);
        __free_page(page);                 /* :2842 ② 逐页回 Buddy */
        cond_resched();                    /* :2843 页多时让出 CPU */
    }
    atomic_long_sub(vm->nr_pages, &nr_vmalloc_pages);  /* :2845 全局统计 */
    kvfree(vm->pages);                     /* :2846 ③ 释放 pages 数组本身 */
    kfree(vm);                             /* :2847 ④ 释放 vm_struct 描述符 */
}
```

四个步骤，与 §2 严格对称：

| 步骤 | §2 分配 | §3 释放 |
|------|---------|---------|
| ① 虚拟区间 | `__get_vm_area_node` reserve | `remove_vm_area` 摘除 |
| ② 物理页 | `vm_area_alloc_pages` 批量分配 | `__free_page` 逐页归还 |
| ③ pages 数组 | `kmalloc`/`__vmalloc_node` 分配 | `kvfree` 释放 |
| ④ 描述符 | `kzalloc_node` 分配 `vm_struct` | `kfree` 释放 |

> **`cond_resched()` 的意义**（`:2843`）：一次 `vfree` 一个 GB 级区域要释放 26 万页，若在循环里不主动让出 CPU，会长时间占着不调度——影响其他进程延迟。`cond_resched()` 在 `need_resched` 时主动调度。

---

## 2. ① 摘除虚拟区间：`remove_vm_area`（`:2684`）

```c
struct vm_struct *remove_vm_area(const void *addr)
{
    struct vmap_area *va;
    struct vm_struct *vm;

    if (WARN(!PAGE_ALIGNED(addr), "Trying to vfree() bad address (%p)\n", addr))
        return NULL;                       /* :2691 地址必须页对齐 */

    va = find_unlink_vmap_area((unsigned long)addr);  /* :2695 红黑树查找 + 摘除 */
    if (!va || !va->vm)
        return NULL;                       /* :2696 地址不在 vmalloc 区 */
    vm = va->vm;                           /* :2698 union 取出 vm_struct */
    ...
    free_unmap_vmap_area(va);              /* :2705 unmap + 懒释放 */
    return vm;
}
```

`find_unlink_vmap_area`（`:1868`）在 **busy 红黑树**里 O(log n) 找到覆盖 `addr` 的 `vmap_area` 并 `unlink_va` 摘下来——这就是 §1 说「红黑树替代链表」的直接收益：**`vfree` 定位不再扫全表**。

---

## 3. 懒 TLB 刷新：三棵树模型

`free_unmap_vmap_area`（`:1847`）里藏着一个重要设计——**不是立即 flush TLB**：

```c
static void free_unmap_vmap_area(struct vmap_area *va)
{
    flush_cache_vunmap(va->va_start, va->va_end);      /* :1849 刷 cache */
    vunmap_range_noflush(va->va_start, va->va_end);    /* :1850 清页表，但不 flush TLB */
    if (debug_pagealloc_enabled_static())
        flush_tlb_kernel_range(va->va_start, va->va_end);

    free_vmap_area_noflush(va);                        /* :1854 丢进 purge 树 */
}
```

`free_vmap_area_noflush`（`:1817`）：

```c
nr_lazy = atomic_long_add_return((va->va_end - va->va_start) >> PAGE_SHIFT,
                                 &vmap_lazy_nr);       /* :1826 累加「懒释放页数」 */

merge_or_add_vmap_area(va, &purge_vmap_area_root, &purge_vmap_area_list); /* :1833 */

if (unlikely(nr_lazy > nr_lazy_max))                   /* :1840 攒够了？ */
    schedule_work(&drain_vmap_work);                   /* :1841 触发真正的 purge */
```

**所以 v6.6 里 `vmap_area` 其实横跨三棵树**：

```
  free 树           busy 树            purge 树（懒释放等待区）
  ┌──────────┐    ┌──────────┐        ┌──────────────┐
  │ 可分配洞  │    │ 已映射    │        │ 已 unmap 但    │
  │ subtree_ │    │ vm_struct│        │ TLB 还没 flush │
  │ max_size │    │ 指针     │        │ 攒着等批量处理  │
  └──────────┘    └──────────┘        └──────────────┘
     ↑ 分配            ↑ 查找/释放        ↑ 懒 flush 中转
   alloc_vmap_area   find_unlink     free_vmap_area_noflush
                     _vmap_area          ↓ 超阈值
                                    __purge_vmap_area_lazy
                                    (flush_tlb_kernel_range)
```

**为什么懒 flush？** 全局 TLB flush（`flush_tlb_kernel_range`）在**多核大机器上极贵**——要发 IPI 让所有 CPU 都刷 TLB。若每次 `vfree` 都 flush，批量释放 N 个区域就要 N 次全局 flush。**懒释放**把「unmap 页表」和「flush TLB」解耦：页表立即清（安全性），TLB 攒到 `lazy_max_pages()` 阈值再统一 flush（性能）。

`lazy_max_pages`（`:1700`）：

```c
log = fls(num_online_cpus());                          /* 在线 CPU 数取对数 */
return log * (32UL * 1024 * 1024 / PAGE_SIZE);         /* = log × 8192 页 */
```

CPU 越多，阈值越高——因为 CPU 越多，一次全局 flush 越贵，越值得攒多点再 flush（对数增长是「保守」选择，注释 :1693-1698 明确说不想在大系统上引入大延迟）。

---

## 4. 中断上下文：`vfree_atomic`（`:2773`）

`vfree` 开头就判断 `in_interrupt()`，中断上下文不能 sleep/做重活，于是转 `vfree_atomic`：

```c
void vfree_atomic(const void *addr)
{
    struct vfree_deferred *p = raw_cpu_ptr(&vfree_deferred);

    BUG_ON(in_nmi());                                  /* :2777 NMI 仍禁止 */
    if (addr && llist_add((struct llist_node *)addr, &p->list))  /* :2786 lockless 入队 */
        schedule_work(&p->wq);                         /* :2787 丢给 workqueue */
}
```

**思路**：原子上下文里只做**无锁入队**（`llist_add`，lockless 链表），真正的释放推迟到 workqueue 上下文（能 sleep）再执行。每个 CPU 一个 `vfree_deferred`，避免锁竞争。

---

## 5. `vfree` vs `vunmap`：谁持有物理页？

| | `vfree(addr)` | `vunmap(addr)` |
|---|---------------|----------------|
| 对应分配 | `vmalloc()` 家族 | `vmap()` |
| 物理页来源 | vmalloc 自己分配的散页 | **调用者提供的页** |
| 释放物理页 | ✅ 逐个 `__free_page` | ❌ 只 unmap，页仍归调用者 |
| 释放 pages 数组 | ✅（若 `VM_MAP_PUT_PAGES` 则连数组一起） | ❌ |

`VM_MAP_PUT_PAGES`（`vmalloc.h:29`）是 `vmap()` 的选项：置位后 pages 数组所有权转移给 vmalloc 层，此时用 **`vfree`**（而非 `vunmap`）释放，会连带释放数组和每页引用。不置位则用 `vunmap`，页和数组都还是调用者的。

---

## 6. HFT / 嵌入式关联

| 场景 | 关联 |
|------|------|
| **懒 flush 的权衡** | 「攒够再批处理」是通用优化：HFT 里日志落盘、订单批处理都是同构思想——**用延迟换吞吐，但要设阈值保证上限**（`lazy_max_pages` 就是那个上限） |
| **`cond_resched` 的启发** | 大块释放主动让出 CPU，避免长时间占用调度——低延迟系统里「长任务切分 + 主动让权」是基本素养 |
| **原子 vs 可睡眠** | `vfree_atomic` 用「无锁入队 + workqueue 兜底」把重活移出原子上下文——HFT 内核模块在 NAPI/软中断里分配释放也要套这个模式 |
| **三棵树的中转** | purge 树是「已 unmap 未 flush」的中间态——**状态机分离**让「安全清页表」和「昂贵 flush」解耦，是资源回收的经典设计 |

---

## 7. 衔接

- 上节 [§2 分配非连续区域](./section-2-分配非连续区域.md)：分配的四步，本节逐条反向拆
- 下节 [§4 2.6 内核的新变化](./section-4-2.6-内核的新变化.md)：红黑树/懒 flush/huge vmalloc 的演进全景
- 页释放器：[Ch6 §3 页面释放](../../chapter-06-physical-page-allocation/notes/section-3-页面释放.md)（`__free_page` 的落点）

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：`vfree` 的四个步骤分别对应 §2 分配的哪四个步骤？**
A：严格对称——① `remove_vm_area` 摘虚拟区间（↔ `__get_vm_area_node` reserve）；② `__free_page` 逐页回 Buddy（↔ `vm_area_alloc_pages` 批量分配）；③ `kvfree` 释放 pages 数组（↔ `kmalloc`/`__vmalloc_node` 分配数组）；④ `kfree` 释放 `vm_struct` 描述符（↔ `kzalloc_node` 分配）。

**Q2：v6.6 里 `vmap_area` 横跨几棵树？各自作用是什么？**
A：三棵——**free 树**（可分配的洞，`subtree_max_size` 剪枝）；**busy 树**（已映射，`vm_struct` 指针）；**purge 树**（已 unmap 但 TLB 未 flush 的懒释放中转站）。`vfree` 时区间从 busy 树 → purge 树 →（攒够后）→ free 树。

**Q3：为什么 `free_unmap_vmap_area` 不立即 flush TLB？**
A：全局 TLB flush 要多核发 IPI，极贵。所以「清页表」和「flush TLB」解耦：页表立即清（保证不再访问），TLB 攒到 `lazy_max_pages()` 阈值再统一 `flush_tlb_kernel_range`。阈值 = `fls(在线CPU数) × 8192 页`，CPU 越多阈值越高，因为 flush 越贵。

**Q4：`vfree_atomic` 为什么能用无锁链表？它和普通 `vfree` 的分流条件是什么？**
A：分流条件是 `in_interrupt()`——中断上下文不能 sleep、不能做 `__free_page`/`kvfree` 这类可能阻塞的操作。`vfree_atomic` 用 `llist_add`（lockless 链表实现）把地址入队，再 `schedule_work` 丢给 workqueue，在**可睡眠上下文**里完成真正释放。NMI 里仍然禁止（`BUG_ON(in_nmi)`）。

**Q5：`vfree` 和 `vunmap` 都作用于 vmalloc 区地址，怎么选？**
A：看**物理页谁持有**。`vmalloc` 分配的页归 vmalloc 层，用 `vfree` 释放（连页带描述符）；`vmap` 映射的页是调用者自己的，用 `vunmap` 只拆映射，页仍归调用者。若 `vmap` 时置了 `VM_MAP_PUT_PAGES`，所有权已转移，就得用 `vfree` 连数组一起释放。

</details>
