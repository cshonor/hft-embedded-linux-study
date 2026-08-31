# 5.2 bpftrace 的特性

> 底本：《BPF之巅》第 5 章 bpftrace（印刷 p137–190），5.2 节（印刷 p139–141）

## 内容详解

与 BCC 不同（内核态/用户态两套 API），bpftrace **只有一种 API：bpftrace 编程语言**。特性按事件源、动作、一般特性分类。

### "只有一种 API"意味着什么

BCC 的双执行域让你在两种语言间**手动搬运上下文**（BPF C 里算好 → map → Python 里取出来格式化）；bpftrace 把这条鸿沟缝死了：

```
BCC:   [BPF C: 探针+过滤+聚合] --map--> [Python: 取数+格式化+打印]
                     ↑ 你要同时维护两边的一致性（键类型、列宽、单位换算）

bpftrace: [probe /filter/ { action }]
          一门语言写到底，map 的创建/键类型/打印全部由语言隐式管理
```

代价是表达力封顶（无浮点、无自由循环、语言刻意保持小）；收益是**没有"两边对不上"这类 bug 的生存空间**。

### 5.2.1 事件源（括号内为 bpftrace 探针类型）

| 内核技术（第 2 章） | bpftrace 探针类型 |
|--------------------|-------------------|
| 动态插桩，内核态 | `kprobe` |
| 动态插桩，用户态 | `uprobe` |
| 静态跟踪，内核态 | `tracepoint`、`software` |
| 静态跟踪，用户态 | `usdt`（借助 libbcc） |
| 定期事件采样 | `profile` |
| 周期事件 | `interval` |
| PMC 事件 | `hardware` |
| 合成事件 | `BEGIN`、`END` |

计划中：sockets 和 skb 事件、裸跟踪点、内存断点、自定义 PMC 事件。

观察这张表的**骨架**：行 = 第 2 章事件源的"态×方式"正交网格（内核/用户 × 动态/静态/采样），列 = 语言里的探针类型名。也就是说 bpftrace 把第 2 章的底层技术矩阵**原样投影成了语法**——学完 ch02 再看这张表，每个探针类型背后的事件源开销模型（kprobe ~百 ns、tracepoint ~几十 ns、profile 固定频率税）可以直接沿用，不用重学。

### 5.2.2 动作（事件触发后可执行，关键部分）

按用途重新分组（原书是平铺清单）：

| 用途 | 动作 | 备注 |
|------|------|------|
| **控制要不要算** | 过滤（谓词条件） | `/pid == 123/`，内核态先筛 |
| **逐事件看细节** | `printf()` | 每事件一次，开销最高的一档 |
| **存中间值** | 基础变量（global、scratch `$x`、per `[tid]`）、内置变量（pid、tid、comm、nsecs…） | 计时器的 `@start[tid]` 就是 per-键变量 |
| **聚合** | 关联数组 `key[value]`、`count()`/`++`、min/max/sum/avg/stats()、hist()/lhist() | 内核态聚合，Ctrl-C 才打印——低开销的关键 |
| **计时** | 时间戳和时间差（nsecs 及哈希存储） | 双探针计时模板的原料 |
| **归因** | 调用栈 kstack/ustack；符号解析 ksym()/kaddr()/usym()/uaddr() | 配 stackid map（见 5.16 语义分析器） |
| **读数据** | 访问 C 结构体成员 `->`、数组 `[]` | 靠 Clang 解析头文件 |
| **联动外部** | shell 命令 `system()`、打印文件 `cat()` | 需 `--unsafe`，见 5.8 |
| **脚本参数** | 位置参数 `$1`、`$2`… | 让 .bt 工具可参数化 |

聚合 + 计时两族动作覆盖了性能分析 80% 的需求（分布、延迟、计数），且全部在内核态完成——这是 bpftrace 低开销的结构性原因。

设计取向：**语言规模保持越小越好，易于学习**。

