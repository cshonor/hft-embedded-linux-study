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


---

← [本章导读](../README.md)
