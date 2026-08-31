# Ch 4 §3 内存区域（VMA · v6.6 视角）

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**
> 源码核验：Linux **v6.6**（`include/linux/mm_types.h` :634 `vm_ops`、`mm/mmap.c` VMA 查找/合并路径）

---

## 本节讲什么

VMA（`vm_area_struct`）是地址空间的"地段划分"：一段连续 VA + 统一属性 + 统一行为。本节讲清 VMA 的字段语义、**合并规则**（直接影响 maps 里的段数）、`vm_ops` 回调体系、以及 HFT 的 VMA 管理纪律。

---

## 1. VMA 字段速览（v6.6）

| 字段 | 语义 | 备注 |
|------|------|------|
| `vm_start/vm_end` | `[start, end)` 半开区间 | **永不重叠**是树的不变量 |
| `vm_flags` | READ/WRITE/EXEC/SHARED/MAYSHARE/`VM_DONTCOPY`… | **页表权限的"申请单"**，缺页按它造 PTE |
| `vm_pgoff`/`vm_file` | 文件映射的偏移与文件 | 匿名映射为 NULL |
| `vm_ops`（mm_types.h:634） | 文件页的行为回调 | fault/close/split/advise |
| `anon_vma` 链 | rmap 的锚（Ch3 §7） | fork/COW 的反向映射根 |
| `vm_policy` | NUMA 策略（mbind 存这） | per-VMA 生效的原因 |

**`vm_flags` 是批量的申请单：** `mprotect` 改的是 VMA 的 flags，然后走 Ch3 的改 PTE+flush 流程。**权限检查（缺页/信号判定）第一层永远先查 VMA flags**（快，无锁读），第二层才是 PTE。

## 2. 合并规则（决定 maps 里段数的隐形逻辑）

新 VMA 加入（mmap_region）时尝试与邻居合并（`vma_merge`）：

| 条件（与前后邻居合并需同时满足） | 说明 |
|----------------------------------|------|
| 地址相邻（prev.end == new.start） | 无缝 |
| flags 相同 | 权限/共享性一致 |
| 同一文件 + 连续 pgoff | 文件视图连续 |
| 同一 anon_vma / 同 policy | rmap 与 NUMA 一致 |

**工程后果：** 分 100 次 `mmap` 相同属性的相邻匿名页 → maps 里 **一段**。反之 `mprotect` 中间一页 → 原段 **分裂成三段**（VMA 数量+2）——**mprotect 是 VMA 数量爆破的元凶**，而 VMA 数量直接影响 maple 树规模与 fork 的复制成本。

`max_map_count`（默认 65530）是 VMA 数量上限——JVM/多线程进程撞过这个线的案例很多。

## 3. `vm_ops`：文件映射的行为面

```c
struct vm_operations_struct {
    vm_fault_t (*fault)(...);      /* 缺页：从文件读页 */
    void (*close)(...);            /* munmap 收尾 */
    void (*map_pages)(...);        /* 批量预fault（readaround） */
    int (*split)(...);             /* VMA 被切分时同步 */
    ...
};
```

| 映射类型 | vm_ops | 缺页行为 |
|----------|--------|----------|
| 匿名 | NULL | `do_anonymous_page`（零页或新页） |
| 文件（ext4/xfs/tmpfs） | `generic_file_vm_ops` 等 | `filemap_fault` → page cache → 磁盘 |
| 设备（DRM/VFIO） | 驱动自定义 | 驱动 fault（常 remap_pfn_range） |
| hugetlb | `hugetlb_vm_ops` | 大页专用路径（不走普通 PTE） |

**DPDK/AF_XDP 的用户映射就是设备 vm_ops**——驱动 fault 时把 DMA 映射进用户 VMA（`VM_IO/VM_PFNMAP` 标志，无 struct page，Ch3 §5）。

## 4. 相关系统调用 × v6.6 行为

| 调用 | v6.6 要点 | HFT 姿势 |
|------|-----------|----------|
| `mmap` | MAP_FIXED_NOREPLACE（不覆盖探测）；MAP_POPULATE（预 fault）；MAP_HUGETLB/UNIFIED | 启动期定格布局三件套 |
| `mremap` | MREMAP_DONTUNMAP（v5.13+，搬走但留旧映射） | 快照切换的页级操作 |
| `mlock`/`mlockall` | 落 RSS+防换出；**不 prefault**（只 fault 过的页才真锁住） | `MAP_POPULATE`+mlock 组合拳 |
| `madvise` | DONTNEED（立刻弃页）/HUGEPAGE/WILLNEED… | 精细分区管理的入口 |
| `mbind` | per-VMA 策略 | NUMA 钉页 |

