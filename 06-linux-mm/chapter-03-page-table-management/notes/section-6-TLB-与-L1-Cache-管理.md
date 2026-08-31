# Ch 3 §6 TLB 与 L1 Cache 管理

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**
> 源码核验：Linux **v6.6**（`mm/mmu_gather.c`、`arch/*/include/asm/tlb.h`、`arch/*/mm/tlb.c`）

---

## 本节讲什么

TLB 是 VA→PA 的硬件 cache，**但它不保证与页表同步**。内核改了页表，必须主动让 TLB 里的旧翻译失效——这个"同步税"是 HFT 尾延迟的最大单点来源之一。本节讲失效 API 面、多核 shootdown 成本模型、以及 lazy TLB 优化。

---

## 1. 基本事实

| 事实 | 数据（现代 x86_64/ARM64 server） |
|------|----------------------------------|
| L1 dTLB | ~64 项，4KiB 页 → 覆盖 256KiB |
| L2 STLB | ~1.5K–2K 项，混合页大小 → 覆盖数十 MiB（含 2MiB 项） |
| 2MiB 大页 | 单项覆盖 = 4KiB 的 512× → 同样项数覆盖暴涨 |
| TLB hit | ~0 周期（与 L1 并行） |
| TLB miss→walker | 数十~上百周期（4 次访存链） |
| **TLB shootdown** | **IPI 往返 + 对方 flush，几百 ns~µs 级**（跨核越多越贵） |

**结论先行：** TLB miss 是"稳态税"（可用大页压），shootdown 是"事件税"（必须少触发）。HFT 优化顺序：先消灭 shootdown，再压 miss。

## 2. 失效 API 面（架构无关层）

| Hook | 粒度 | 典型调用者 |
|------|------|-----------|
| `flush_tlb_all()` | 全核全表 | 极少（内核自身重映射） |
| `flush_tlb_mm(mm)` | 该 mm 全部 | `mprotect`/`munmap` 大区间（mmu_gather 收口） |
| `flush_tlb_range(vma, start, end)` | 地址区间 | zap/protect 路径（能单页就别整 mm） |
| `flush_tlb_page(vma, addr)` | 单页 | `ptep_clear_flush` 单点操作 |
| `flush_tlb_kernel_range` | 内核 VA | vmalloc unmap、模块卸载 |

**关键语义：** 这些函数返回 ≠ TLB 已干净。x86 的 INVLPG/CR3 写是同步的，但 **跨核** 必须 IPI——`mmu_gather` 的三段式（收集→flush→free，见 [§3](./section-3-页表的分配与释放.md)）就是为此设计。单核视角"我 flush 完了"，别的核可能还在路上。

## 3. 多核 shootdown：成本模型

```
CPU0: 改 PTE ──► 需要所有"用过该 mm 的核"失效
        │
        ├─ smp_call_function_many(mask=mm_cpumask)   ← IPI 广播
        │       CPU1: 进 IPI handler，flush 本核 TLB
        │       CPU2: 正在跑长原子区/中断关闭…… ← **等待者 CPU0 自旋/软等待**
        ▼
      全员 ack 后，CPU0 才能安全 free 表页
```

| 成本项 | 量级 | HFT 影响点 |
|--------|------|-----------|
| IPI 发送+处理 | 每核 ~1µs 内 | 被广播核的 **中断被劫持**——执行线程出现计划外抖动 |
| 等最慢的核 | 无上界（对方 irq 关闭多久就等多久） | `tlb_flush_pending` 期间页表释放停滞 |
| PCID/ASID | x86 PCID 12-bit / ARM64 ASID 16-bit | 免整表 flush，只刷指定地址空间——**必须开**（默认开） |

**谁在触发 shootdown（用户态可感知清单）：**

1. `mprotect`/`munmap`/`madvise(MADV_DONTNEED)`
2. THP collapse/split（khugepaged 折叠 511 个 4K 页 → PMD 大页，全局 flush）
3. `mlock` 解锁收缩、`mremap`
4. fork 后 COW 写缺页（只单页 flush，便宜）
5. `process_vm_writev`、ptrace 改对方页表

## 4. Lazy TLB：内核线程的免单

上下文切换到内核线程（kworker/softirqd/kswapd）时：

```c
/* 概念代码（arch tlb.c 的 switch_mm） */
if (next->mm == NULL) {                  /* 内核线程 */
    next->active_mm = prev->active_mm;   /* 借用，不换 CR3/TTBR0 */
} else {
    switch_mm_irqs_off(prev->active_mm, next->mm, next);
}
```

**收益：** 内核线程不写 TTBR/CR3 → 用户 TLB 项不失效；切回原进程时发现 CR3 没变 → **零 flush**。

**代价：** 借用期间 `mm_cpumask` 记着该核——用户进程改页表时，IPI 也要发给"正在跑内核线程借用我 mm 的核"。**HFT 数值线程绑核 + 内核线程被赶到同核**（isolcpus 隔离不彻底时）会把这税带进来。

