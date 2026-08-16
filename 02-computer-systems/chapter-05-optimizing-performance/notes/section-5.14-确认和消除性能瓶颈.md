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
2. **区分** CPU bound vs memory bound vs I/O wait（→ [06.6-Systems-Performance](../../../06.6-systems-performance/)）
3. 改完对比 **同一 workload、同一硬件、同一编译 flags**
4. 避免 **微观基准误导** — 微基准只验证 CPE，端到端用 replay

**HFT 工作流：**

```
生产/trace replay → perf 火焰图 → 改热函数 → 回归 P99 延迟
回测与生产 binary flags 对齐；改完跑 regression + 压力测试
```

---

### 常见陷阱

1. **微基准误导** — 微基准只测单个函数的 CPE，可能跟端到端表现不一致。一个函数 CPE 降了 2× 但整体 P99 没改善——说明瓶颈在别处。用 trace replay 做端到端验证。
2. **profile 采样率太低** — `perf record` 默认采样率可能漏掉短热路径。HFT 纳秒级路径需要高采样率（`-F 9999`）或用 `perf c2c`/`perf mem` 精确定位。
3. **改了代码但 binary flags 不一致** — 回测用 `-O2`，生产用 `-O3`，优化效果不同。确保 benchmark 和生产用**完全相同的编译 flags、CPU 型号、OS 配置**。

### 自测题

<details>
<summary>1. perf 的基本工作流是什么？</summary>

①`perf record -g ./program` 采样（`-g` 记录调用栈）→ ②`perf report` 看热点函数 → ③`perf annotate -s hot_function` 看热点汇编 → ④改代码 → ⑤重新 benchmark 对比。HFT 还需要看 `branch-misses`、`cache-misses`、`stalled-cycles` 等硬件计数器。
</details>

<details>
<summary>2. 如何区分 CPU bound vs memory bound？</summary>

看 `perf stat` 的硬件计数器：**CPU bound** → `IPC > 1`、`stalled-cycles-backend` 低、`cache-misses` 低；**memory bound** → `IPC < 1`、`stalled-cycles-backend` 高、`cache-misses`/`L1-dcache-load-misses` 高。也可以看 `perf mem` 或 `perf c2c` 精确定位内存访问问题。
</details>

<details>
<summary>3. 为什么微基准好但端到端没提升？</summary>

可能原因：①微基准的 workload 不代表真实场景（数据分布、分支模式不同）；②优化的函数不是端到端瓶颈（阿姆达尔定律）；③优化引入了其他开销（如代码膨胀导致 icache miss）；④binary flags 不一致。**必须用 trace replay 做端到端 P99 回归验证**。
</details>

---

← [§5.13 ←](./section-5.13-现实生活性能提高技术.md) · [本章导读](../README.md) · [§5.15 →](./section-5.15-小结.md)
