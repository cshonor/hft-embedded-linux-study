## 4.5 四大追踪器

> 本章导读 · [4.1 危机工具](./section-4.1-工具覆盖范围与危机工具.md) · [4.4 sar 工具](./section-4.4-sar-工具.md) · [4.6 观测的观测](./section-4.6-观测的观测Observing-Observability.md)

---

### 本节讲什么

把「追踪」（tracing）从「统计」（counting）里分离出来讲：**统计告诉你发生了多少，追踪告诉你每一次是怎么发生的**。本节梳理现代 Linux 四大追踪器的分工谱系、它们与数据源的关系、开销量级对比，以及 HFT 语境下的选型决策树。

### 要点

| # | 要点 | 一句话展开 |
|---|------|-----------|
| 1 | 统计 vs 追踪是两个世界 | iostat 给速率，biosnoop 给每一个 I/O 的完整故事 |
| 2 | 四大追踪器各有辖区 | perf 剖析、Ftrace 内核内建、BCC 生产工具集、bpftrace 即兴查询 |
| 3 | 数据源是共同的，前端正交 | perf/Ftrace/eBPF 都消费 tracepoint/kprobe/uprobe/USDT |
| 4 | 开销从「可忽略」到「毁灭性」排开 | Ftrace 函数级 > BPF 聚合 > perf 采样 >（更差的）strace |
| 5 | strace 不在四大之列但是必修反面教材 | ptrace 双陷阱让热路径慢 10-100× |
| 6 | 生产环境的选择顺序 | 有预制工具用预制 → bpftrace 单行 → 才轮到自写 BCC |

---

### 一、先分清：统计与追踪

| 维度 | 统计（counting） | 追踪（tracing） |
|------|-----------------|-----------------|
| 例子 | `iostat -x`、`vmstat`、`mpstat` | `biosnoop`、`perf trace`、`bpftrace` |
| 输出 | 聚合值（速率、占比、均值） | 事件流（每次调用的参数/时长/时序） |
| 开销 | 极低（读 counter） | 与事件数成正比 |
| 回答 | 「每秒多少次」「平均多长」 | **「最长的那一次是谁、卡在哪一步」** |
| HFT 视角 | 常驻可用 | 按需开窗，事件级诊断 |

一条铁律：**均值正常 + P99 异常的问题，只能靠追踪查**。统计把事件聚合掉的瞬间，尾部信息就消失了（见 [2.5 方法论](../../chapter-02-methodologies/notes/section-2.5-性能分析方法论.md) 的 histogram 论述）。

---

### 二、四大追踪器分工

| 工具 | 定位 | 擅长 | 详见 |
|------|------|------|------|
| **perf** | 官方剖析器 | CPU 采样、PMC、部分 trace、火焰图 | [Ch 13](../../chapter-13-perf/) |
| **Ftrace** | 内核内置 | 内核函数路径、调度、irq、latency histogram | [Ch 14](../../chapter-14-ftrace/) |
| **BCC** | eBPF + Python/Lua 前端 | 复杂脚本、生产级工具集（biolatency…） | [Ch 15](../../chapter-15-bpf/) |
| **bpftrace** | eBPF 单行 DSL | ad hoc 查询、一行命令、教程友好 | [Ch 15](../../chapter-15-bpf/) |

**关系：数据源共享，前端正交**

```
        ┌─────────── 数据源 ───────────┐
        │ /proc  PMC  tracepoint     │
        │ kprobe  uprobe  USDT       │
        └─────────────┬──────────────┘
                      │
     ┌────────────────┼────────────────┐
     ▼                ▼                ▼
   perf            Ftrace          eBPF 引擎
     │                │                │
     │                │         ┌──────┴──────┐
     │                │         ▼             ▼
     └────────────────┴────  BCC        bpftrace
```

关键点：**四大追踪器不是四种数据源，而是同一个数据源集合的四种消费方式**。选哪个工具，本质是选「消费方式」——要采样选 perf、要内核内建配置选 Ftrace、要可编程聚合选 eBPF 系。数据源本身的属性（tracepoint 稳定 vs kprobe 不稳）在 [4.3 核心数据源](./section-4.3-核心观测数据源.md) 已讲，与前端无关。

---

### 三、开销量级：决定生产可用性的排序

