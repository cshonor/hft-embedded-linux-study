# Ch 10 §5 换出进程页面 (Swapping Out)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**
> 源码核验：Linux **v6.6**（`include/linux/rmap.h` 的 `try_to_unmap`、`mm/vmscan.c` 的 `shrink_folio_list`）

---

## 本节讲什么

匿名页没有后备存储，回收它必须先「**写出去**」。本节回答：

1. 文件页和匿名页的回收路径差在哪？
2. rmap 反向映射怎么解决「从页找到所有 PTE」？
3. 换出之后页去哪了、怎么还能换回来？

---

## 1. 文件页 vs 匿名页：回收路径分叉

| 页类型 | 回收方式 | 能否再读回 |
|--------|----------|-----------|
| 文件页（干净） | 直接 free（数据还在文件里） | 缺页时从文件重读 |
| 文件页（脏） | writeback 写回文件再 free | 同上 |
| 匿名页（堆栈堆、私有 mmap） | 必须**写 swap 设备**才能腾物理页 | 缺页时从 swap 读回 |

匿名页的麻烦在于：它**只存在于内存里**，没有文件可回退。要回收它，就得先把它写进 swap 区（Ch 11 详述），换出后 PTE 里留下一个 **swap entry** 指向「它躺在 swap 区的哪个槽位」。

---

## 2. 反向映射 rmap：从页找到所有 PTE

回收一个匿名页，第一步是**解除所有进程对它的映射**（unmap）。问题是：一个物理页可能被多个进程映射（fork 后共享、多进程共享内存），怎么从 `struct page` 反查所有 PTE？

**2.4 的痛：** 没有 rmap，只能**线性扫描所有进程的页表**找谁映射了这个页——`swap_out()` 极慢。

**2.6+ rmap：** 在 `struct page/folio` 上维护**反向映射**，能直接定位所有 PTE。v6.6 的接口（`rmap.h:372`）：

```c
void try_to_unmap(struct folio *folio, enum ttu_flags flags);
```

`try_to_unmap()` 遍历 folio 的所有映射（通过 `i_mmap` 红黑树找 VMA，再定位 PTE），逐个清除 PTE 的 present 位、替换成 swap entry。**解映射后，物理页的引用才归零，才能真正 free**。

---

## 3. swap out 完整流程

```
匿名页回收（shrink_folio_list 里）
    ├─ try_to_unmap(folio)      # rmap 解所有 PTE，PTE 变 swap entry
    ├─ folio 加入 swap cache    # 防止「换出过程中又被 fault 回来」造成双写
    ├─ 写 swap 设备（swap_writepage）
    ├─ 物理页归还 Buddy
    └─ PTE 里留着 swap entry → 下次访问触发缺页 → 从 swap 读回
```

**swap cache 的作用：** 换出还没写完时，如果进程又访问这个页（fault），内核会先查 swap cache——命中说明「正在换出」，就等它写完再从 swap 读回，避免「一边写、一边又建新映射」导致数据撕裂。

---

## 4. 工作集 refault 检测

回收不是「踢掉就完了」。v6.6 的 `lruvec->refaults[]`（§1）持续统计：**被回收的页，多久之后又被 fault 回来？**

```
回收页 → 一段时间后又被访问（refault）
    ├─ refault 频繁 → 说明回收误踢了工作集 → 调大 active，保护工作集
    └─ refault 稀少 → 回收正确，继续
```

这是「回收器自我纠错」的反馈回路——通过观察 refault 频率，动态调整回收激进程度。原书（2.6）没有这个机制。

---

## 5. HFT / 嵌入式关联

| 现象 | 本节机制的兑现 |
|------|----------------|
| swap 是最大延迟源 | 匿名页换出换入要碰磁盘，比 page cache 回收慢一个量级 |
| `mlock` 防换出 | 页进 unevictable，`try_to_unmap` 根本不会被调用 |
| 频繁 refault | 回收误踢工作集 → 二次抖动，监控 `/proc/vmstat` 的 `workingset_refault` |
| `vm.swappiness=0` | 让回收器尽量先回收文件页、别碰匿名页 |

---

## 6. 衔接

- 上节 [§4 收缩各类缓存](./section-4-收缩各类缓存.md)
- [§6 kswapd](./section-6-页面换出守护进程.md)：谁在驱动 swap out
- 详 [Ch 11 交换管理](../../chapter-11-swap-management/)（swap 槽位、swap cache 细节）
- 前置：[Ch 4 §4 缺页](../../chapter-04-process-address-space/notes/section-4-异常处理与缺页异常.md)（swap entry 触发的缺页）

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：为什么匿名页回收前必须先 `try_to_unmap`？**
A：物理页只有在**没有任何 PTE 指向它**、引用计数归零时，才能安全 free。`try_to_unmap` 解除所有映射、把 PTE 换成 swap entry，是把页「从所有进程的地址空间里摘出来」的必要一步。不 unmap 就 free，等于留下悬空 PTE，之后访问就是 use-after-free。

**Q2：swap cache 和 page cache 什么关系？**
A：swap cache 是 page cache 的一种特殊形态——它缓存「正在换出/已换出的匿名页」。作用是保证换出换入的**原子性**：换出写盘期间，若页又被 fault，先查 swap cache，避免同时有两个路径操作同一页造成数据撕裂。Ch 11 会详讲它的 `swp_entry_t` 编码。

**Q3：rmap 是「页 → PTE」的反向映射，正向是谁？**
A：正向是页表（VA → PA），以及 `address_space->i_mmap`（文件 → 所有映射它的 VMA）。rmap 补上了「PA → 所有映射它的 PTE」这一环，让回收器能从物理页出发，反向找到并解除所有虚拟映射。正向查表、反向 unmap，两条路配合。

**Q4：swap out 之后，物理页还在内存里吗？**
A：不在了，已经归还 Buddy。留在内存里的只是 PTE 里的 **swap entry**（一个编码了 swap 区槽位的值）。进程下次访问这个地址，触发缺页，内核从 swap entry 解码出槽位，从 swap 设备读回数据，重新建立映射。

**Q5：refault 检测怎么帮助减少「回收抖动」？**
A：如果回收器误踢了工作集里的热页，这些页很快会被再次访问（refault），造成「回收 → 读回 → 又回收」的抖动。`lruvec->refaults[]` 统计 refault 频率，频率高就**调大 active 链**（保护更多页不降级），让回收器更保守，减少误踢。

</details>

---
