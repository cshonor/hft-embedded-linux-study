## 6.7 小结（原书）

> ↔ [Hennessy §2.7 谬误与陷阱](../../../15-computer-architecture/chapter-02-memory-hierarchy-design/notes/section-2.7-谬误与陷阱.md)

> **Ch6 §6.7** · [章导读](../README.md) · 上节 [§6.6 ←](./section-6.6-存储器山.md) · 下节 —

---

← [本章导读](../README.md)

---

### 常见陷阱

1. **学完 Ch6 就以为 cache 优化做完了** — Ch6 讲的是单核 cache 原理；多核场景还有 MESI 一致性协议、false sharing、NUMA 等问题（→ Hennessy Ch2/Ch5）。
2. **只优化数据 cache 忽略指令 cache** — 热函数太大/跳转分散导致 I-cache miss。用 `perf stat -e iCache-misses` 检查。


### 自测题

<details>
<summary>1. Ch6 全章最核心的三个教训是什么？</summary>

①**局部性决定性能**——时间局部性（复用数据）+ 空间局部性（顺序访问）；②**cache line 是最小单位**——即使读 1 字节也拉 64B，布局要对齐 cache line；③**工作集要 fit cache**——超过容量就 capacity miss，用分块（blocking）控制。
</details>

<details>
<summary>2. HFT cache 优化的完整检查清单是什么？</summary>

①热数据是否连续存储（数组/vector/节点池）；②循环是否按行扫（stride=1）；③工作集是否 fit L1/L2；④结构体字段是否紧凑（热字段放前面）；⑤是否 false sharing（多线程写同一 cache line）；⑥是否每包 malloc（改预分配）；⑦`perf stat` 检查 `cache-misses`/`L1-dcache-load-misses`。
</details>

---

← [§6.6 ←](./section-6.6-存储器山.md) · [本章导读](../README.md) · —
