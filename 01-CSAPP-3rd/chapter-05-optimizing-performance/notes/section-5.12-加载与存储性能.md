## 5.12 理解内存性能（5.12.1–5.12.2）

### 5.12.1 加载的性能

- **load 延迟** — L1 hit ~4 周期量级；miss 到 DRAM **上百周期**
- **load-use** — load 结果就绪前，依赖它的指令 stall（Ch4 PIPE）
- **多条 load 并行** — 若地址独立、命中 cache，可多发射

**优化方向：**

- 提高 **局部性** — 顺序扫数组（→ [Ch 6](../../chapter-06-存储器层次结构.md)）
- **预取** — `__builtin_prefetch` 对下一 cache line
- 减少 **指针追踪** — 链表 vs 数组

### 5.12.2 存储的性能

- **store** 通常不阻塞 retirement（写缓冲），但 **load 依赖 store** 时需等地址解析
- **写后读 (WAR)** 同地址 — 内存依赖链

**HFT：**

- **SoA** 批量写行情字段 vs **AoS** 单条更新 — 按访问模式选
- **false sharing** — 多线程写相邻字段 → 同一 cache line（→ Ch3 [3.9](../chapter-03-machine-level-programs/notes/section-3.9-结构体联合与对齐.md)）
- `perf`：`mem-loads`、`L1-dcache-load-misses`、`cache-misses`

---

← [本章导读](../README.md)
