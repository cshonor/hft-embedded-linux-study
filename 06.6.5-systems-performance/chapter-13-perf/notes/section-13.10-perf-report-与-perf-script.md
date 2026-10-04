# 13.10 `perf report` 与 `perf script`

> [章节导航](../README.md) · 上一节：[13.9 perf record](./section-13.9-perf-record-剖析采样.md) · 下一节：[13.11 perf trace](./section-13.11-perf-trace-系统调用追踪.md)

## 本节讲什么

`perf.data` 的两种读法：

- **report**：聚合视图（热点表 + 调用图）——回答"哪热"
- **script**：逐样本流水（每行一个样本）——回答"何时、什么序列"，且是**火焰图的标准输入**

外加 self/children 计分原理、火焰图管道、以及 on-CPU 图的根本盲区。

---

## 1. `perf report` — 聚合视图

```bash
perf report --stdio --no-children | head -50       # 纯文本热点表
perf report --sort comm,dso,symbol                  # 按进程/库/符号三维
perf report -g graph,0.5,caller                     # 调用图（caller 视角）
perf report -g fractal,0.5,callee                   # callee 视角（火焰图式）
```

### 1.1 ⭐ Overhead 的分母与 self/children

样本里每帧都被记一次账。两种计法：

| 计法 | 含义 | 用途 |
|---|---|---|
| **Self（`--no-children`）** | 只记**栈顶帧**的样本 | 找"真正的热点函数"——CPU 周期烧在哪行 |
| **Children** | 栈里**每一帧都记**一次 | 找"调用链热点"——哪条路径整体在烧 |

```
main() ──> dispatch() ──> decode() ──> 解码热点
```

100 个样本都长这样：decode 的 self=100；main/dispatch/decode 的 children 都是 100（每帧全记）。

**判读纪律**：

1. **先 self 后 children**：self 表定位叶子热点（改哪里）；children 表看传播路径（谁把流量引来的）
2. children 高 self 低 ≠ 无罪——`dispatch()` 可能是**分错账的入口**（它调了三个函数，热度被摊薄），用 `-g` 展开看分叉
3. **`--no-children` 是严肃分析姿势**——默认 TUI 的 children 视图容易把注意力引向 main 这种"人人经过"的帧

### 1.2 常用 sort 维度

| 维度 | 回答 |
|---|---|
| `symbol` | 哪个函数 |
| `dso` | 哪个 .so / 内核模块（**第三方库拖后腿证据**） |
| `comm` | 哪个进程/线程 |
| `cpu` | 哪个核（热核 vs 均衡） |
| `srcline`（需 debuginfo） | 哪一行 |
| `parent` | 谁调的 |

## 2. `perf script` — 逐样本流水

```bash
perf script > out.perf        # 每样本一块：线程/CPU/时间戳/符号栈
perf script -F comm,pid,tid,cpu,time,ip,sym    # 定制字段
```

输出形态（火焰图管道的原料）：

```
strategy  4242/4242  [003] 40231.812345:  1234 cycles:
        fffffe00+00a0b0 [strategy] decode_tick
        fffffe00+0091c2 [strategy] dispatch
        ...
```

**定位差异**：report 是"聚合后的结论"，script 是"原始证据"——看**时间分布**（热点是均匀烧还是爆发式）、**单样本异常**（一个 2ms 长栈在 99Hz 下稀有几个样本）、**与其他日志对时间轴**时必须用 script。

## 3. 火焰图管道

```bash
git clone https://github.com/brendangregg/FlameGraph
export PATH=$PATH:/path/to/FlameGraph

perf record -F 99 -g -p $(pidof strategy) -- sleep 60
perf script | stackcollapse-perf.pl | flamegraph.pl --title="strategy CPU" > strategy.svg
```

三级流水各做什么：

| 级 | 做什么 |
|---|---|
| `perf script` | 吐逐样本栈 |
| `stackcollapse-perf.pl` | 每个样本压成 `main;dispatch;decode 123` 的单行（分号链 + 计数） |
| `flamegraph.pl` | 聚合同链计数 → 画宽度∝样本数的矩形 |

**读图规则**（对照 [Ch 2 方法论](../../chapter-02-methodologies/)）：

| 规则 | 原因 |
|---|---|
| **看宽度不看高度** | 高度=栈深（无意义），宽度=样本占比 |
| 横向顺序无关 | 按字母排——**不代表执行顺序** |
| 平顶宽 = 叶子热点 | 直接优化目标 |
| 热但很浅的塔 | 调用链级问题（间接开销/分派混乱） |
| 变体 | `flamegraph.pl --invert`（icicle）、`--colors` 按模块着色 |

