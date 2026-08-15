## 5.12 理解内存性能（5.12.1–5.12.2）

### 5.12.1 加载的性能

- **load 延迟** — L1 hit ~4 周期量级；miss 到 DRAM **上百周期**
- **load-use** — load 结果就绪前，依赖它的指令 stall（Ch4 PIPE）
- **多条 load 并行** — 若地址独立、命中 cache，可多发射

**优化方向：**

- 提高 **局部性** — 顺序扫数组（→ [Ch 6](../../chapter-06-memory-hierarchy/)）
- **预取** — `__builtin_prefetch` 对下一 cache line
- 减少 **指针追踪** — 链表 vs 数组

### 5.12.2 存储的性能

- **store** 通常不阻塞 retirement（写缓冲），但 **load 依赖 store** 时需等地址解析
- **写后读 (WAR)** 同地址 — 内存依赖链

**HFT：**

- **SoA** 批量写行情字段 vs **AoS** 单条更新 — 按访问模式选
- **false sharing** — 多线程写相邻字段 → 同一 cache line（→ Ch3 [3.9](../../chapter-03-machine-level-programs/notes/section-3.9-结构体联合与对齐.md)）
- `perf`：`mem-loads`、`L1-dcache-load-misses`、`cache-misses`

---

### 常见陷阱

1. **load-use 冒险被忽略** — load 的数据要到 M 级才出来，紧接着的指令在 D 级就要用 → 必须 stall 一拍。热循环避免 load 后立刻依赖，或用预取/重排让编译器插入独立指令。
2. **store-load 同地址依赖** — 先 store 再 load 同一地址，CPU 必须等 store 地址解析完才能确认 load 是否读 store 的值（store-to-load forwarding）。避免在热循环中写后读同一地址。
3. **预取用错反而变慢** — `__builtin_prefetch` 预取太远（数据还没到使用时间，被挤出 cache）或太近（来不及预取）都无效。正确预取距离 ≈ cache miss latency / 每元素处理时间。

### 自测题

<details>
<summary>1. load 延迟取决于什么？L1 hit 和 DRAM miss 差多少？</summary>

取决于**cache 命中层级**：L1 hit ~4 周期，L2 hit ~10-15 周期，L3 hit ~40 周期，DRAM miss **上百周期**。HFT 热数据必须留在 L1/L2，一次 DRAM miss 就能导致毛刺。
</details>

<details>
<summary>2. 为什么多条独立 load 可以并行？什么情况不行？</summary>

如果多个 load 的**地址独立**且都命中 cache，CPU 的超标量/乱序引擎可以同时发射多条 load 指令。不行的情况：①地址依赖（load B 的地址需要 load A 的结果——指针追踪/链表）；②cache miss（miss 会占用 MSHR，限制后续 miss 并行度）；③load-use 冒险（紧接着依赖 load 结果）。
</details>

<details>
<summary>3. false sharing 是什么？HFT 中怎么避免？</summary>

多线程写**同一 cache line 的不同字段** → cache line 在核心间反复 invalidate/transfer，性能暴跌。避免方法：①`alignas(64)` 让每个线程的独立数据独占 cache line；②SoA 布局让同一线程访问的数据连续；③只读数据共享无妨（只读不触发 invalidate）。
</details>

---

← [本章导读](../README.md)
