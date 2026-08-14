# 4. Low-Level Memory Accesses（底层内存访问）

> [← 章索引](./README.md)

外设 **MMIO** 寄存器可在两次读之间被硬件改变 — 编译器 **CSE / 省略** 会得到错误观测。

## Volatile

```rust
core::ptr::read_volatile(addr);
core::ptr::write_volatile(addr, val);
```

约束编译器对该地址读写的优化语义。

## 注意

**Volatile ≠ 完整内存排序模型** — 多核 / 设备强顺序需求结合 **ISA 手册** 与 **`atomic::*`**。

→ [第 10 章 · 原子](../Chapter-10-Concurrency-and-Parallelism/07-memory-operations.md) · Nomicon
