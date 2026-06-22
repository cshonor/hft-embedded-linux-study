## 9.10 垃圾收集（9.10.1–9.10.3）

### 基本概念

- **GC** — 自动回收不可达对象；**Java/Go** 默认有
- **可达性** — 从根（栈、全局）沿指针图遍历

### Mark & Sweep

1. **标记** 所有可达块
2. **清除** 未标记块回空闲链表

### C 的保守 GC

- 把 **栈/寄存器里像指针的位模式** 都当根 — **可能漏标或误留**
- 生产 C/C++ **极少** 用；HFT 用 **RAII / 池 / Rust 所有权**

**HFT：** 理解 GC **停顿 (stop-the-world)** 为何不适合 tick 线程；策略语言选型时考虑 **latency tail**。

→ [Ch 5 应用/GC 对比](../../chapter-05-optimizing-performance/) · [Ch 12 并发](../../chapter-12-concurrent-programming/)

---

← [本章导读](../README.md)