追踪器之间的核心差异之一是**每个事件的成本**：

| 追踪方式 | 每事件开销量级 | 机制 | 生产可用性 |
|----------|---------------|------|-----------|
| perf 采样（-F 99） | 与事件数无关（固定采样率） | PMI/定时器采样 | 常年可开 |
| **BPF 内核态聚合** | ~1µs 级，且**不向用户态逐事件发数据** | map 聚合，读直方图 | 高（前提是 filter 收窄） |
| Ftrace function 跟踪 | 数百 ns~µs/次，**逐事件写 ring buffer** | 内核内建 | 短窗口 |
| perf trace / bpftrace 逐事件打印 | 事件越多越贵，**用户态格式化是瓶颈** | 事件→用户态 | 限时窗口 |
| strace（ptrace） | **每 syscall 两次进程陷入，最重** | ptrace 停-继续模型 | **生产禁用** |

BPF 的「内核聚合」是本表的关键分水岭：bpftrace 的 `@usecs = hist(nsecs)` 在内核里把一百万次事件聚成一个直方图再交给用户态——**高频事件不打爆用户态**正是它能进生产的原因（详见 [15.2 bpftrace](../../chapter-15-bpf/)）。

**strace 为什么是反面教材**：ptrace 模型下每个 syscall 前后都要把被追踪进程停下来、唤醒 tracer、再放行——两次上下文切换外加两次 ptrace 系统调用本身。热路径上等于每次 syscall 乘一个常数因子，吞吐型负载可观测到 10-100× 减速。等价的正确姿势：

```bash
# 错：strace -p <PID>
# 对（低开销替代，仍有开销但低一个量级以上）：
perf trace -p <PID>          # 单点旁路，不 ptrace 停进程
bpftrace -e 'tracepoint:raw_syscalls:sys_enter @[comm] = count();'
```

---

### 四、选型决策树

```
要回答什么问题？
│
├─ CPU 时间花在哪（剖析）
│    └─> perf record -F 99 -g → 火焰图          [Ch 13]
│
├─ 某个内核路径内部发生了什么（时序/路径）
│    └─> Ftrace（function_graph / 事件 trace）   [Ch 14]
│
├─ 事件级统计（延迟直方图 / 按进程聚合）
│    ├─> 有现成 BCC 工具？── 是 ──> biolatency/runqlat/tcpretrans
│    └─> 没有 ──> bpftrace 单行（先试）
│                  └─ 复杂逻辑/需要发布 ──> 自写 BCC / libbpf
│
└─ 只想看某进程干了哪些 syscall（盘点/审计）
     └─> bpftrace sys_enter 计数 或 syscount（绝不 strace）
```

**HFT 实践路径**（按上手顺序）：

1. **perf** — 火焰图、cache miss（[Ch 13](../../chapter-13-perf/)）
2. **bpftrace** — syscall 计数、run queue 延迟、网络栈 tracepoint（[Ch 15](../../chapter-15-bpf/) + [附录 C](../../appendix-C-bpftrace单行命令.md)）
3. **Ftrace** — 内核延迟 odd case（[Ch 14](../../chapter-14-ftrace/)）
4. **BCC** — 现成工具不够时再写 Python BPF（[15.1](../../chapter-15-bpf/)）

即：**先采样后追踪、先预制后自写、先聚合后逐事件**。每一步都比下一步便宜，能用便宜的回答就不动用贵的。

---

### 五、strace 的合法用途

翻案一下：strace 并非一无是处，它的 ptrace 模型换来的能力是**进程级隔离视角**（只追一个进程、能看到完整 syscall 参数解码）：

| 场景 | 判断 |
|------|------|
| 调试启动失败（配置文件找不着、权限拒绝） | ✅ 合法——进程还没进热路径，开销无所谓 |
| 一次性短命令的行为审计 | ✅ 合适 |
| 生产热路径进程 | ❌ 绝对禁止 |
| 「这个程序打开了哪些文件」 | 更好的答案：`bpftrace opensnoop.bt` 或 `perf trace -e open*` |

经验法则：**strace 只碰「还没开工」或「即将退出」的进程，永远不碰正在跑热路径的进程**。更进一步——syscall 资产盘点的正确姿势是 [5.5 观测工具](../../chapter-05-applications/notes/section-5.5-观测工具.md) 里讲的 syscount / `bpftrace sys_enter` 计数法。

