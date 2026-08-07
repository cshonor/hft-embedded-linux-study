## 6.6 存储器山

> **Ch6 §6.6** · [章导读](../README.md) · 上节 [§6.5 ←](./section-6.5-编写高速缓存友好的代码.md) · 下节 [§6.7 →](./section-6.7-小结.md)
> ↔ [Harris §8.2 性能分析](../../../00-digital-logic-cpu/ch08_memory/8.2_存储器系统性能分析.md)

---

- 二维测试：**读数组** 随 **stride** 与 **working set** 变化测 **读吞吐 (MB/s)**
- **山脊** — stride 小 + working set < cache → 高吞吐
- **平地/悬崖** — 超出 L3 → 吞吐骤降

**实验（原书 `mountain`）：** 亲眼见 **stride 8 元素 vs 1 元素** 差一个数量级。

### 6.6.2 重新排列循环提高空间局部性

- **矩阵乘、卷积、order book 批量统计** — 循环顺序决定性能
- 编译器 `-O3` 可能自动 **循环交换 (interchange)**，但别完全依赖

### 6.6.3 在程序中利用局部性

- **分块 (blocking/tiling)** — 使子块 fit L1/L2
- **融合 (fusion)** — 多次扫合并成一次（减总带宽）
- **预取** — 软件 `__builtin_prefetch` 对 predictable stride

**HFT 工作流：**

```
改布局/循环 → microbench (CPE/MB/s) → perf cache-misses → 端到端 P99
```

---

### 常见陷阱

1. **以为 stride 越小越好** — stride=1 确实空间局部性最好，但如果 working set 超过 cache 容量，stride=1 仍然 capacity miss。存储器山展示的是 stride **和** working set 的二维关系，不是只看 stride。
2. **分块（blocking/tiling）忘记处理边界** — 分块后每个子块要 fit L1/L2，但矩阵尺寸不一定是块大小的整数倍，需要处理尾部剩余行列。
3. **预取距离算错** — 预取太远（数据还没到使用时间，被挤出 cache）或太近（来不及预取）都无效。正确距离 ≈ cache miss latency / 每元素处理时间。

### 自测题

<details>
<summary>1. 存储器山展示的是什么关系？山脊和悬崖分别代表什么？</summary>

二维测试：读数组时**读吞吐（MB/s）**随 **stride**（步长）和 **working set size**（工作集大小）变化。**山脊**：stride 小 + working set < cache → 高吞吐（cache 命中）；**悬崖**：working set 超出 L3 → 吞吐骤降（DRAM miss）。亲眼见 stride=8 元素 vs 1 元素差一个数量级。
</details>

<details>
<summary>2. 分块（blocking/tiling）是什么？为什么能提高性能？</summary>

把大矩阵运算拆成小块，使每个子块 **fit L1/L2 cache**。例如矩阵乘法 C=A×B，把 A、B、C 分成 64×64 的子块，每个子块 64×64×8B=32KB fit L1。子块内的运算 cache 命中率高，总 miss 数大幅减少。**代价**：需要处理块边界（矩阵尺寸不是块大小整数倍）。
</details>

<details>
<summary>3. 循环融合（fusion）是什么？HFT 中什么时候用？</summary>

把多次遍历同一数据的循环**合并成一次**。例如 `for: sum += a[i]; for: max = max(max, a[i]);` 融合成 `for: { sum += a[i]; max = max(max, a[i]); }`——只扫一次数组，减少总 cache miss 和内存带宽消耗。HFT 中 tick 数据批量统计时常用。
</details>

---

← [§6.5 ←](./section-6.5-编写高速缓存友好的代码.md) · [本章导读](../README.md) · [§6.7 →](./section-6.7-小结.md)
