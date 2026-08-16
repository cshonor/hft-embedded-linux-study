## 13.3–13.7 perf 事件源

### 硬件事件（Hardware Events / PMCs）

来自 CPU **性能监控计数器** — Ch 6 周期分析基础。

| 事件（示例） | 含义 |
|--------------|------|
| `cycles` | CPU 周期 |
| `instructions` |  retired 指令 → **IPC** |
| `cache-references` / `cache-misses` | cache 行为 |
| `L1-dcache-load-misses` | L1 数据 miss |
| `LLC-load-misses` | 末级 cache miss |
| `branch-misses` | 分支预测失败 |
| `stalled-cycles-frontend/backend` | 流水线停滞 |

```bash
perf stat -e cycles,instructions,cache-misses,LLC-load-misses ./strategy
```

**频率采样（Frequency Sampling）：**

- `perf record -F 99` — 约每秒 99 次样本（**非** 固定每 N 周期）。
- 优点：样本量随 CPU 活动自适应；**99 Hz** 减与 OS timer 拍频共振。
- 对比：`perf record -c 1000000 -e cycles` — 每 100 万周期采一次（event-based）。

**HFT：** 优化 order book 前后各跑 **`perf stat` IPC + LLC-misses** — 比凭感觉改结构可靠。

### 软件事件（Software Events）

内核维护的计数 — 无需特定 PMC。

| 事件 | 含义 |
|------|------|
| `page-faults` / `minor-faults` / `major-faults` | 缺页 |
| `context-switches` | 上下文切换 |
| `cpu-migrations` | 线程迁核 |
| `emulation-faults` | 等 |

```bash
perf stat -e page-faults,context-switches,cpu-migrations -p $(pidof strategy) -- sleep 10
```

→ Ch 7 [缺页火焰图](../../chapter-07-memory/)

### 追踪点事件（Tracepoint Events）

内核 **静态** 观测点 — 稳定 ABI。

| 类 | 例子 |
|----|------|
| syscalls | `syscalls:sys_enter_read` |
| sched | `sched:sched_switch` |
| block | `block:block_rq_issue/complete` |
| kmem | `kmem:kmalloc` |

```bash
perf list 'syscalls:*' | head
perf record -e 'syscalls:sys_enter_write' -a -- sleep 5
```

**与 Ftrace：** tracepoint 是 Ftrace 子集；perf 可 **采样或计数** tracepoint — Ch 14 更偏 Ftrace 专精。

### 探针事件（Probe Events）

| 类型 | 作用 | 稳定性 |
|------|------|--------|
| **kprobes** | 内核任意函数动态插桩 | 内核版本变可能断 |
| **uprobes** | 用户态函数插桩 | 需符号 |
| **USDT** | 用户静态探针（如 libc、MySQL） | 应用需编译支持 |

```bash
# 创建 uprobe（示例）
perf probe -x /path/strategy 'decode_entry'
perf record -e probe_strategy:decode_entry -p PID -- sleep 10
```

**HFT：** 热路径函数 uprobe **有开销** — 开发/短采；生产优先 **99Hz 采样** 或 USDT span。

→ Ch 15 [BPF 可编程探针](../../chapter-15-bpf/)

---


### 常见陷阱

1. 硬件事件当软件事件用——PMC 数量有限（通常 4-8 个），同时监测的事件有上限
2. kprobe 不理解稳定性——kprobe 追踪内核函数名可能随版本变更，应优先 tracepoint
3. uprobe 不测开销——uprobe 每次函数调用都陷入，热路径高频函数显著增延迟

<details>
<summary>自测题（点击展开）</summary>

1. perf 事件源的四大类是什么？
   <details><summary>答</summary>硬件事件（PMC）、软件事件（page-fault/cs）、tracepoint（稳定 ABI）、probe（kprobe/uprobe）</details>
2. kprobe 和 tracepoint 的稳定性区别？
   <details><summary>答</summary>tracepoint 是稳定 ABI 不会变；kprobe 追踪的内核函数名可能随版本变更而失效</details>
3. uprobe 在热路径的风险？
   <details><summary>答</summary>每次函数调用都陷入 BPF 处理——高频函数显著增延迟，应限时长或用 USDT 替代</details>

</details>


---

← [本章导读](../README.md)
