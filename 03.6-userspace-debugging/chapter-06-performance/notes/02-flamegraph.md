# 6.2 火焰图（调用链可视化 / cache miss 初探）

> 选读 · 把「谁调用了热点」画成一张图

## 本节要点

`perf report` 给的是扁平列表，能看出「哪个函数占 CPU 多」，但看不出**调用关系**——`slow_sqrt` 是被 3 个不同上层各自调了 33%，还是被 1 个上层调了 99%？优化策略完全不同。**火焰图（FlameGraph）**把「调用栈 + 时间占比」画成一张可交互的 SVG 图，一眼看清热点及其调用链。本节讲火焰图的生成、读法，以及 cache miss 的初探。

## 火焰图长什么样

用 6.1 的 `hot.c` 生成的火焰图（概念示意）：

```
        ┌─────────────┬──────────┐
  main  │             │          │        ← 底部 = 调用栈根部
        └──────┬──────┴───┬──────┘
        ┌──────▼──────┐┌──▼───────┐
 do_work│            ││ (其它)   │
        └──────┬──────┘└──────────┘
        ┌──────▼──────┐
slow_sqrt│████████████│           ← 顶部宽框 = 热点
        └─────────────┘
  ↑ 横轴宽度 = 该函数（含其子孙）占的总采样比例
```

- **横轴**：字母序排列，宽度 = 采样占比（CPU 时间占比）。
- **纵轴**：调用栈深度，从下往上 = 从 main 到叶子函数。
- **每个框**：一个函数，框越宽 = 越热。

## 生成流程（四步）

火焰图是 Brendan Gregg 的开源工具，配合 perf 使用：

```bash
# 1. 采样（必须 -g 记录调用栈）
perf record -g -F 99 ./hot

# 2. 把 perf.data 转成文本调用栈
perf script > out.perf

# 3. 折叠调用栈（stackcollapse 脚本）
stackcollapse-perf.pl out.perf > out.folded

# 4. 生成火焰图 SVG
flamegraph.pl out.folded > flame.svg
```

