## 5.6 常见陷阱（Gotchas）

> 章节导航：[5.5 观测工具](./section-5.5-观测工具.md) · 上一篇 ← · [本章导读](../README.md)

**本节讲什么**：应用层观测的三大可观测性陷阱——缺失符号（[unknown]）、缺失堆栈（栈断层）、过度 inline——的机制与工程对策，以及 HFT 发布构建的可观测性配置基线。

### 要点

| # | 要点 | 一句话 |
|---|------|--------|
| 1 | 可观测性是**编译期决定**的 | 出事时补不回来 |
| 2 | `[unknown]` = 符号缺失；**平头栈** = 帧指针缺失 | 两个不同病 |
| 3 | **-O3 与可观测性有权衡** | inline 太狠栈变浅 |
| 4 | JIT 语言要**额外喂符号**给 perf | perf-map 机制 |
| 5 | Release 也要 `-g` + 帧指针 | 危机时刻 5 分钟定生死 |

---

### 一、Missing Symbols（缺失符号）

火焰图 / perf report 出现 `[unknown]` 或裸 `0x7f...` 地址：

| 原因 | 机制 | 解决 |
|------|------|------|
| **strip** 了符号表 | 发布流水线默认 strip | 编译加 `-g`，发布用 **split debuginfo**（符号进单独 debug 包） |
| 动态库无 debuginfo | 系统库只装了运行时 | 安装 `-dbg` / `-debuginfo` 包 |
| **JIT**（Java、Node） | 代码运行时生成，没有 ELF 符号 | `perf-map-agent`、`-XX:+PreserveFramePointer`、JITDump |

**split debuginfo 的工作方式**：二进制里的 `.gnu_debuglink` 指向外部 debug 文件——生产机器只部署轻量二进制，分析机上有 debug 包即可还原符号。**两头兼顾**：发布体积小 + 危机 perf 可读。

**JIT 符号问题的本质**：perf 采到的 PC 指向运行时生成的机器码，ELF 里查无此址——JVM 的解法是往 `/tmp/perf-PID.map` 写运行时映射（perf-map-agent），或 JITDump 直接喂符号化信息；`-XX:+PreserveFramePointer` 让栈回溯可用。

### 二、Missing Stacks（缺失堆栈）

栈断层 → 火焰图「平头」、深度不够：

| 原因 | 机制 | 解决 |
|------|------|------|
| **省略帧指针**（`-fomit-frame-pointer`） | 栈上没有回溯链，fp 回溯断 | 编译 `-fno-omit-frame-pointer`（叶子函数可 `-mno-omit-leaf-frame-pointer` 折中） |
| 栈太深 / 采样限制 | 回溯层数上限 | 增大 `--call-graph fp` 深度 / `--stack-size` |
| **inline 过多** | 函数被内联后栈上没有独立帧 | 权衡 `-O3` 与可观测性；`--inlines` 展示内联帧 |
| unwind 信息缺失（dwarf 模式） | `.eh_frame` 不完整 | `--call-graph dwarf` 需要完整 debuginfo |

**fp vs dwarf vs lbr 三种栈回溯的取舍**（[ch13.9](../../chapter-13-perf/notes/section-13.9-perf-record-剖析采样.md) 详细对比）：

| 方式 | 前提 | 开销 | 精度 |
|------|------|------|------|
| **fp（帧指针）** | 编译保留 fp | 最低 | 好（除 inline/汇编帧） |
| dwarf | debuginfo | 高（每样本拷贝栈） | 好 |
| lbr | Intel LBR 硬件 | 低 | 硬件栈深有限 |

**HFT 默认 fp**：开销最低且常驻可接受——代价是编译选项（本节核心）。

### 三、过度 inline 的双重代价

`-O3` 激进 inline：

1. **性能反面**：函数体膨胀 → I-cache 命中率下降（热路径代码挤爆 L1i）——有时 -O2 反而更快（[5.3](./section-5.3-编程语言与垃圾回收.md)）。
2. **可观测反面**：内联函数没有独立栈帧——火焰图「平头」，多个逻辑函数合并成一坨，**热点看不出调用层次**。

