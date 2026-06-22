## 1.5 高速缓存至关重要

### 为何 cache 是性能第一公民

- CPU 与主存速度差距 **持续拉大**（「内存墙」）
- **缓存 (cache)** — 由 SRAM 构成的小而快的存储，自动保存主存中 **最近用过** 的数据副本
- 典型三级：**L1（指令/数据）→ L2 → L3（LLC）→ 主存**

| 命中 | 含义 | 延迟量级（直觉） |
|------|------|------------------|
| L1 hit | 数据已在 L1 | ~1 ns 级 |
| L2/L3 hit | 逐级更慢 | 数 ns ~ 十数 ns |
| **cache miss** | 需访问 DRAM | ~50–100+ ns |
| 跨 NUMA | 远程节点 DRAM | 更慢 + 不一致风险 |

### 局部性（locality）— 程序行为决定 miss 率

1. **时间局部性** — 刚访问过的，很可能很快再访问（循环变量、热结构体）
2. **空间局部性** — 刚访问地址附近，很可能接着访问（数组顺序扫描）

**编译器/程序员能做的：** 数据结构布局、SoA vs AoS、对齐、避免冷路径污染 cache line。

**HFT 高频场景：**

- **Order book / ring buffer** 顺序访问 → 空间局部性好
- **指针 chasing、链表跳来跳去** → miss 多，P99 抖
- **伪共享 (false sharing)** — 两线程改同一 cache line 不同字段 → 行乒乓（→ [Ch 6](../../chapter-06-memory-hierarchy/)、[Ch 12](../../chapter-12-concurrent-programming/)）
- **perf `cache-misses` / `perf c2c`** — 生产验证（→ [02-SysPerf Ch 13](../../../02-Systems-Performance-2nd/chapter-13-perf/)）

### 缓存按行 (cache line) 管理

- 常见 **64 字节一行** — 读 1 字节可能整行载入
- **预取 (prefetch)** 硬件/软件可隐藏部分延迟

→ 全书深入：[Ch 6 存储器层次结构](../../chapter-06-memory-hierarchy/)

---

← [本章导读](../README.md)