其中 `stackcollapse-perf.pl` 和 `flamegraph.pl` 来自 [FlameGraph](https://github.com/brendangregg/FlameGraph) 仓库：

```bash
git clone https://github.com/brendangregg/FlameGraph.git
export PATH=$PATH:$PWD/FlameGraph
```

生成后用浏览器打开 `flame.svg`，可**悬停查看**每个框的函数名、采样次数、占比，可点击放大。

## 读火焰图的四个要点

| 读法 | 含义 | 优化启示 |
|------|------|----------|
| **宽框** | CPU 时间占比高 | 首要优化目标 |
| **平顶（plateau）** | 某函数直接消耗大量 CPU（如 `slow_sqrt` 顶部一片平） | 优化这个函数本身 |
| **塔尖（tower）** | 一个调用链层层嵌套很深 | 优化链路中间的某层，或减少调用次数 |
| **底部有多个分叉** | 同一热点被多个上层调用 | 先看哪个上层贡献大，优化调用方而非函数 |

> 区分「平顶」和「塔尖」是火焰图最大的价值：**平顶 = 函数自身耗时（self time）高，优化函数体；塔尖 = 调用链长，优化调用关系/减少调用次数**。`perf report` 扁平列表分不清这两者。

## cache miss 初探

6.1 提到低 IPC（访存密集）要看 cache miss。perf 用硬件事件采样 cache：

```bash
# 统计 cache 命中率
perf stat -e cache-references,cache-misses ./hot
#        12,345,678  cache-references
#         1,234,567  cache-misses        # 10.0% 的 cache miss 率

# 采样「哪行代码在频繁 cache miss」
perf record -e cache-misses ./hot
perf report
```

cache miss 率高（HFT 里通常 >3% 就要警惕）说明数据访问模式差——常是**数据结构布局**问题（遍历链表而非数组、结构体字段分散、false sharing）。这是低延迟优化的重要入口，但深入交给 06.6。

> **注意**：perf 的事件名因 CPU 架构而异（`cache-misses` 在 x86 通用，ARM 上可能叫别的），用 `perf list` 查看当前机器支持的事件。树莓派 5（aarch64）上的事件名和 x86 不完全一致。

## 火焰图的局限

1. **需要 `-g` 调用栈**：没记录调用栈就画不出纵深。
2. **栈可能不完整**：`-fomit-frame-pointer`（`-O2` 默认）编译的程序，perf 靠 frame pointer 回溯会丢栈；需要 `--call-graph dwarf`（用 DWARF 回溯，更准但更慢）或编译加 `-fno-omit-frame-pointer`。
3. **采样是统计**：火焰图也是采样近似，微小开销看不清。
4. **只反映 CPU 采样**：默认火焰图反映的是 CPU 时间，不能直接反映「锁等待」「IO 等待」（那需要 off-CPU 火焰图，进阶内容，见 06.6）。

```bash
# 栈回溯不准时，用 DWARF 回溯重新采样
perf record -g --call-graph dwarf -F 99 ./hot
# 或编译时保留 frame pointer
gcc -g -O2 -fno-omit-frame-pointer -o hot hot.c
```

## HFT 关联

1. **「平顶 vs 塔尖」决定优化策略**：撮合引擎热点若是「平顶」（某价格计算函数自身贵），优化函数体（换算法/查表）；若是「塔尖」（层层调用），优化调用链（减少中间层、合并函数）。选错方向白费力气。
2. **cache miss 火焰图找「刷缓存」的代码**：HFT 低延迟的敌人是 cache miss。`perf record -e cache-misses` + 火焰图能定位「哪行代码在频繁把数据踢出缓存」，往往是遍历链表、跨 cache line 访问、false sharing 的地方。
3. **火焰图是性能汇报的通用语言**：给团队/上级讲「为什么慢、优化后提升多少」，一张前后对比的火焰图比一页数字有说服力得多——宽度肉眼可见地变窄。
4. **frame pointer 的取舍**：为了火焰图栈完整，调试构建加 `-fno-omit-frame-pointer`（有 <1% 开销）；生产构建权衡后通常还是保留 `-O2` 默认，靠 DWARF 回溯补采样。

```bash
# HFT 场景：撮合引擎 off-CPU 前的 CPU 火焰图
perf record -g -F 99 ./matching_engine --sim data.csv
perf script | stackcollapse-perf.pl | flamegraph.pl > engine_cpu.svg
# 打开 svg，找最宽的那个框 = 头号优化目标
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 火焰图横轴、纵轴、每个框分别代表什么？

> 横轴：按字母序排列的采样占比，框越宽 = 该函数（含其调用的子孙）占 CPU 时间越多。纵轴：调用栈深度，从下往上 = 从根部（main）到叶子函数。每个框：一个函数，宽度即其时间占比。整张图本质是「所有采样到的调用栈」按占比堆叠而成。

**Q2:** 「平顶」和「塔尖」有什么区别？为什么区分它们重要？

> **平顶**：某个函数顶部是一大片平坦宽度，说明这个函数**自身耗时（self time）高**（如循环、除法），优化方向是改函数体（换算法、查表）。**塔尖**：一条调用链层层收窄、很高，说明耗时在**调用关系**上（调用次数多、链路深），优化方向是减少调用次数、合并中间层。`perf report` 扁平列表只给「总占比」分不清 self time 和调用开销，火焰图能一眼区分，选错优化方向会白费力气。

**Q3:** 生成火焰图的四步流程是什么？哪一步最容易出错？

> ① `perf record -g` 采样（带调用栈）；② `perf script > out.perf` 转文本；③ `stackcollapse-perf.pl out.perf > out.folded` 折叠栈；④ `flamegraph.pl out.folded > flame.svg` 生成图。最容易错在第一步：忘加 `-g` 就没调用栈画不出纵深；或 `-O2` 默认 `-fomit-frame-pointer` 导致栈回溯不完整，需改用 `--call-graph dwarf` 或编译加 `-fno-omit-frame-pointer`。

**Q4:** cache miss 率和程序性能什么关系？怎么定位「哪行代码在 miss」？

> cache miss 意味着 CPU 要访问的数据不在高速缓存里，得去慢几十倍的主存取，CPU 空等（stall），表现为低 IPC、低吞吐。cache miss 率高说明数据访问模式差。定位方法：`perf stat -e cache-references,cache-misses` 看整体 miss 率；`perf record -e cache-misses` + `perf report`（或火焰图）定位到具体函数/代码行。HFT 里 >3% 通常就要警惕。

**Q5:** CPU 火焰图和 off-CPU 火焰图的区别？什么时候需要 off-CPU？

> CPU 火焰图（默认）反映「CPU 时间花在哪」，只看得见「正在计算」的热点。off-CPU 火焰图反映「线程不在 CPU 上的时间花在哪」——锁等待、IO 等待、sleep 等。当程序「慢」但不是 CPU 忙（比如 `perf stat` 显示 task-clock 远小于 wall time），说明瓶颈在等待而非计算，这时需要 off-CPU 火焰图才能看到「卡在哪」。它是进阶内容，深入见 06.6。

</details>

## 交叉引用

- [6.1 perf 基础采样](01-perf-basics.md)
- [06.6 Systems Performance](../../../06.6-systems-performance/README.md)
- [Ch6 性能类](../README.md)
