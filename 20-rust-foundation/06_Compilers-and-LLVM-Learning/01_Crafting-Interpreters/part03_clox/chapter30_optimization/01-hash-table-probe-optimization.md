# 第 30 章 · Optimization（优化） · §30.1～§30.2 哈希表探测优化（Hash Table Probe Optimization）

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-nan-boxing.md](./02-nan-boxing.md)

---

**Profiler 发现**：**哈希表探测** 中 **`hash % capacity` 取模** 占 **极高 CPU 时间**（方法表 · globals · fields · intern…）。

**前提**：**`capacity` 始终为 2 的幂**（ch20 扩容 **`capacity *= 2`** 已保证）。

**位运算替换**：

```c
// 慢
uint32_t index = hash % capacity;

// 快
uint32_t index = hash & (capacity - 1);   // 掩码 Masking
```

| 原理 | 说明 |
|------|------|
| **2^n 取模** | 等价 **保留低 n 位** |
| **`& (cap-1)`** | 单条 **AND** · 比 **IDIV** 快一个数量级 |
| **缓存 / 流水线** | 热点在 **inner loop** 时每条探测都省 |

**书中效果**：方法/变量密集 benchmark **~2×** 加速（仅改这一处）。

**对照 [ch20](../chapter20_hash-tables/README.md)**：开放寻址 + 线性探测 · 优化 **index 计算** 即优化 **整条 lookup 链**。

**本仓库延伸**：[04_Learn-LLVM-17](../../../04_Learn-LLVM-17/) · **O0 vs O3** · 编译器也会做类似 strength reduction。

---
