## 6.4.7 Cache 参数的性能影响

> **Ch6 §6.4.7** · [章导读](../README.md) · 上节 [§6.4.6 ←](./section-6.4.6-真实Cache层次解剖.md) · 下节 [§6.5 →](./section-6.5-编写高速缓存友好的代码.md)
> ↔ [Harris §8.2 性能分析](../../../00-digital-logic-cpu/ch08_memory/8.2_存储器系统性能分析.md)
> ↔ [Hennessy §2.3 缓存优化](../../../18-computer-architecture/chapter-02-memory-hierarchy-design/notes/section-2.3-缓存性能十项高级优化.md)

---

← [本章导读](../README.md)

---

### 常见陷阱

1. **以为增大 cache 容量就能解决所有 miss** — 容量增大降 capacity miss，但不降 conflict miss（如果相联度不变）和 compulsory miss（第一次访问必 miss）。不同 miss 类型需要不同对策。
2. **忽略 cache line 大小对性能的双刃剑效应** — 大 line（128B）提高空间局部性（一次拉更多数据），但增加 miss penalty（拉更多字节）和 false sharing 概率。64B 是当前主流折中。
3. **混淆三种 miss 类型** — Compulsory（冷启动必 miss）、Capacity（工作集 > cache 容量）、Conflict（相联度不够）。HFT 优化重点在 capacity 和 conflict miss。


### 自测题

<details>
<summary>1. 三种 cache miss 类型分别是什么？各如何缓解？</summary>

- **Compulsory（冷缺失）**：第一次访问必 miss → 预取（prefetch）
- **Capacity（容量缺失）**：工作集 > cache 容量 → 分块（blocking/tiling）使子块 fit cache
- **Conflict（冲突缺失）**：相联度不够，同组 thrashing → 增大相联度 / 调整数据布局避免同组映射
</details>

<details>
<summary>2. 增大 cache line 大小的利弊是什么？</summary>

**利**：提高空间局部性——一次拉 128B 比 64B 多覆盖一倍数据，顺序访问 miss 减半。**弊**：①miss penalty 增加（拉更多字节）；②false sharing 概率增加（更多线程数据可能落在同一 line）；③浪费带宽（如果只用其中一小部分）。64B 是当前主流折中。
</details>

<details>
<summary>3. HFT 中哪种 miss 最致命？为什么？</summary>

**Capacity miss 和 conflict miss**。Compulsory miss 只在第一次访问发生，后续有 cache；但 capacity/conflict miss 在热循环中**反复发生**，每次 miss penalty ~100ns（到 DRAM），直接导致延迟毛刺。HFT 优化重点：cache 友好的数据布局（降 conflict）+ 控制工作集大小（降 capacity）。
</details>

---

← [§6.4.6 ←](./section-6.4.6-真实Cache层次解剖.md) · [本章导读](../README.md) · [§6.5 →](./section-6.5-编写高速缓存友好的代码.md)
