## 2.7 谬误与陷阱


> ↔ [CSAPP §6.7 小结](../../../02-computer-systems/chapter-06-memory-hierarchy/notes/section-6.7-小结.md)

### 常见谬误

| 谬误 | 真相 |
|------|------|
| **用程序 A 的 cache 行为预测程序 B** | 访问模式差异极大；必须 **实测自己的热路径** |
| **容量越大总是越好** | 大 cache → 命中时间↑、功耗↑；存在甜点 |
| **峰值内存带宽 = 你的带宽** | 随机访问、跨 NUMA、多核争用远低于峰值 |
| **软件优化无关紧要** | 分块/数据布局常比换 CPU 更有效（优化 #7） |
| **在未考虑虚拟化的 ISA 上轻松做 VMM** | 极难；需硬件辅助（VT-x/SVM） |

---

### HFT 特有陷阱

| 陷阱 | 对策 |
|------|------|
| **false sharing** | padding、按核分片、每线程私有计数器 |
| **冷数据与热数据混在同一 cache line** | SoA、结构拆分 |
| **LLC 被同机其他进程污染** | `isolcpus`、 cgroup、专用机 |
| **THP 导致的延迟尖刺** | 显式 hugepage 或关闭 THP（环境相关） |
| **用 microbench 的 L1 命中率推断端到端** | 端到端含内核、网卡、队列 — [Ch1 执行时间](../../chapter-01-quantitative-design-fundamentals/notes/section-1.7-1.8-可靠性与性能量化.md) |

---

### 本章小结

Ch2 把 [Ch1 局部性](../../chapter-01-quantitative-design-fundamentals/notes/section-1.9-计算机设计的量化原则.md) 落实为 **SRAM/DRAM/HBM/Flash 层次 + 十项 cache 优化 + VM/TLB + 真实 CPU 案例**。

**HFT 下一步：**

1. `perf` 量 LLC miss / dTLB load miss  
2. 审查订单簿与行情结构体的 **cache line 布局**  
3. 读 [Ch5 线程级并行](../../chapter-05-thread-level-parallelism/) — 多核一致性  
4. 读 [Ch3 ILP](../../chapter-03-instruction-level-parallelism/) — 分支与乱序执行  


### 常见陷阱

- 用 microbench 的 L1 命中率推断端到端性能 — 端到端含内核态切换、网卡 DMA、队列等待；L1 命中率 99% 不等于端到端快
- 认为容量越大总是越好 — 大 cache → 命中时间↑、功耗↑；存在甜点，超出后收益递减甚至变差（tag 查找延迟增加）
- 峰值内存带宽 = 你的带宽 — 随机访问、跨 NUMA、多核争用远低于峰值；实际带宽可能只有标称的 10-30%

### 自测题（点击展开）

<details>
<summary>Q1. 为什么不能用程序 A 的 cache 行为预测程序 B？</summary>

访问模式差异极大 — 顺序扫描 vs 随机查找 vs 指针追逐 → miss rate、预取效果、bank 利用率完全不同。必须 **实测自己的热路径**（`perf stat -e cache-misses,LLC-load-misses`）。

</details>

<details>
<summary>Q2. THP 导致延迟尖刺的机制是什么？对策有哪些？</summary>

THP 后台碎片整理/合并 → khugepaged 线程扫描+合并 4KB 页为 2MB → 可能在热路径执行时触发 **内存整理/迁移** → 延迟尖刺。对策：1) 显式 hugepage（`mmap(MAP_HUGETLB)`）2) `echo never > /sys/kernel/mm/transparent_hugepage/enabled` 关闭 THP 3) 环境相关，需压测确认。

</details>

<details>
<summary>Q3. HFT 特有的 5 个 cache 陷阱是什么？各举一个对策。</summary>

1) false sharing → alignas(64)/per-thread 计数器
2) 冷热数据混 line → SoA/结构拆分
3) LLC 被其他进程污染 → isolcpus/cgroup/专用机
4) THP 延迟尖刺 → 显式 hugepage
5) microbench ≠ 端到端 → 实测完整路径

</details>
---
