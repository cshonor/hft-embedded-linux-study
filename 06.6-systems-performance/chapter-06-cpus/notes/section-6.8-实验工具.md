## 6.8 实验工具 · CPU 基准

> 章节导航：[6.6–6.7 观测工具与可视化](./section-6.6-6.7-观测工具与可视化.md) · 上一篇 ← · 下一篇 [6.9 CPU 调优](./section-6.9-CPU-调优.md) · [本章导读](../README.md)

**本节讲什么**：CPU 实验工具的阶梯（Ad Hoc 验证 → sysbench 粗比 → stress-ng 定向 → 真实负载压测）、每层的适用问题、实验纪律（观测工具先自证可信）。

### 要点

| # | 要点 | 一句话 |
|---|------|--------|
| 1 | **先验证观测工具，再信实验数据** | Ad Hoc 满载自证 |
| 2 | sysbench 测**纯算力** | HFT 瓶颈不在算质数 |
| 3 | stress-ng 是**定向压力源** | 可选 CPU/cache/内存/调度各维度 |
| 4 | 真正的 HFT 实验是**行情回放压测** | 微基准只是 sanity check |
| 5 | 实验时**常开 mpstat** | 确认并行度符合预期 |

---

### 一、Ad Hoc：验证观测工具（第 0 层）

最简单的 CPU 满载——**目的是验证观测工具，不是测性能**：

```bash
# 单线程 CPU-bound（"hot on one CPU"）
while :; do :; done &
taskset -c 2 bash -c 'while :; do :; done' &   # 指定核版本
# 用完 kill
```

**HFT 用法**：跑一个绑核 2 的死循环，验证 `mpstat -P ALL 1` 正确显示 CPU2 100% usr——**工具可信才能信生产数据**（[ch12 sanity check](../../chapter-12-benchmarking/notes/section-12.3-基准测试方法论.md) 的最小版本）。同时可验证：taskset 生效（没被调度器迁移）、%usr 与 %sys 的归属、pidstat 找得到进程。

### 二、SysBench：纯算力粗比（第 1 层）

```bash
sysbench --num-threads=8 --test=cpu --cpu-max-prime=100000 run
```

| 输出字段 | 含义 |
|----------|------|
| total time | 8 线程算完 100000 以内质数的总时间 |
| per-request min/avg/max | 单次事件延迟分布 |
| Threads fairness | 线程间负载均衡度（stddev 越小越好） |

**用途**：不同系统/CPU/内核版本/编译器选项间的**纯算力对比**——前提是测试二进制构建一致（同编译器同优化级，否则比的是编译器）。

### 三、stress-ng：定向压力源（第 1.5 层）

stress-ng 是 sysbench 的泛化版——**按维度定向施压**：

```bash
stress-ng --cpu 8 --cpu-method matrixprod --timeout 60s      # CPU 算力
stress-ng --cache 4 --timeout 60s                            # cache 压力
stress-ng --vm 4 --vm-bytes 1G --timeout 60s                 # 内存带宽
stress-ng --sched 8 --timeout 60s                             # 调度压力（频繁切换）
stress-ng --matrix 4 --matrix-size 128 --timeout 60s          # 矩阵（SIMD 路径）
```

| 场景 | stress-ng 模式 | 验证什么 |
|------|---------------|---------|
| 隔离核验证 | `--cpu 1 --taskset 2` | CPU3 上跑负载，CPU2 的 mpstat 应纹丝不动 |
| cache 噪声实验 | `--cache` 在邻居核 | 热核 IPC 掉多少（LLC 争用的对照实验） |
| NUMA 验证 | `--numa` | 远端访问的延迟差 |
| 调度器噪声 | `--sched` | 热核 runqlat 是否被邻居影响 |

**cache 噪声实验是 HFT 特色用法**：dedicated 核 A 跑策略，邻居核 B 跑 `stress-ng --cache`——A 的 IPC/延迟变化就是 LLC/内存带宽串扰的量化证据（物理隔离必要性的实验证明）。

