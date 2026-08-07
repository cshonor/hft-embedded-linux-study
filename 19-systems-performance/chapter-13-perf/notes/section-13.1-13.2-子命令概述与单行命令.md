## 13.1–13.2 子命令概述与单行命令

### perf 工具集架构

```
perf
├── stat      事件计数（低开销）
├── record    采样 → perf.data
├── report    交互/文本汇总热点
├── script    逐行样本 → 火焰图输入
├── top       实时 TUI 热点
├── trace     syscall 追踪（低开销 strace）
├── list      列出可用 events
├── probe     创建 kprobe/uprobe
└── ...       mem, sched, lock, stat 等扩展
```

**版本：** `perf` 需 **匹配运行内核**（`linux-tools-$(uname -r)`）— Ch 4 危机工具清单。

### 单行命令集锦（HFT 常备）

```bash
# --- 快速健康 ---
perf stat -e cycles,instructions,cache-misses,branch-misses -- sleep 1
perf stat -p $(pidof strategy) -- sleep 5

# --- IPC + 缺页 ---
perf stat -e cycles,instructions,page-faults,major-faults -p $(pidof strategy) -- sleep 10

# --- CPU 热点（短采，限时长）---
perf record -F 99 -g -p $(pidof strategy) -- sleep 30
perf report --stdio | head -40

# --- 火焰图管道（需 FlameGraph 仓库）---
perf record -F 99 -g -p $(pidof strategy) -- sleep 60
perf script | stackcollapse-perf.pl | flamegraph.pl > strategy.svg

# --- 实时 top ---
perf top -p $(pidof strategy)

# --- syscall 追踪（开发/debug，生产限时长）---
perf trace -p $(pidof strategy) -- sleep 5

# --- 列出事件 ---
perf list | grep -E 'cache|fault|sched'
```

**生产原则：** `stat`/`top` 优先；`record` **限 PID + 限时长**；`trace` 比 strace 轻但仍非零开销。

→ Ch 4 [perf 定位](../../chapter-04-observability-tools/) · Ch 12 [压测时 profile](../../chapter-12-benchmarking/)

---


### 常见陷阱

1. perf 版本不匹配内核——perf 需要 linux-tools-$(uname -r)，不匹配时事件不可用或数据错误
2. perf record 生产不限时长——perf record 有开销（采样写入），生产应限 PID + 限时长
3. perf trace 当 strace 长跑——perf trace 比 strace 轻但仍非零开销，生产限时长

<details>
<summary>自测题（点击展开）</summary>

1. perf 的核心子命令有哪些？
   <details><summary>答</summary>stat（计数）、record（采样）、report（热点）、script（逐行→火焰图）、top（实时）、trace（syscall）</details>
2. perf 版本为什么必须匹配内核？
   <details><summary>答</summary>perf 需要 linux-tools-$(uname -r)——不匹配时 PMC/tracepoint 事件可能不可用或数据错误</details>
3. HFT 生产环境 perf 的使用原则？
   <details><summary>答</summary>stat/top 优先（低开销）；record 限 PID + 限时长；trace 仍限时长</details>

</details>


---

← [本章导读](../README.md)
