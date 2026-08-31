# 透明大页 THP · HFT 决策专题

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴** · 原书未专章，THP 为 2.6.38+ 机制
> 源码核验：Linux **v6.6**（`mm/huge_memory.c`、`mm/khugepaged.c`）

---

## 本节讲什么

THP（Transparent Huge Page）让内核 **自动** 把连续 4KiB 匿名页合并成 2MiB PMD 大页映射。它是"TLB miss 消减器"，也是"随机延迟发生器"——HFT 机器上 **开还是关** 是必须一次性想清楚并写进部署清单的决策。本节给全机制 + 决策表。

---

## 1. 机制全景（v6.6 实锚）

```c
/* huge_memory.c:50 */
unsigned long transparent_hugepage_flags __read_mostly =
#ifdef CONFIG_TRANSPARENT_HUGEPAGE_ALWAYS
    (1<<TRANSPARENT_HUGEPAGE_FLAG)|          /* always 模式 */
#endif
#ifdef CONFIG_TRANSPARENT_HUGEPAGE_MADVISE
    (1<<TRANSPARENT_HUGEPAGE_REQ_MADV_FLAG)| /* madvise 模式 */
#endif
    (1<<TRANSPARENT_HUGEPAGE_DEFRAG_REQ_MADV_FLAG)|
    (1<<TRANSPARENT_HUGEPAGE_DEFRAG_KHUGEPAGED_FLAG)|
    (1<<TRANSPARENT_HUGEPAGE_USE_ZERO_PAGE_FLAG);
```

| 组件 | 位置 | 作用 |
|------|------|------|
| 缺页路径大页 | `__do_huge_pmd_anonymous_page()`（huge_memory.c:646） | 匿名缺页时直接试分 2MiB 连续物理页 |
| 后台合并 | **khugepaged**（khugepaged.c） | 扫描已按 4K 映射的 VMA，找机会 collapse 成 PMD 大页 |
| 拆分 | `split_huge_pmd()` | mprotect/munmap/迁移时把 PMD 拆回 512 个 PTE |
| 准入检查 | `hugepage_vma_check()`（huge_memory.c:74） | 排除 DAX、特殊 VM flags 等不适用 VMA |

**三种模式（`/sys/kernel/mm/transparent_hugepage/enabled`）：**

```
always   [always] madvise never    # 括号即当前值
```

| 模式 | 分配时机 | 延迟特征 |
|------|----------|----------|
| `always` | 匿名缺页直接 2MiB；失败退 4K；khugepaged 后台继续补 | 缺页变贵（找 512 连续页），后台 collapse 随时 shootdown |
| `madvise` | 仅 `madvise(MADV_HUGEPAGE)` 标过的 VMA 参与 | **进程自主**——不标的 VMA 完全不受干扰 |
| `never` | 全禁 | 只有显式 hugetlbfs 大页可用 |

**khugepaged 两个节流参数（实锚 khugepaged.c:74/76）：**

```c
khugepaged_scan_sleep_millisecs  = 10000;  /* 每轮扫描后睡 10s */
khugepaged_alloc_sleep_millisecs = 60000;  /* 分配失败（内存碎片化）睡 60s */
```

## 2. collapse 的真实代价（HFT 为什么怕它）

khugepaged 把 511 个 **已在用** 的 4K 页改写成 1 个 PMD 项，动作链：

```
1. 拿 mmap_lock（读）+ pmd 锁
2. 验证 511 页物理连续且全 anonymous、无共享、无 pin
3. 一次性替换 PMD → 旧 PTE 表页作废
4. flush_tlb_range（对整 2MiB）→ IPI 广播所有 mm_cpumask 核
5. 释放旧 PTE 表页（走 mmu_gather RCU 延迟）
```

**受害的不只是触发进程：** 同 mm 的其他线程、甚至借了该 mm 的内核线程都会吃 IPI。时间点不可预测（默认每 10s 一轮扫描）→ **尾延迟 p99.9 直接受染**。

**反方向的代价（关掉 THP 损失什么）：**
- TLB 覆盖从 2MiB/项 降回 4KiB/项 → 大工作集引擎 TLB miss 率上升（可用显式大页补回）
- 512 倍 PTE 表页内存 + 更深 walk
- ARM64 contiguous bit 能部分缓解但仅对 cacheline 对齐的连续 PTE

## 3. 决策表

| 场景 | THP 设置 | 理由 |
|------|----------|------|
| HFT 低延迟引擎（2MiB 大页显式管理） | **never** + `hugepages=N` boot 预留 | 消灭随机 collapse；TLB 收益由显式大页全覆盖 |
| 大内存 Java/数据库类同机租户 | madvise | 只让标记过的进程享受，不干扰别人 |
| 无法改代码的吞吐型负载 | always | 平均延迟下降，尾延迟不敏感 |
| 树莓派（小内存） | madvise 或 never | 小内存上 2MiB 粒度浪费大；无 NUMA 压力 |
| 行情回放/离线研究 | always | 吞吐导向，无实时约束 |

**配套必须做（选 never 时）：**

