## 6.6 存储器山

> **Ch6 §6.6** · [章导读](../README.md) · 上节 [§6.5 ←](./section-6.5-编写高速缓存友好的代码.md) · 下节 [§6.7 →](./section-6.7-小结.md)

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

### 口述巩固 · 自测

1. （待口述补）本节核心一句话？

---

← [§6.5 ←](./section-6.5-编写高速缓存友好的代码.md) · [本章导读](../README.md) · [§6.7 →](./section-6.7-小结.md)
