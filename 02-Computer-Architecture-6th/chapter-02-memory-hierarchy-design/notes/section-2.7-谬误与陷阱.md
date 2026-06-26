## 2.7 谬误与陷阱

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
3. 读 [Ch5 线程级并行](../../../chapter-05-线程级并行.md) — 多核一致性  
4. 读 [Ch3 ILP](../../../chapter-03-instruction-level-parallelism/) — 分支与乱序执行  

---