```bash
# boot: hugepages=2048           # 4GiB 大页池（按引擎预算）
# 运行期验证：
grep -i huge /proc/meminfo       # HugePages_Total/Free/Rsvd
cat /proc/pid/smaps | grep -A1 AnonHugePages   # 应为 0（THP 已禁）
```

## 4. THP vs hugetlbfs vs mTHP（易混三兄弟）

| | THP | hugetlbfs | mTHP（6.8+） |
|---|-----|-----------|--------------|
| 尺寸 | 仅 2MiB（PMD） | 2MiB/1GiB 池 | 16K~1M 中间尺寸 |
| 分配 | 自动/后台 | 显式预留池 | 缺页按档 |
| 可靠性 | 内存碎则静默退 4K | 预留即保证 | 退下一档 |
| 拆分 | 随时（迁移/mprotect） | **不能**拆 | 可 |
| swap | 可换出（5.8+） | 不能 | 可 |
| HFT 首选 | ✗ | **✓** | 观望（6.8+ 再说，v6.6 无） |

**注意 v6.6 边界：** mTHP 是 6.8 引入，v6.6 的 `/sys/kernel/mm/transparent_hugepage/` 下 **没有** 按尺寸分档目录；THP swap-out 是 5.8+ 支持（早期"THP 不能 swap"的说法已过时）。

## 5. 观测与排障

```bash
cat /sys/kernel/mm/transparent_hugepage/enabled
grep AnonHugePages /proc/<pid>/smaps | awk '{s+=$2} END{print s}'  # 实际吃了多少 THP
grep -w 'thp_fault_alloc\|thp_collapse_alloc\|thp_split_pmd' /proc/vmstat
# 尾延迟怀疑 THP 时（bpftrace）：
bpftrace -e 'kprobe:collapse_huge_page { @[comm]=count(); }'
```

| vmstat 计数 | 含义 |
|-------------|------|
| `thp_fault_alloc` | 缺页直接拿到 2MiB 次数 |
| `thp_collapse_alloc` | khugepaged 成功合并次数（**HFT 关注：非零即异常**） |
| `thp_split_pmd` | PMD 被拆回次数（mprotect 等触发） |
| `thp_deferred_split` | 挂延迟拆队列（内存紧张才拆） |

## 6. 衔接

- [§6 TLB 管理](./section-6-TLB-与-L1-Cache-管理.md)：collapse→shootdown 成本模型
- [§1 页目录与页表项](./section-1-页目录与页表项.md)：PMD 大页在层级里的位置
- [Ch 10 页框回收](../../chapter-10-page-frame-reclamation/)：THP 的碎片化压力与 compaction 的因果
- [13-DPDK EAL 大页](../../../13-dpdk/01-Intro-Book/notes/chapter-01-DPDK架构与EAL.md)：显式大页的工程用法
- [05.5 现代内核](../../../05.5-modern-kernel/)：调度/NUMA 视角的配套隔离

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：THP=always 下，缺页处理为什么可能变慢？**
A：`do_huge_pmd_anonymous_page` 要求 **2MiB 物理连续**。内存碎片化后 alloc 失败 → 触发 direct compaction（同步搬移页面凑连续块，**毫秒级**）→ 再失败才退回 4K 页。同步 reclaim/compaction 缺页 = 用户态一次普通写入随机卡几毫秒。`/sys/kernel/mm/transparent_hugepage/defrag` 子开关（`defer`/`defer+madvise`）就是控这个的。

**Q2：khugepaged 为什么默认敢全机扫描？设计假设是什么？**
A：假设是"吞吐型服务器"。scan_sleep 默认 10s、collapse 对吞吐负载净收益为正。HFT 属于假设外人群——正解不是调 khugepaged 参数而是整个关掉（never 下 `start_stop_khugepaged()` 自动停线程，huge_memory.c:274）。

**Q3：`MADV_HUGEPAGE` + madvise 模式下，引擎热区想享受大页又不想后台被 collapse，可行吗？**
A：可行且是折中良方：缺页路径直接给 PMD 大页（已经是终态），khugepaged 对大页区域无事可做。剩余风险是 split（mprotect/迁移），运行期不动映射布局即可控。

**Q4：THP 大页能被 swap out 吗？能被迁移吗？**
A：v6.6 两者都支持（swap-out 5.8+，迁移是 compaction/NUMA balancing 的常规操作）。但每次迁移以 2MiB 为单位——比 4K 页贵 512 倍且持更多锁。auto NUMA balancing 开着时这又是隐藏抖动源；HFT 建议 `numa_balancing=0`。

**Q5：怎么证明一次尾延迟尖刺是 THP 引起的？**
A：三条证据链：① 事发时刻 `/proc/vmstat` 的 `thp_collapse_alloc`/`thp_split_pmd` 是否 +1；② bpftrace 挂 `collapse_huge_page`/`split_huge_pmd` kprobe 记时间戳，或 `perf record -e irq_vectors:*call_function*`；③ `/proc/<pid>/smaps` 的 AnonHugePages 面积在事发前后变化。三者对上即可定罪。

</details>

---
