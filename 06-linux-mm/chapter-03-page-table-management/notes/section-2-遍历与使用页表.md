# Ch 3 §2 遍历与使用页表 (Using Page Table Entries)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**
> 源码核验：Linux **v6.6**（`include/linux/pgtable.h`、`mm/memory.c`）

---

## 本节讲什么

内核在哪些路径要 **自己走页表**、用什么宏逐级下钻、以及读写 PTE 状态的 API 面。这是读 MM 源码的"识字课"——后面所有章节（缺页、COW、swap、munmap、mprotect）全是这些宏的组合应用。

---

## 1. 谁在遍历页表

| 路径 | 入口 | 干什么 |
|------|------|--------|
| 缺页 | `handle_mm_fault()`（memory.c） | walk 到空位 → 分配页/读文件 → `set_pte` |
| 解除映射 | `unmap_page_range()` → `zap_pte_range()`（memory.c:1629/:1394） | 批量 clear PTE + 释放页表页 |
| 保护位变更 | `mprotect` → `change_pte_range()` | 批量改 RW 位 → **必须 flush TLB** |
| 反向查询 | `follow_page()`/GUP（`get_user_pages`） | VA → `struct page`（DMA、io_uring、vmsplice 全靠它） |
| swap/回收 | rmap（`page_referenced`、`try_to_unmap`） | 从 page 反向清 PTE 的 accessed/present |
| 调试/观测 | `/proc/pid/pagemap`、`clear_refs` | 用户态可见的 PTE 状态镜像 |

## 2. 逐级下钻宏（v6.6 pgtable.h 实锚）

```c
pgd_t *pgd = pgd_offset(mm, addr);          /* :145  mm->pgd + pgd_index(addr) */
p4d_t *p4d = p4d_offset(pgd, addr);         /* 四级配置下折叠：返回 pgd 本身 */
pud_t *pud = pud_offset(p4d, addr);         /* :129 */
pmd_t *pmd = pmd_offset(pud, addr);         /* :121 */
pte_t *pte = pte_offset_map(pmd, addr);     /* 注意：v6.6 是 _map/_unmap 配对！ */
...
pte_unmap(pte);
```

**原书（2.4）写的是 `pte_offset()`；v6.6 里它已换成 `pte_offset_map()`/`pte_offset_unmap()` 配对。** 为什么：x86-32 HIGHMEM 时代 PTE 页可能临时 kmap；64 位上没有 HIGHMEM，但 API 形态保留——并且 v6.6 引入了 `pte_offset_map_nolock()`/rcu 读取语义，防并发替换。**读旧书遇到 `pte_offset` 要脑内换新名。**

每一级下钻前都要判空/判坏：

```c
if (pgd_none(*pgd) || pgd_bad(*pgd)) return -1;   /* 没建过下层表 */
if (pmd_trans_huge(*pmd)) { /* 大页！PMD 即终态，别再下钻 */ }
```

`pmd_trans_huge()` 检查是 **必须的**——THP 开启时 walk 到 PMD 就直接是大页映射，`pmd_offset` 之后再 `pte_offset_map` 会把 PMD 项错当 PTE 表指针解引用。

## 3. PTE 读写 API 面

| 类别 | 宏 | 说明 |
|------|----|------|
| 判型 | `pte_none / pte_present / pte_write / pte_dirty / pte_young` | 查询位 |
| 造/改 | `pte_mkwrite / pte_wrprotect / pte_mkdirty / pte_mkyoung / pte_mknovnode` | 返回 **新 pte_t**（值语义，不是原地改） |
| 安装 | `set_pte_at(mm, addr, ptep, pte)` | 原子写表项+arch 钩子（如 ARM 需要 DSB 屏障） |
| 清除 | `pte_clear / ptep_get_and_clear` | 后者原子取旧值——COW/换出要先取走 |
| PFN | `pte_pfn(pte)` / `pte_page(pte)` | 项 → PFN → `struct page` |
| 特判 | `pte_special / pte_devmap / pte_marker` | 非常规映射（见下） |

**值语义陷阱：** `pte_mkwrite(pte)` 返回修改后的副本，**必须再 `set_pte_at` 写回** 才生效。漏写回 = 白改。

## 4. v6.6 新增：PTE marker（`pte_marker`）

PTE 非 present 时除了编码 swap entry，v6.6 还能编码 **PTE marker**（`CONFIG_PTE_MARKER_UFFD_WP` 等）：用户态 fault injection（uffd）、uffd-wp 写保护在 **换出后** 仍能记住"这里被 userfaultfd 监视"。读旧书时这片区域是空白——"非 present PTE = swap entry"在 v6.6 已不成立。

