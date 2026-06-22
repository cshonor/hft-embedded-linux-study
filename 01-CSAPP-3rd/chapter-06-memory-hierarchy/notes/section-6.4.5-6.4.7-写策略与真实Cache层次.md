## 6.4.5–6.4.7 写策略与真实 Cache 层次

### 6.4.5 有关写的问题

| 策略 | 行为 |
|------|------|
| **写直达 (write-through)** | 写同时更新 cache 与下层 — 简单，总线忙 |
| **写回 (write-back)** | 只写 cache，**dirty** 位；替换时才写回 — 常用 |
| **写分配 (write-allocate)** | miss 时先 load line 再写 — 利用局部性 |
| **非写分配** | miss 直接写下层 — 少用于 L1 |

- **store 引发 miss** — 可能触发 load line（与 Ch5 load 性能联动）

### 6.4.6 真实 Cache 层次解剖（Intel 类）

典型桌面/服务器：

```
L1i / L1d  32KB, 8-way, 64B line, ~4 cycles
L2         256KB–1MB per core
L3 LLC     共享，数十 MB
```

- ** inclusive vs exclusive** LLC — 多核一致性协议（MESI）在 LLC 层可见
- **预取器** — 硬件 stride prefetch

### 6.4.7 Cache 参数的性能影响

| 参数 | 增大时 |
|------|--------|
| **B (块大小)** | 空间局部性好则降 miss；太大则 conflict↑、填充慢 |
| **S/E (容量/相联)** | 降 capacity/conflict miss；延迟与功耗↑ |
| **关联度** | 降 conflict，比较器成本↑ |

**HFT / 多线程：**

- **False sharing** — 两核写 **不同变量** 但在 **同一 cache line** → MESI 行乒乓  
  **fix：** `alignas(64)` 隔离、`std::hardware_destructive_interference_size`（C++17）
- `perf c2c`（若可用）或 PMC 看 **HITM** 事件

→ 并发：[Ch 12](../../chapter-12-concurrent-programming/) · [02-SysPerf Ch 7 内存](../../../02-Systems-Performance-2nd/chapter-07-memory/)

---

← [本章导读](../README.md)
