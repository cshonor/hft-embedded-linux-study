## 5.3 编程语言与垃圾回收

### 执行方式对比

| 类型 | 例子 | 性能特征 |
|------|------|----------|
| **编译型** | C、C++、Rust | 静态优化、`gcc -O3` / LTO；延迟可预测 |
| **解释型** | Python、早期 Ruby | 启动快、峰值慢；量化热路径慎用 |
| **VM + JIT** | Java、C# | 预热后接近原生；预热期与 deopt 需关注 |

**编译优化级别（C/C++）：**

- `-O0`：调试
- `-O2`：生产默认
- `-O3`：激进内联、向量化 — **需 benchmark 验证**，有时反而变大导致 I-cache miss
- `-flto`：链接期优化

**HFT：** 策略核心多为 **C++ / Rust**；研究层 Python 可以，但**不能把解释型路径放上 tick 热路径**。

→ [13-Rust Guide](../16-Rust-Quant-Trading-Guide/) 零成本抽象 vs GC 语言

### 垃圾回收（GC）

自动内存管理的代价：

| 问题 | 表现 | 对策 |
|------|------|------|
| **内存膨胀** | 堆一直涨 | 对象池、复用 buffer |
| **GC CPU** | 年轻代频繁 minor GC | 少短命对象、`-XX:+AlwaysPreTouch` |
| **Stop-the-world** | **P99/P999 尖刺** | 选低延迟 GC（ZGC、Shenandoah）、堆 sizing |
| **分配速率** | 分配越快 GC 越勤 | 逃逸分析、栈上对象、off-heap |

**HFT 经验法则：**

- **tick 路径：** 无分配、无 GC — C++/Rust 或 Java 里把热路径做成 **off-heap + 预分配**。
- **监控：** GC log + **延迟热力图** 对齐，看尖刺是否与 Full GC 重合。

---


---

← [本章导读](../README.md)