**mlock 陷阱（高频误用）：** mlock **只锁已驻留的页**，"mlock 了但没 touch"的页在首次访问时仍走缺页。正确顺序：mmap → **touch 全区**（或 MAP_POPULATE）→ mlock。

## 5. 观测与排障

```bash
cat /proc/<pid>/maps                     # VMA 清单（起止/权限/来源）
wc -l /proc/<pid>/maps                   # VMA 数量健康度（别逼近 max_map_count）
cat /proc/sys/vm/max_map_count
cat /proc/<pid>/smaps                    # 每段 RSS/PSS/THP/locked 明细
```

| 症状 | 定位 |
|------|------|
| maps 行数持续增长 | 动态 mmap 未复用 / mprotect 爆破段数 → 查泄漏 |
| smaps 里 Locked < 预算 | mlock 陷阱（未 touch 就锁） |
| 巨型 anon 段但 RSS 小 | mmap 了没 prefault——启动期裸奔 |

## 6. HFT / 嵌入式关联

| 纪律 | 理由 |
|------|------|
| 布局一次性定型 | 消灭运行期写锁事件（§2）+ VMA 数稳定 |
| 段数预算 | 引擎 maps 行数应 <100 且恒定；告警线：日环比增长 |
| vm_flags 预检 | 共享行情只读映射 → `PROT_READ`+`MAP_SHARED`，防误写触发 COW fault 风暴 |
| mremap 替代 munmap+mmap | 不产生 VMA 空洞，树不碎 |

## 7. 衔接

- [§4 缺页](./section-4-异常处理与缺页异常.md)：VMA flags 兑现成 PTE 的时刻
- [Ch 3 §7 rmap](../../chapter-03-page-table-management/notes/section-7-2.6-内核的新变化.md)：anon_vma 的去向
- [Ch 12 shmem](../../chapter-12-shared-memory-virtual-filesystem/)：MAP_SHARED 的文件后端
- [06.5/ch05](../../../06.5-modern-mm/chapter-05-vm-address-space-maple-tree/)：VMA 树结构

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：为什么 VMA 必须按属性分割，而不是每页记权限？**
A：粒度经济学：页级权限记在 PTE（已有），VMA 是 **批量管理层**——缺页判定、NUMA 策略、rmap 锚、锁边界都以 VMA 为单位。若每页独立管理，fork/遍历/策略的元数据和管理成本爆炸。**分层：VMA 管策略，PTE 管执行。**

**Q2：`mprotect` 一页为什么会产生 3 个 VMA？**
A：原段 [A,B) 被改 [x,x+1)：前段 [A,x)、改段 [x,x+1)、后段 [x+1,B)。前后段与新 flags 不同不能合并。反复对不同页 mprotect = VMA 数量线性涨——**权限变更应按区域规划，不要逐页打补丁**。

**Q3：匿名 VMA 的 vm_ops 为 NULL，缺页怎么分发？**
A：`handle_mm_fault` 判 `vma->vm_ops == NULL || !vm_ops->fault` → 走 `do_anonymous_page`（memory.c:4067）。文件/设备 VMA 才调 `vm_ops->fault`。所以 vm_ops 是否为空是"匿名与否"的分路开关，不靠 flags 推断。

**Q4：MAP_SHARED 匿名映射没有文件，怎么实现共享？**
A：tmpfs 兜底：内核为 SHARED 匿名 mmap 建 shmem 对象（Ch 12），vm_file 指向它——**"匿名共享"其实有文件（tmpfs）**，pages 挂在该 inode 的 page cache 上，多进程经同一 rmap 树看到同一批页。`/dev/zero` mmap 老写法同机制。

**Q5：VMA 的 split 回调为什么存在？谁能承受 VMA 分裂？**
A：mprotect/分裂 VMA 时，回调让持有者同步（如 hugetlb 的预留计数、userfaultfd 的注册区间）。不承受的例子：v6.6 起部分驱动声明 `VM_DONTSPLIT`。HFT 视角：你的固定映射区域永远不该被 split——布局定型后没有任何 mprotect 会切它。

</details>

---
