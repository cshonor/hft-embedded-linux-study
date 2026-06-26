## 13.10 `perf report` 与 `perf script`

### `perf report`

解析 `perf.data` — TUI 或文本热点。

```bash
perf report --stdio --no-children | head -50
perf report --sort comm,dso,symbol    # 按进程/库/符号
perf report -g graph,0.5,caller        # 调用图
```

| 视图 | 用途 |
|------|------|
| **Overhead %** | 哪个符号占样本比最多 |
| **Children** | 含子调用累计 |
| **DSO** | 哪个 .so/.内核模块 |

### `perf script`

**逐行打印** 每个样本 — 火焰图 **预处理输入**。

```bash
perf script > out.perf
perf script | stackcollapse-perf.pl | flamegraph.pl > cpu.svg
```

**FlameGraph 仓库（Brendan Gregg）：**

```bash
# 克隆一次
git clone https://github.com/brendangregg/FlameGraph
export PATH=$PATH:/path/to/FlameGraph

perf script | stackcollapse-perf.pl | flamegraph.pl --title="strategy CPU" > strategy.svg
```

→ Ch 1/2/5/6 [火焰图读法](../../chapter-02-methodologies/)

**Off-CPU：** `perf record` 默认采 **on-CPU**；off-CPU 用 BPF `offcputime`（Ch 5/15）— **CPU + Off-CPU 火焰图缺一不可**。

---


---

← [本章导读](../README.md)
