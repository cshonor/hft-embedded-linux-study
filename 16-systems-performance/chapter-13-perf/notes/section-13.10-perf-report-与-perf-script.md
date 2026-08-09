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


### 常见陷阱

1. report 只看 overhead%——Children 列（含子调用累计）也很重要，单看 overhead 会漏调用链热点
2. script 不用 stackcollapse——直接看 perf script 输出是逐行原始样本，需要 stackcollapse + flamegraph 才可视化
3. on-CPU 火焰图当全部——off-CPU 时间（等锁/IO/调度）用 perf record 看不到，需 BPF offcputime

<details>
<summary>自测题（点击展开）</summary>

1. perf report 的 Overhead% 和 Children 列有什么区别？
   <details><summary>答</summary>Overhead = 当前函数自身样本占比；Children = 含子调用累计占比——Children 高说明调用链热</details>
2. perf script 输出如何变成火焰图？
   <details><summary>答</summary>perf script | stackcollapse-perf.pl | flamegraph.pl > out.svg——需要 FlameGraph 仓库工具</details>
3. on-CPU 火焰图的局限？
   <details><summary>答</summary>只显示在 CPU 上执行的时间——等锁/IO/调度的时间不在图上，需 offcputime 补充</details>

</details>


---

← [本章导读](../README.md)