**off-CPU 火焰图**：perf record 默认只在**线程 on-CPU** 时采样——等锁/IO/调度的时间在图上**完全不可见**（这类时间根本不产生样本）。补法：BPF `offcputime`（[Ch 15](../../chapter-15-bpf/)）采"唤醒→上CPU"间隔的栈——**CPU + Off-CPU 两张图缺一不可**，只看 CPU 图会把"等"误判成"没事"。

## 4. 一个 HFT 完整例：tick 处理 P99 恶化

```bash
# ① 计数先行（S2）
perf stat -e cycles,instructions,LLC-load-misses,context-switches -p $(pidof strat) -I 1000 -- sleep 60
#    → IPC 正常但 context-switches 每 10s 一个尖 → 调度问题，CPU 图白采

# ② on-CPU 长采（S3）
perf record -F 499 -g -p $(pidof strat) -C 3 -- sleep 120
perf script | stackcollapse-perf.pl | flamegraph.pl > strat-cpu.svg
#    → CPU 图无异常热点 → 排除计算慢

# ③ off-CPU 补刀（转 BPF）
offcputime -p $(pidof strat) 60 > off.svg
#    → 5ms 尖刺=epoll_wait 唤醒后等 CPU → runqlat 迁核审计
```

这个例子的教训在 [ch16.1.7 动态追踪](../../chapter-16-case-studies/notes/section-16.1.7-16.1.8-动态追踪与结论.md)：**工具会撒谎——不是数据假，是覆盖面有洞**。on-CPU 图"干净"只是证明"CPU 上没问题"。

---

## HFT / 嵌入式关联

| 场景 | 用法 |
|---|---|
| 调优验收 | 前后各留 svg + `--sort dso` 表——LLC miss 类优化看 dso 维度（数据结构所在库） |
| 第三方库审计 | `--sort dso`：行情解码库/内核协议栈各占多少，一键分账 |
| 长尾样本考古 | `perf script` 后按时间过滤稀有长栈样本——99Hz 下 P99 尖刺每秒约 1 个样本，60s 采样约 60 个证据点 |
| 容器/多机归档 | perf.data + svg 进 release 工单，[ch16 可回放纪律](../../chapter-16-case-studies/notes/section-16.0-案例背景An-Unexplained-Win.md) |
| Pi5 | 火焰图管道全流程可用——[06.7 eBPF](../../../06.7-bpf-observability/) 实验线的标配输出 |

---

## 衔接

- 上一节：[record 采样机制](./section-13.9-perf-record-剖析采样.md)
- 下一节：[13.11 perf trace——syscall 视角](./section-13.11-perf-trace-系统调用追踪.md)
- off-CPU 补齐：[Ch 15 BPF](../../chapter-15-bpf/)
- 方法论：[Ch 2 USE/Drill-Down](../../chapter-02-methodologies/) · [ch16 演练](../../chapter-16-case-studies/notes/section-16.9-HFT-版Unexplained-Win演练模板.md)

---

## 代码自测

<details><summary>Q1：Self 和 Children 列分别怎么记账？各自用来回答什么问题？</summary>

Self 只把样本记给栈顶帧（真热点在哪行）；Children 给栈里每帧都记（哪条路径在烧）。分析顺序：先 `--no-children` 找叶子热点，再看 children 理解流量从哪来。children 高 self 低的帧是"路径"不是"病灶"。
</details>

<details><summary>Q2：火焰图为什么"看宽度不看高度"？塔的横向顺序有意义吗？</summary>

宽度∝样本占比（时间份额），高度只是调用深度。横向按字典序排列、可任意重排——不代表执行顺序或时间顺序。想看顺序要回 perf script 流水。
</details>

<details><summary>Q3：CPU 火焰图上完全看不到锁等待，为什么？</summary>

record 只在线程 on-CPU 时由 PMC 溢出触发采样；等锁/IO/调度的线程不产生任何样本——这段时间在图上是空白而非显式条目。要显式看"等"的时间需 off-CPU 采样（BPF offcputime 用唤醒/切换事件对栈）。
</details>

<details><summary>Q4：report 和 script 各自的不可替代场景？</summary>

report：聚合结论（热点表、dso 分账）。script：时间维度证据——热点的时间分布（均匀 vs 爆发）、稀有长栈样本、与外部日志对时间轴。火焰图丢失时间信息，script 保留全部。
</details>

<details><summary>Q5：<code>--sort dso</code> 在第三方库审计里怎么用？</summary>

按动态库/内核模块聚合样本：一行一个 .so。行情解码库占比 40% 而自家策略 10%——优化方向立判；也用于验证"换版 libfoo 后开销占比下降"这类跨版本对比。
</details>
