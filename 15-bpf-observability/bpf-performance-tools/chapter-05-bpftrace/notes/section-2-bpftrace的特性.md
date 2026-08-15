# 5.2 bpftrace 的特性

> 底本：《BPF之巅》第 5 章 bpftrace（印刷 p137–190），5.2 节（印刷 p139–141）

## 内容详解

与 BCC 不同（内核态/用户态两套 API），bpftrace **只有一种 API：bpftrace 编程语言**。特性按事件源、动作、一般特性分类。

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

### 5.2.2 动作（事件触发后可执行，关键部分）

- 过滤（谓词条件）
- 每事件输出 `printf()`
- 基础变量（global、scratch `$x`、per `[tid]`）
- 内置变量（pid、tid、comm、nsecs…）
- 关联数组（`key[value]`）
- 频率计数（`count()` 或 `++`）
- 统计值（min()/max()/sum()/avg()/stats()）
- 直方图（hist()/lhist()）
- 时间戳和时间差（nsecs 及哈希存储）
- 调用栈：kstack（内核态）/ ustack（用户态）
- 符号解析：ksym()/kaddr()（内核）、usym()/uaddr()（用户）
- 访问 C 结构体成员（`->`）、数组访问（`[]`）
- shell 命令 `system()`、打印文件 `cat()`
- 基于位置的参数（`$1`、`$2`…）

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

**作者结论：无须拘泥于 bpftrace。目标是解决问题，有时组合使用工具更快。**

## HFT 关联

- 嵌入式/资源受限的行情网卡机器上 Ftrace（无 LLVM 依赖）可能更合适；正常交易/回测机用 bpftrace 获得内核态直方图能力。
- 对比 perf：需要**自定义分布**（延迟直方图、报文大小分布）时 bpftrace 胜；需要**全量事件落盘离线分析**时 perf record 胜。

## 陷阱

- ⚠️ "bpftrace 万能"是误区——函数级高频统计 Ftrace 更快，事件转储分析 LTTng/perf 更强；按问题选工具。

<details>
<summary>自测题</summary>

1. bpftrace 有几套 API？与 BCC 有何不同？
   <details><summary>答案</summary>一套（bpftrace 语言）；BCC 分内核态（BPF C）与用户态（Python）两套。</details>

2. 与 Ftrace 相比 bpftrace 的劣势场景？
   <details><summary>答案</summary>依赖更多（LLVM/Clang），不适合嵌入式；函数统计类 Ftrace 版本启停更快、开销更低。</details>
</details>