## 5. GUP：内核拿用户页的标准通道

```c
get_user_pages_fast(start, nr_pages, gup_flags, pages);  /* 无锁快路径 */
get_user_pages(start, nr_pages, gup_flags, pages, vmas); /* 慢路径，可睡眠 */
```

`_fast` 版本先 **无锁走页表**（仅本地中断禁止），失败（缺页/迁移中）退回慢路径拿 mmap_lock + 走 fault。**MSG_ZEROCOPY 的 page pin、io_uring 的 fixed buffer、AF_XDP 的 umem 注册全走 GUP**——它是"内核引用用户内存"的唯一正门，`vm_normal_page()`（memory.c:581）在底层区分"普通页"与 VM_IO/VM_PFNMAP 特殊映射（`_PAGE_SPECIAL`）。

**HFT 关键：** GUP pin 住的页不能被回收/迁移——这就是 12.5/ch13 里"注册过的 buffer 物理页被钉死"的机制源头。pin 的页在 cgroup OOM / 内存紧张时是 **不可移动内存**，注册区过大（几十 GB）会直接推高内存碎片。

## 6. HFT / 嵌入式关联

| 现象 | 机制兑现 |
|------|----------|
| prefault（启动时摸一遍内存） | 批量触发 `do_anonymous_page`（memory.c:4067）建 PTE，消灭运行期缺页 |
| `mlock` + `mlockall` | PTE 永久 present，swap 路径绕开 |
| 大页合并/分裂 | PMD 级改写 + 全核 TLB shootdown → **尾延迟尖刺源**（见 THP 笔记） |
| `process_vm_readv`/ptrace | 内核 walker 替你走对方页表 |
| 侧信道缓解 `madvise(MADV_DONTNEED)` | PTE 清空 + TLB flush 立即收回 |

## 7. 衔接

- 上节 [§1 页目录与页表项](./section-1-页目录与页表项.md)
- 下节 [§3 页表的分配与释放](./section-3-页表的分配与释放.md)
- GUP 实战：[12.5/ch13 零拷贝](../../../12.5-modern-networking/chapter-13-zerocopy-highperf/)（page pin 一节）

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：`pte_offset_map` 之后为什么必须 `pte_unmap`？64 位上不是直接可解引用吗？**
A：API 契约层面：x86-32 HIGHMEM 的 kmap 配对习惯被保留为通用接口，防止 arch 特定问题。v6.6 里它还有 **rcu/mmap_lock 读取语义**——表页可能被并发 `tlb_remove_table` 延迟释放，map/unmap 圈出的区间受 RCU 保护。64 位上开销≈0，但配对是强制的。

**Q2：walk 到 PMD 发现 `pmd_trans_huge()`，想改这一页的权限怎么办？**
A：不能下钻——PMD 项本身就是"大 PTE"。要么用 `pmd_*` 系列（`pmd_mkwrite` 等）直接改 PMD，要么先 `split_huge_pmd()` 拆成 4KiB PTE 再改。错误下钻 = 把 PMD 值当 PTE 表指针 → panic。

**Q3：`set_pte` 与 `set_pte_at` 差在哪？**
A：`set_pte_at(mm, addr, ptep, pte)` 带 mm/addr 上下文，arch 可插入屏障/缓存维护（ARM64 必须 DSB 保证页表写对 MMU 可见）；裸 `set_pte` 只写内存。内核里几乎全用 `_at` 版本。

**Q4：为什么 GUP 要 `_fast`/慢路径两套？**
A：快路径假设 PTE 已 present 且非特殊映射，无锁读表+pte 提 pin（`try_grab_folio`）。缺页/`pte_special`/迁移条目则退慢路径：拿 `mmap_lock`（读）、可能触发缺页或等待迁移——可睡眠。HFT 场景务必启动时把页 pin 好，避免热路径掉进慢路径。

**Q5：遍历用户页表需要持什么锁？**
A：稳定遍历需 `mmap_lock`（读）——防 VMA 树被并发改。v6.6 的 VMA 已是 maple tree（06.5/ch05），锁语义不变。仅读单个 PTE 用 rcu 版 `pte_offset_map_nolock` 可免 mmap_lock，但要自校验。**结论：MM 内核代码的锁纪律 = 先锁再钻。**

</details>

---
