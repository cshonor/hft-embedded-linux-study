# 第 30 章 · Optimization（优化） · 本章定位

← [本章目录](./README.md) · 下一节：[01-hash-table-probe-optimization.md](./01-hash-table-probe-optimization.md)

---

全书 **Lagniappe（额外赠礼）** · **最后一章含新代码**。用 **Benchmark + Profiler** 找热点，实施两项 **底层微优化** —— 不改 Lox 语义，只压榨 **VM 实现**。

| 工具 | 用途 |
|------|------|
| **`clock()` native** | 基准计时（ch10 jlox / ch24 clox 已备） |
| **Profiler** | 指令 / 函数 **热点占比** |

| 小节 | 主题 |
|------|------|
| **§30.1～§30.2** | 哈希表 **掩码** 替 **`%`** |
| **§30.3** | **NaN boxing** · Value **16B → 8B** |

---
