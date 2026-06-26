## 5.13–5.15 实战技术与程序剖析

### 5.13 现实生活：性能提高技术

| 层级 | 手段 |
|------|------|
| 算法 | 更低复杂度、更好数据结构 |
| 源码 | 本章 5.4–5.12 |
| 编译 | `-O3`、`-march=native`、PGO、LTO |
| 并行 | 多线程、绑核、无锁（→ [Ch 12](../../chapter-12-concurrent-programming/)） |
| 系统 | hugepage、NUMA、隔离核（→ [12-HFT](../../../15-HFT-Low-Latency-Practice/)） |

**顺序：** 先正确 + profile，再小步改；每次改 **测一遍**。

### 5.14 确认和消除性能瓶颈

#### 5.14.1 程序剖析 (Profiling)

| 工具 | 用途 |
|------|------|
| **`gprof`** | 经典采样/插桩（课程作业） |
| **`perf record/report`** | 生产级，CPU、cache、分支 |
| **`perf annotate`** | 热点汇编 |
| **编译器报告** | `-fopt-info`、LLVM remarks |

```bash
perf record -g ./strategy --args
perf report
perf annotate -s hot_function
```

#### 5.14.2 用剖析指导优化

1. **找占时间 >5–10% 的函数** — 阿姆达尔
2. **区分** CPU bound vs memory bound vs I/O wait（→ [03-SysPerf](../../../03-Systems-Performance-2nd/)）
3. 改完对比 **同一 workload、同一硬件、同一编译 flags**
4. 避免 **微观基准误导** — 微基准只验证 CPE，端到端用 replay

**HFT 工作流：**

```
生产/trace replay → perf 火焰图 → 改热函数 → 回归 P99 延迟
回测与生产 binary flags 对齐；改完跑 regression + 压力测试
```

### 5.15 小结（原书）

- 编译器强大但需 **合作式 C 代码**
- **测量驱动** — CPE、profile、annotate
- 内存与分支常是隐形天花板 — 与 Ch4、Ch6 联动

→ 下一章专攻 **cache 与局部性**：[Ch 6](../../chapter-06-memory-hierarchy/)

---

← [本章导读](../README.md)