## 5. L1 cache 别名问题（原书重点，64 位已淡化）

32 位 VIPT cache 时代：同物理页两个 VA 映射 → cache 里可能出现 **两份缓存行**（alias）→ 写一个另一个不失效 = 数据不一致。解法是 `flush_cache_*` 家族 + 限制映射颜色。

64 位 x86_64/ARM64：L1 为 PIPT（物理索引）→ **别名问题硬件消除**，`flush_cache_page` 等退化为空或仅 D-cache 维护（ARM64 特定场景 `__flush_dcache_page` 仍真实存在——非一致性设备 DMA 用）。原书这块按"历史机制"读即可。

## 6. v6.6 新东西

| 特性 | 内容 | 相关 |
|------|------|------|
| `tlb_remove_table` RCU 批量释放 | 表页经 RCU grace 后 free，与 `pte_offset_map` 读侧并发安全 | mmu_gather.c |
| ARM64 contiguous TLB bit | 一批连续 PTE 标记 contiguous → 硬件按大项缓存 | 次级大页效果，不占 PMD |
| `CONFIG_MMU_GATHER_RCU_TABLE_FREE` | 默认开（SMP） | 减少同步等待 |
| 批量上限 | `MAX_GATHER_BATCH_COUNT`（mmu_gather.c:32） | munmap 数 GB 时分批 flush |

## 7. HFT 配置范式（本节的总出口）

```bash
# 1) 消灭 shootdown 源
transparent_hugepage = never          # 防 khugepaged collapse 抖动（见 THP 笔记）
                                      # 显式 hugetlbfs 大页不受影响
# 2) 压 TLB miss
hugepages = <预算>                     # 2MiB 大页给行情/订单簿热区
# 3) 别让别的核替你交税
isolcpus=<数值核> nohz_full=<数值核> rcu_nocbs=<数值核>
# 4) 确认 PCID/ASID 生效
grep -o 'pcid' /proc/cpuinfo          # x86: flags 里应见 pcid
# 5) 运行期自证：shootdown 计数（perf）
perf stat -e dTLB-load-misses,dTLB-loads ./engine
bpftrace -e 'kprobe:flush_tlb_mm { @[comm] = count(); }'
```

## 8. 衔接

- [§3 页表的分配与释放](./section-3-页表的分配与释放.md)：三段式 flush 的另一半
- [note-透明大页THP](./note-透明大页THP.md)：THP 与 shootdown 的取舍决策
- [06.5/ch04 页表与 TLB](../../../06.5-modern-mm/chapter-04-page-table-tlb/)
- [15-体系结构 TLB](../../../15-computer-architecture/chapter-02-memory-hierarchy-design/)：硬件视角
- [12.5/ch13 TCP_ZEROCOPY_RECEIVE](../../../12.5-modern-networking/chapter-13-zerocopy-highperf/notes/04-tcp-zero-copy-recv.md)：mmap 换页 + TLB 成本实例

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：为什么 HFT 机器"关 THP 用显式大页"而不是"开 THP"？**
A：THP 的 collapse 由 khugepaged 后台触发——**把 511 个已映射的 4K 页改写成 1 个 PMD 大页映射**，这个动作需要全核 shootdown + 持 mmap_lock，发生在你无法控制的时刻 = 随机尾延迟。显式 hugetlbfs 大页在 mmap 时就位，运行期不再动。THP 笔记有全表对比。

**Q2：单核改自己进程的 PTE（如缺页填表），也要 IPI 别的核吗？**
A：不用。缺页是"往空表项装新映射"——旧翻译不存在，别的核 TLB 没有该项，无需失效。只有 **清除/改变已有映射** 才 shootdown。这也是 prefault（提前填表）零 shootdown 成本的原因。

**Q3：PCID 是什么，为什么 HFT 必须确认它开着？**
A：x86 的进程上下文 ID（12-bit tag）：CR3 切换不自动全表失效，TLB 项按 PCID 分桶。多线程同核切换/内核进出时保留用户 TLB。关掉则每次上下文切换全 flush——syscall 密集型引擎每次进出内核都丢 TLB。ARM64 的对应物是 ASID（16-bit），作用相同。

**Q4：`flush_tlb_range` 之后表页为什么还不能立刻 free？**
A：`flush_tlb_range` 返回只代表**本核**发起完成；其他核的 IPI handler 可能尚未执行。mmu_gather 模型里 free 推迟到 `tlb_finish_mmu`/RCU 之后，就是这个等待的封装。直接 free = 竞态窗口里 walker 读到已复用的表页。

**Q5：两个线程跑在不同核、共享 mm，一个线程 munmap 一段映射，另一核的线程会付出什么？**
A：被 IPI 打断（进入 flush handler，数十~百 ns），并且如果它恰在 walk 那段页表（GUP/缺页），会被 mmap_lock 或 pte 锁挡住。所以多线程引擎应 **避免运行期动态 mmap/munmap**——启动期定格映射布局。

</details>

---
