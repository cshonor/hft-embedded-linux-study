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

→ [13-Rust Guide](../22-rust-quant/) 零成本抽象 vs GC 语言

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


### 常见陷阱

1. GC 语言做 HFT 热路径——Java/Go 的 GC pause 即使是 ms 级也远超 HFT 预算，热路径应用 C++
2. C++ 不管内存分配——默认 malloc 有锁竞争和碎片，HFT 应用对象池/arena/pre-allocated
3. 不测 GC/分配器实际暂停——声称「无 GC」但不测，实际有 major page fault 或 malloc stall

<details>
<summary>自测题（点击展开）</summary>

1. HFT 热路径为什么通常用 C++ 而非 Java/Go？
   <details><summary>答</summary>GC pause 即使 ms 级也远超 HFT 微秒级预算——C++ 手动内存管理可预分配/池化，无暂停</details>
2. C++ 热路径内存管理应该怎么做？
   <details><summary>答</summary>对象池/arena/pre-allocated——热路径零 malloc，避免分配器锁和 page fault</details>
3. 如何验证热路径真的「无暂停」？
   <details><summary>答</summary>BPF 追踪 malloc/page-fault/sched 时长——看热路径线程是否有非预期事件</details>

</details>


---

← [本章导读](../README.md)