**权衡手段**：关键模块 `-fno-inline`（或 `__attribute__((noinline))`）标注；PGO 让编译器按真实热度决定 inline（冷的展开、热的保留）。

### 四、HFT 发布构建基线

```
Release：-O3 -g -fno-omit-frame-pointer
Debug symbols：单独 debug 包（split debuginfo），生产按需挂载
线程：命名（pthread_setname_np）——pidstat -t / 火焰图直接可读
USDT：关键阶段埋探针（decode/strategy/risk/send）——危机时 BPF 直读
验证：上线前跑一次 perf record -g，确认火焰图可读、无 [unknown] 大块
```

**最后一条是纪律**：可观测性配置改完要**验证**——发布前 5 分钟的 perf 采样能救危机时的 5 小时。这是 [ch16「baseline 事前纪律」](../../chapter-16-case-studies/)在构建维度的版本：**不是出事时才想符号，是构建时保证出事时能读**。

### 五、症状速查表

| 火焰图症状 | 病因 | 方向 |
|-----------|------|------|
| 大块 `[unknown]` | 符号缺失 | split debuginfo / debuginfo 包 |
| 栈全 2-3 层就断（平头） | 帧指针被 omit | -fno-omit-frame-pointer |
| 内核侧正常用户侧全断 | 用户态二进制没 -g | 用户态构建选项 |
| 地址正常但函数名怪 | JIT 代码 | perf-map / PreserveFramePointer |
| 个别深递归断 | 回溯深度上限 | --call-graph 深度 |
| 同一函数巨大无层次 | inline 过度 | noinline 标注 / PGO |

### 衔接

- 上一节：[5.5 观测工具](./section-5.5-观测工具.md)
- 关联：[ch13 perf 栈回溯](../../chapter-13-perf/notes/section-13.9-perf-record-剖析采样.md)、[ch15 BPF 栈](../../chapter-15-bpf/)、[ch12 baseline 纪律](../../chapter-12-benchmarking/notes/section-12.1-基准测试的背景与挑战.md)、[ch16 案例研究](../../chapter-16-case-studies/)

---

### 常见陷阱

1. **-O2 默认 omit 帧指针不知道**——主流平台 -O2 起默认 `-fomit-frame-pointer`，perf 栈全是 [unknown]；显式加回。
2. **strip 后不备 debug 包**——生产 perf report 无函数名；split debuginfo 流程要在 CI 里。
3. **inline 过度只看性能不看可观测**——火焰图平头同样阻碍危机归因。
4. **JIT 应用不配 perf-map**——用户态栈全断；PreserveFramePointer 一行参数。
5. **构建选项改完不验证**——上线前跑一次 perf record -g 确认可读。

<details>
<summary>自测题（点击展开）</summary>

1. 为什么 HFT Release 构建要保留帧指针？
   <details><summary>答</summary>fp 是开销最低的栈回溯方式（比 dwarf 便宜）——保住它，危机时 perf 火焰图可读；-O2 起默认会 omit，要显式加回。</details>
2. split debuginfo 怎么两头兼顾？
   <details><summary>答</summary>.gnu_debuglink 指向外部 debug 文件——生产部署轻量二进制，分析机挂 debug 包还原符号。</details>
3. [unknown] 和平头栈分别是缺什么？
   <details><summary>答</summary>[unknown] = 符号缺失（strip/无 debuginfo）；平头栈 = 帧指针缺失（omit-frame-pointer）——两个不同病两个不同药。</details>
4. JIT 语言的符号问题怎么解？
   <details><summary>答</summary>运行时代码无 ELF 符号——perf-map-agent 写 /tmp/perf-PID.map、JITDump、或 -XX:+PreserveFramePointer 保栈回溯。</details>
5. inline 过度的两个代价？
   <details><summary>答</summary>①I-cache 命中率下降（可能比 -O2 慢）②火焰图平头（函数无独立帧，热点无层次）——PGO/noinline 控制。</details>

</details>


---

← [本章导读](../README.md)