### 四、微基准工具对比

| 工具 | 测什么 | HFT 定位 |
|------|--------|---------|
| Ad Hoc loop | 观测工具自证 | 第 0 步 |
| sysbench cpu | 纯算力（质数） | 粗比硬件/编译器 |
| stress-ng | 多维定向压力 | 隔离/串扰验证 |
| perf bench | 内核微操作（syscall/sched/mem） | 子系统开销 |
| `lmbench`/`stream` | 内存延迟/带宽 | 内存子系统 baseline |
| **真实回放压测** | tick-to-trade 端到端 | **真正的实验** |

### 五、HFT 真正的 CPU 实验

微基准全是 sanity check——**HFT 性能瓶颈不在算质数**，在：

```
行情回放压测（tick-to-trade 端到端）
  ├─ 负载：真实 tick replay（节奏保真，[ch12 replay](../../chapter-12-benchmarking/notes/section-12.2-基准测试的类型.md)）
  ├─ 观测常开：mpstat -P ALL / runqlat / perf stat / biolatency
  ├─ 判读：per-CPU 使用率 + 调度延迟 + IPC + 端到端 P99 分布
  └─ 对照：一次只改一个变量（ch12 方法论）
```

**操作建议**：跑实验时**始终开 `mpstat -P ALL 1`**——确认 CPU 使用率和并行度符合预期（8 线程却只用了 4 核？绑核错了。热核 %soft 异常？中断没赶走）。

### 衔接

- 上一节：[6.6–6.7 观测工具与可视化](./section-6.6-6.7-观测工具与可视化.md)
- 下一节：[6.9 CPU 调优](./section-6.9-CPU-调优.md)（实验结论的落地）
- 关联：[Ch1.8 微观 vs 宏观](../../chapter-01-intro/notes/section-1.8-实验与微观宏观基准.md)、[Ch12 基准测试](../../chapter-12-benchmarking/)、[14-HFT ch09 延迟测量](../../../14-hft-engineering/chapter-09-latency-measurement-benchmarking/README.md)

---

### 常见陷阱

1. **sysbench 分数当 HFT 性能**——质数算力与 tick 处理路径无关；微基准只做 sanity/粗比。
2. **实验不开观测**——跑完只有总时长，没有 per-CPU/调度/IPC 证据，等于白跑。
3. **stress-ng 参数不对齐目标**——`--cpu` 默认方法是循环运算，测 cache 串扰要用 `--cache` 显式指定。
4. **比对不同编译器构建的二进制**——比的是编译器不是 CPU。

<details>
<summary>自测题（点击展开）</summary>

1. Ad Hoc 死循环的真正用途？
   <details><summary>答</summary>验证观测工具可信度（mpstat/pidstat 显示正确）+ 验证 taskset 生效——先确保工具对，再信生产数据。</details>
2. stress-ng --cache 在 HFT 中的实验用法？
   <details><summary>答</summary>邻居核跑 cache 压力，观测热核 IPC/LLC miss 变化——量化 LLC/带宽串扰，证明物理隔离必要性。</details>
3. 为什么 sysbench 不适合评价 HFT 系统？
   <details><summary>答</summary>质数算力是纯 CPU 密集单循环——HFT 瓶颈在 cache/分支/调度/内存访问模式，真实实验是 tick-to-trade 回放压测。</details>
4. 实验时为什么常开 mpstat -P ALL？
   <details><summary>答</summary>确认并行度符合预期（绑核是否生效、热核是否被软中断污染）——没有它，实验只是黑盒计时。</details>
5. perf bench 和 sysbench 的区别？
   <details><summary>答</summary>perf bench 测内核子系统的微操作开销（syscall/sched/mem 子命令）；sysbench 是应用级算力负载。</details>

</details>


---

← [6.7 可视化](./section-6.6-6.7-观测工具与可视化.md) · [6.9 调优](./section-6.9-CPU-调优.md) · [本章导读](../README.md)
