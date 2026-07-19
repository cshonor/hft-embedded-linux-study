## 5.14 确认和消除性能瓶颈

> **Ch5 §5.14** · [章导读](../README.md) · 上节 [§5.13 ←](./section-5.13-现实生活性能提高技术.md) · 下节 [§5.15 →](./section-5.15-小结.md)

---

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
2. **区分** CPU bound vs memory bound vs I/O wait（→ [14-Systems-Performance](../../../15-Systems-Performance-2nd/)）
3. 改完对比 **同一 workload、同一硬件、同一编译 flags**
4. 避免 **微观基准误导** — 微基准只验证 CPE，端到端用 replay

**HFT 工作流：**

```
生产/trace replay → perf 火焰图 → 改热函数 → 回归 P99 延迟
回测与生产 binary flags 对齐；改完跑 regression + 压力测试
```

---

### 口述巩固 · 自测

1. （待口述补）本节核心一句话？

---

← [§5.13 ←](./section-5.13-现实生活性能提高技术.md) · [本章导读](../README.md) · [§5.15 →](./section-5.15-小结.md)