---

### HFT / 嵌入式关联

- **热路径机器的追踪纪律**：追踪器全部有开销（[4.6 观测的观测](./section-4.6-观测的观测Observing-Observability.md)）；HFT 的规矩是「常驻只用统计（sar + 超时计数器），追踪开窗需授权且限时长」。
- **四大追踪器在 tick 路径排查中的分工**：perf 剖析答「CPU 花哪」、bpftrace 答「事件延迟分布」、Ftrace 答「内核内这条路怎么走的」、BCC 工具集答「标准问题用标准答案」——四者互查可以交叉验证，正合 4.6 的「多来源对照」原则。
- **嵌入式**：perf 与 Ftrace 随内核提供（开 CONFIG 即可），BCC 交叉编译困难（见 [4.1](./section-4.1-工具覆盖范围与危机工具.md)），嵌入式板卡上实际可用的是 perf（简化版）+ Ftrace + libbpf 预编译。

---

### 衔接

- 上一节 [4.4 sar](./section-4.4-sar-工具.md)：统计侧的王者；本节是事件侧。
- 深入各工具：[Ch 13 perf](../../chapter-13-perf/) · [Ch 14 Ftrace](../../chapter-14-ftrace/) · [Ch 15 BPF](../../chapter-15-bpf/)（ch13-15 三章正是对四大追踪器的逐一深挖）。
- strace 机制细节与 syscount 用法：[5.5 应用观测工具](../../chapter-05-applications/notes/section-5.5-观测工具.md)。

---

### 常见陷阱

1. strace 生产直接跑——strace 开销巨大（每个 syscall 两次 ptrace），生产禁用或限时
2. perf trace 当 strace 用——perf trace 开销比 strace 低但仍有开销，生产限时长
3. ftrace 和 BPF 不分场景——ftrace 适合内核内建追踪，BPF 适合可编程聚合
4. 逐事件打印上高频路径——bpftrace 逐事件 print 在百万级事件/秒的路径上本身成为瓶颈；用 map 聚合（hist/count）
5. 「追踪比统计高级」的错觉——两者回答不同问题；P99 异常但均值正常时才轮到追踪，日常容量问题统计就够

<details>
<summary>自测题（点击展开）</summary>

1. 四大追踪器分别是什么？
   <details><summary>答</summary>perf（官方剖析器：采样/PMC/火焰图）、Ftrace（内核内建：函数路径/调度/irq）、BCC（eBPF+Python 前端：生产级工具集）、bpftrace（eBPF 单行 DSL：即兴查询）——strace/perf trace 是入门级追踪但不在「四大」之列</details>
2. 为什么 strace 不能在生产环境用？
   <details><summary>答</summary>每个 syscall 两次 ptrace 陷入（进程停-继续-再放行），热路径会变成原来的 10-100 倍慢；合法用途只剩调试启动失败/一次性短命令审计</details>
3. ftrace 和 BPF 各自适合什么场景？
   <details><summary>答</summary>ftrace 适合内核内建 tracepoint/函数追踪（要路径时序），BPF 适合可编程聚合（直方图/过滤/计算）；两者共享数据源，选的是消费方式</details>
4. 四大追踪器为什么说是「同一数据源的四种消费方式」？
   <details><summary>答</summary>perf/Ftrace/eBPF 都消费 tracepoint/kprobe/uprobe/USDT/PMC——数据源属性（如 tracepoint 稳定、kprobe 不稳）在前端之间通用，选工具选的是采样、路径记录还是可编程聚合</details>
5. BPF 内核态聚合为什么能进生产？
   <details><summary>答</summary>事件在内核里聚合成直方图/计数，不向用户态逐事件发数据——百万事件/秒的路径上开销仍是 µs 级/事件，用户态零压力；这是与逐事件追踪（perf trace/打印型）的本质分水岭</details>
6. HFT 的追踪使用纪律是什么？
   <details><summary>答</summary>常驻只用统计（sar + 超时计数器）；追踪按「先采样后追踪、先预制后自写、先聚合后逐事件」的顺序选；热路径机器上开窗需授权且限时长</details>

</details>


---

← [本章导读](../README.md)
