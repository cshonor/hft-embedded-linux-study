# Ch 13 perf 性能分析 · perf

> **Systems Performance 2nd** · Brendan Gregg · **精读**

> 本章定位：**Linux 官方标准剖析器 `perf(1)` 全书参考** — 贯穿 Ch 5–10 的 CPU/内存/磁盘/网络分析。Ch 4 选了 perf；本章讲 **事件源、子命令、栈回溯、火焰图流水线**。Ch 15 BPF 可编程更强；**perf 仍是 HFT 裸机第一工具**（零代码、PMC、官方支持）。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 13.1–13.2 子命令概述与单行命令 | [notes/section-13.1-13.2-子命令概述与单行命令.md](./notes/section-13.1-13.2-子命令概述与单行命令.md) |
| 13.3–13.7 perf 事件源 | [notes/section-13.3-13.7-perf-事件源.md](./notes/section-13.3-13.7-perf-事件源.md) |
| 13.8 `perf stat` — 事件计数 | [notes/section-13.8-perf-stat-事件计数.md](./notes/section-13.8-perf-stat-事件计数.md) |
| 13.9 `perf record` — 剖析采样 | [notes/section-13.9-perf-record-剖析采样.md](./notes/section-13.9-perf-record-剖析采样.md) |
| 13.10 `perf report` 与 `perf script` | [notes/section-13.10-perf-report-与-perf-script.md](./notes/section-13.10-perf-report-与-perf-script.md) |
| 13.11 `perf trace` — 系统调用追踪 | [notes/section-13.11-perf-trace-系统调用追踪.md](./notes/section-13.11-perf-trace-系统调用追踪.md) |
| 13.12 其他常用能力（延伸） | [notes/section-13.12-其他常用能力延伸.md](./notes/section-13.12-其他常用能力延伸.md) |

---

## 大白话 · 本章就五件事

> **perf = 计数（stat）+ 采样（record）+ 追踪（trace），接硬件到用户态。**

**① 子命令分工：`stat` 数事件，`record` 采栈，`report/script` 读结果，`trace` 跟 syscall。**

- 生产危机：**先 `perf stat` / `perf top`** — 低开销；深入再短 **`perf record -g`**。

**② 四类事件源：Hardware / Software / Tracepoint / Probe。**

- **PMC**：cycles、IPC、cache miss — Ch 6 周期分析落地。
- **tracepoint**：syscall、block I/O；**kprobe/uprobe/USDT**：动态插桩。

**③ `perf stat` — 计数，极低开销。**

- 全局或 `-p PID`；`-I 1000` 间隔统计；算 **IPC**、看 branch-miss。

**④ `perf record` + `report`/`script` — 采样剖析 → 火焰图。**

- **99 Hz** 常见（减与 timer 锁频）；**`-g`** 采调用栈；**`-F` 频率采样** vs **`-c` 周期/event 计数触发**。
- `perf script | stackcollapse | flamegraph.pl` — **火焰图必备管道**。

**⑤ 栈与符号：fp、`-g`、debuginfo — 否则 Ch 5 Gotchas 全中。**

- Release 构建 **`-g -fno-omit-frame-pointer`**；Java 要 perf-map-agent。

下面按原书 13.1–13.12 展开。

---

## 本章 Checklist

- [ ] `perf` 版本与 **运行内核匹配**
- [ ] 会用 **`perf stat`** 算 IPC、看 cache-miss
- [ ] 会 **`perf record -F 99 -g --call-graph fp -p PID`**
- [ ] 会从 **`perf script`** 生成 **CPU 火焰图**
- [ ] Release 构建保留 **符号 + 帧指针**（Ch 5）
- [ ] 知道 **on-CPU perf** vs **off-CPU BPF** 分工
- [ ] 生产：**stat/top 优先**，record **限时长**

---

## HFT 精读捷径（Ch 13 在路线中的位置）

```
Ch 4  观测工具选型
Ch 5–10  各资源章「perf 能做什么」
Ch 12  压测时必须 profile
Ch 13  perf（本章：官方剖析器实操）
  → Ch 15 BPF（可编程、off-CPU、runqlat）
  → 04-BPF 专书
  → 12-HFT ch10 延迟与回归
```

**本章最小行动集：**

1. 裸机确认 **`perf --version`** 与 **`uname -r`** 匹配。
2. 对 strategy 跑 **`perf stat -e cycles,instructions,cache-misses -- sleep 5`** — 记 IPC。
3. **`perf record -F 99 -g -p PID -- sleep 30`** → 火焰图一张。
4. 检查二进制：**`-g -fno-omit-frame-pointer`**，`perf report` 无大片 `[unknown]`。

**Gregg 本章金句（HFT 版）：**

> **`perf` 是 Linux 性能分析的默认答案** — `stat` 数清楚，`record` 采明白，**script 画火焰图**。  
> 没有栈和符号的 profile **等于没 profile** — 编译时就要为 perf 留后路。

---

## 相关章节

- 上一章：[../chapter-12-benchmarking/](../chapter-12-benchmarking/)
- 下一章：[../chapter-14-ftrace/](../chapter-14-ftrace/)
- 观测地图：[../chapter-04-observability-tools/](../chapter-04-observability-tools/)
- CPU / PMC：[../chapter-06-cpus/](../chapter-06-cpus/)
- 应用剖析：[../chapter-05-applications/](../chapter-05-applications/)
- 缺页：[../chapter-07-memory/](../chapter-07-memory/)
- BPF 互补：[../chapter-15-bpf/](../chapter-15-bpf/)
- 附录 C bpftrace：[appendix-C-bpftrace单行命令.md](../appendix-C-bpftrace单行命令.md)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