### 5.2.3 一般特性

- 额外开销较低的插桩技术（BPF JIT 和映射表）
- 生产环境安全性（BPF 验证器）
- 众多工具（/tools）、新手指引、参考手册（/docs）

### 5.2.4 与其他观测工具的比较

| 工具 | 对比要点 |
|------|----------|
| **perf(1)** | bpftrace 语言简练；perf 脚本语言冗长但 perf record 转储高效。bpftrace 可在内核中高效统计（自定义直方图），perf 内核态统计只有简单计数（perf stat）。perf 也能跑 BPF（附录 D） |
| **Ftrace** | Ftrace 自有语法（hist-triggers 等）；bpftrace 近 C/awk。Ftrace 依赖更少、**适合嵌入式**；函数统计（function profiling）经过专门优化，Ftrace 版 funccount 启停更快、开销更低 |
| **SystemTap** | 都有高级语言；SystemTap 用自研内核模块，在 RHEL 之外的发行版不可靠（正在推动 BPF 后端）；SystemTap 的 tapsets 库辅助函数更多 |
| **LTTng** | LTTng 优化事件导出转储+离线分析；bpftrace 定位是**临时性实时分析**——方法论完全不同 |
| **应用程序定制工具** | 只能看用户态；bpftrace 能同时探内核与硬件，确认问题范围更广，但需要自行编码 |

按"你要什么"翻译成选型决策：

| 需求 | 首选 |
|------|------|
| 自定义延迟分布直方图（内核态聚合） | bpftrace |
| 全量事件落盘、离线反复分析 | perf record / LTTng |
| 嵌入式/无 LLVM 环境 | Ftrace |
| 高频函数计数（启停快、税低） | Ftrace function profiling |
| RHEL 系 + 已有 tapset 资产 | SystemTap（其他发行版别碰） |
| 只有用户态、且厂商提供探针 | 应用定制工具 |

**作者结论：无须拘泥于 bpftrace。目标是解决问题，有时组合使用工具更快。**

## HFT 关联

- 嵌入式/资源受限的行情网卡机器上 Ftrace（无 LLVM 依赖）可能更合适；正常交易/回测机用 bpftrace 获得内核态直方图能力。
- 对比 perf：需要**自定义分布**（延迟直方图、报文大小分布）时 bpftrace 胜；需要**全量事件落盘离线分析**时 perf record 胜。
- 一个现实组合拳：盘中用 bpftrace 实时盯直方图（聚合、不落盘、开销可控），异常时段切换 perf record 全量抓现场，盘后离线交叉分析——实时排障与取证两条线用不同工具，别让一个工具干两种活。

## 陷阱

- ⚠️ "bpftrace 万能"是误区——函数级高频统计 Ftrace 更快，事件转储分析 LTTng/perf 更强；按问题选工具。
- ⚠️ "一种 API"不等于"一种实现"：底层仍是 libbcc/libbpf + LLVM（5.16），部署依赖并没有消失，消失的只是你写代码时的双语言切换成本。

<details>
<summary>自测题</summary>

1. bpftrace 有几套 API？与 BCC 有何不同？
   <details><summary>答案</summary>一套（bpftrace 语言）；BCC 分内核态（BPF C）与用户态（Python）两套。</details>

2. 与 Ftrace 相比 bpftrace 的劣势场景？
   <details><summary>答案</summary>依赖更多（LLVM/Clang），不适合嵌入式；函数统计类 Ftrace 版本启停更快、开销更低。</details>

3. 5.2.2 的动作清单里，哪一族动作是"低开销看分布"的关键？为什么？
   <details><summary>答案</summary>聚合族（count/sum/hist 等 + 关联数组）：每事件只在内核态做一次 map 更新（约百 ns 级），不产生逐事件输出，Ctrl-C 才统一打印——对比 printf 每事件都要走 perf 缓冲区+用户态格式化。</details>
</details>
