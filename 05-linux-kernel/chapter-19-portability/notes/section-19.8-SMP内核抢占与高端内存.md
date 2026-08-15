## ⑧ SMP、内核抢占与高端内存

可移植 ≠ 只换 CPU — 还要兼容 **内核配置**。

| 配置 | 编写假设 |
|------|----------|
| **SMP** | **始终** 可能真并发 — **锁 / per-CPU** |
| **内核抢占** | 临界区可被插 — **短临界区** · `preempt_disable` 仅当合理 |
| **HIGHMEM** | 可能需 **`kmap`/`kmap_atomic`** — 勿假设线性映射 |

> **原则：按「最坏情况」写** — 单核 UP、关抢占、无 HIGHMEM 的「侥幸」代码 **迟早炸**。

| 章节回溯 | |
|----------|--|
| SMP/锁 | **Ch 9–10** |
| 抢占 | **Ch 4** |
| HIGHMEM | **Ch 12** |

**HFT：** 用户态也要 **按多核+弱序** 写无锁结构 — `memory_order`、对齐、false sharing。



<details>
<summary>自测题（点击展开）</summary>

**Q1.** CONFIG_PREEMPT 和 CONFIG_PREEMPT_NONE 对 HFT 有什么影响？

<details><summary>答案</summary>

PREEMPT_NONE：内核态不可抢占（syscall 执行完才调度）→ 延迟高但吞吐好。PREEMPT（ voluntary）：内核态可自愿抢占点 → 平衡。PREEMPT_RT（实时）：几乎所有内核代码可抢占 + 自旋锁转 mutex → 延迟最小但吞吐损失。HFT 通常用 PREEMPT_RT 补丁内核，保证交易线程的调度延迟 < 100μs。

</details>

</details>
---
