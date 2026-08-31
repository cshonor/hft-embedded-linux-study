# Ch 6 §5 避免碎片化（迁移类型 / compaction / HFT 大页保卫战）

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**
> 源码核验：Linux **v6.6**（`mm/compaction.c`、`mm/page_alloc.c` :226/:3383）

---

## 本节讲什么

外部碎片是 buddy 的原罪：拆分-合并只保证"空闲时能拼回"，**只要块里钉着一页不可移动的钉子，整块连续性就永久报废**。v6.6 的抗碎片化 = 分区（migratetype）+ 搬迁（compaction）+ 预留（CMA/hugetlb）。HFT 的大页可用性完全由这场战争决定。

---

## 1. 碎片两分法（复习+深化）

| 类型 | 含义 | 缓解手段 |
|------|------|----------|
| **内部碎片** | 分到 2^k 页只用一部分 | slab 切小对象（Ch 8） |
| **外部碎片** | 总空闲够但无大连续块 | migratetype 分区 + compaction |

**关键认知：外部碎片不可完全消除，只能"分区管理"。** 把不可移动的东西关进自己的区，保住 MOVABLE 区的大块纯度——这是 v6.6 的核心策略。

## 2. 抗碎片三件套

### ① Migratetype 分区（预防）

见 [§1](./section-1-空闲块的管理.md)。pageblock 级标签，分配按类入座。**纯度**是关键指标：

```bash
cat /proc/pagetypeinfo      # 看 MOVABLE 高阶行的余量 = 大页健康度
```

### ② Compaction（治疗）

```
kcompactd（后台）/ direct compaction（分配路径同步触发）
  扫描 zone：一端找空闲页，另一端找可迁移页
  → 把 MOVABLE 页搬走（改 PTE+TLB flush+拷贝内容）
  → 腾出连续高阶块 → 交给请求者（常是 THP/大页）
```

| 事实 | 实锚 |
|------|------|
| 同步入口 | `try_to_compact_pages`（page_alloc.c:3383 分配慢路径调用） |
| 重试判定 | `should_compact_retry`（:3427） |
| 成本 | 迁移一页 = 内容拷贝 + PTE 改写 + **TLB shootdown**——毫秒级整段停顿 |
| HFT 关系 | THP=always 机器的尾延迟尖刺头号嫌疑 |

**迁移的可能**依赖"页真的 MOVABLE"：pin 住的（GUP/mlock）、UNMOVABLE 类型、内核直映都不能搬——**每 pin 一页，就给 compaction 增加一分不可为**。

### ③ 预留（隔离带）

| 手段 | 语义 |
|------|------|
| hugetlb 预留 | boot 期直接扣走，与 buddy 永久隔离——**最硬的保证** |
| CMA | 启动预留，空闲期借给 MOVABLE，需要时赶人（设备 DMA 场景） |
| `MIGRATE_HIGHATOMIC` | 高阶请求的迷你储备，抗抖动 |

## 3. 碎片化的时间线（一台 HFT 机器的自毁过程）

```
Day 0   开机：MOVABLE 区纯净，order-9 充足，THP 全中
Day N   各种驱动高阶 kmalloc 偷页（§2 steal），UNMOVABLE 钉子散布
        引擎 pin 住 GB 级页（GUP/mlock），大片区域失去可迁移性
Day M   pagetypeinfo 的 order-9 MOVABLE 行清零
        THP 缺页 → direct compaction → 同步毫秒级停顿（尾延迟尖刺）
        或失败退 4K → TLB miss 率上升（稳态延迟缓涨）
```

**对策按阶段：** Day 0 就关 THP 用显式大页（预约式）；运行期监控 `pagetypeinfo` 与 `compact_stall`；绝不运行期动态 pin 大量页。

## 4. 观测与量化

```bash
# 碎片体检
cat /proc/pagetypeinfo | head -30
# 压缩压力
grep -w 'compact_stall\|compact_fail\|compact_success' /proc/vmstat
# 迁移失败（compaction 搬不动的页）
grep -w 'compact_isolated\|compact_migrate_scanned\|compact_free_scanned' /proc/vmstat
# 大页水位
grep -i huge /proc/meminfo
```

| 指标组合 | 诊断 |
|----------|------|
| compact_stall 高 + THP on | direct compaction 在吃你的尾延迟 |
| compact_migrate_scanned 高但 isolated 低 | 大量页搬不动（被 pin）——查引擎/驱动 |
| order-9 MOVABLE = 0 但 UNMOVABLE 高阶有 | 偷页混居晚期，重启或大整改 |

## 5. HFT 决策收束

| 策略 | 适用 |
|------|------|
| hugetlb 预留 + THP=never | 生产低延迟（12.5 系列反复出现的结论） |
| THP=madvise + MADV_HUGEPAGE 限定热区 | 无法全显式管理时的折中 |
| `min_free_kbytes` 上调 | 减压缓冲（治标） |
| `numa_balancing=0` | 免掉自动 NUMA 迁移的额外搬迁（它也是碎片源+抖动源） |
| 周期性 `compact_memory`（手动 echo） | 维护窗口触发——别在交易时段做 |

## 6. 衔接

- [§1 迁移类型](./section-1-空闲块的管理.md) / [§2 偷页](./section-2-页面分配.md)：碎片怎么被制造
- [Ch 3 THP](../../chapter-03-page-table-management/notes/note-透明大页THP.md)：compaction 的最大客户
- [Ch 10 回收](../../chapter-10-page-frame-reclamation/)：与 compaction 并列的另一个"治疗"手段
- [06.5/ch06 folio](../../../06.5-modern-mm/chapter-06-page-cache-folio/)：回收单元的现代化

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：compaction 和回收（reclaim）都"腾内存"，区别是什么？**
A：目标不同——reclaim 要 **空闲页数量**（释放 cache 页即可，零散的也行）；compaction 要 **连续性**（把页搬到一起挤出大块）。顺序上分配慢路径先 reclaim 后 compaction（内存都不够时搬迁无意义）。一个求量一个求形。

**Q2：为什么 compaction 迁移页要 TLB flush？谁付出？**
A：迁移 = 物理位置变 → PTE 改写 → 旧翻译必须失效。flush 是跨核 IPI——**被波及的是所有映射了该页的进程**（rmap 找出所有 PTE）。这就是"后台 compaction 也伤前端延迟"的机制根源。

**Q3：hugetlb 预留的页为什么不受碎片影响？**
A：它们从 memblock/buddy 早期就被 **整批摘出**，不在 free_area 里参与日常分配-合并循环——碎片化再严重也烧不到池子内部。代价：这部分内存别人永远用不了（除非手动缩池）。**隔离的代价是低利用率，收益是确定性**——HFT 用延迟买确定性。

**Q4：`echo 1 > /proc/sys/vm/compact_memory` 在生产机上安全吗？**
A：能做，但它就是全 zone 的同步 compaction——搬迁+TLB shootdown 全套。交易时段等于自残；只在维护窗口或盘后跑。自动触发（kcompactd）反而有节流和优先级控制，比手动 echo 温和。

**Q5：怎么证明引擎 pin 的页在阻碍 compaction？**
A：压测前后对比 `compact_migrate_scanned`（扫描候选数）与 `compact_isolated`（实际隔离数）——扫描远大于隔离 = 大量候选被 `page_mapcount`/pin 拒绝。再配 `bpftrace` 挂 `migrate_page` 统计迁移失败原因即可闭环。

</details>

---
