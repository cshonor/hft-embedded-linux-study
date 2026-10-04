# 4.2 BCC 的特性

> 底本：《BPF之巅》第 4 章 BCC（印刷 p91–136），4.2 节

## 内容详解

BCC 的特性分**内核态**（BPF C 程序里可用的能力）与**用户态**（Python 等前端提供的能力）两层。

### 内核态特性（BPF C 程序可用）

书中列出 17 项，核心包括：

| 类别 | 特性 |
|------|------|
| 探针挂载 | kprobes、kretprobes、uprobes、uretprobes、tracepoints、USDT |
| 辅助函数 | `bpf_ktime_get_ns()`、`bpf_get_current_pid_tgid()`、`bpf_get_current_uid_gid()`、`bpf_get_current_comm()`、`bpf_probe_read()`、`bpf_probe_read_str()`、`bpf_trace_printk()` |
| 映射表 | `BPF_HASH`、`BPF_HISTOGRAM`、`BPF_ARRAY`、`BPF_PERF_OUTPUT`、`BPF_STACK_TRACE`、`BPF_PERCPU_HASH` 等 |

### 用户态特性（Python 前端）

| 特性 | 作用 |
|------|------|
| `BPF(text=...)` | 嵌入 BPF C 源码并即时编译、加载 |
| `b["table"]` / `b.get_table()` | 访问 BPF 映射表（Python 魔术方法，两种写法等价） |
| `b.trace_pipe()` | 读取 `bpf_trace_printk()` 输出（调试用） |
| `b["events"].open_perf_buffer(cb)` / `perf_reader_poll()` | perf 事件缓冲区回调读数据 |
| `b.print_log2_hist()` | 打印 2 的幂次直方图（`BPF_HISTOGRAM` 配套） |
| `bcc.ksym()` / `usym()` | 内核/用户态符号解析 |
| `BPF.attach_kprobe()` / `attach_uprobe()` / `attach_tracepoint()` / `attach_usdt()` | 挂载各类探针 |
| `USDT(pid=...)` | USDT 独立对象：需先 attach 到进程 ID 或路径 |

### 两条标准数据通路（写工具必会的模板）

把内核态特性与用户态特性组合，得到两条输出通路——**所有 BCC 工具二选一**：

```text
通路 A（聚合，生产推荐）:
  探针命中 ─▶ BPF_HASH/HISTOGRAM 更新（内核态聚合）
           ─▶ 用户态定时（interval）读 map ─▶ print_log2_hist()/表格

通路 B（逐事件，排障用）:
  探针命中 ─▶ bpf_perf_event_output() 推入 perf 缓冲
           ─▶ open_perf_buffer(cb) + kprobe_poll() 回调 ─▶ printf 每事件一行
```

选 A 还是 B 与 [4.1 的三分天下](./section-1-BCC的组件.md)完全对应：聚合类走 A，逐事件类走 B。**B 通路的天花板是用户态消费速度**——回调里做字符串格式化、打印终端，每秒几万行就开始丢事件（perf buffer 满了覆盖旧数据），所以逐事件工具必须配内核态过滤把量先压下来。

### USDT 为何是独立对象

USDT 与 kprobes/uprobes/tracepoints 行为不同：

1. 初始化时**必须挂载到某个进程 ID 或库路径**；
2. 有些 USDT 需要**在进程映像中设置信号量**来激活——应用程序用该信号量决定是否为探针准备参数，未激活时探针被当作性能优化跳过（详见 2.10）。

## HFT 关联

- `BPF_HASH` + `print_log2_hist()` 是延迟分布工具的标准骨架（runqlat/biolatency 都是这套）；
- `BPF_PERF_OUTPUT` + `open_perf_buffer()` 是**低开销逐事件输出**的正道，比 `bpf_trace_printk()`（全局共享、混杂 Ftrace 输出）适合生产。
- 通路 A/B 的选择纪律在交易机上要写死：常驻工具一律通路 A（map 聚合 + interval 读），通路 B 只允许挂载到已过滤的低频事件（如 `retval != 0` 的错误分支）。

## 陷阱

- ⚠️ `bpf_trace_printk()` 每次调用约 1μs 且全系统共享一个缓冲区，只能调试用，不能进生产路径。
- ⚠️ 直方图是 **2 的幂次分桶**（log2），不是线性桶——读数时要理解 `->usecs` 是桶边界。
- ⚠️ 通路 B 的丢事件是**静默覆盖**（perf buffer 环形写满即弃旧）：看到的输出可能已经缺了最早的样本，统计结论不可靠——输出量不可控时换通路 A。

<details>
<summary>自测题</summary>

1. `counts = b.get_table("counts")` 还可以怎么写？
   <details><summary>答案</summary>`counts = b["counts"]`（Python 魔术方法 `__getitem__`）。</details>

2. USDT 为什么在 BCC 中是独立 Python 对象？
   <details><summary>答案</summary>它必须挂到进程 ID/库路径上，且部分探针需要设置进程映像中的信号量来激活，行为与 kprobe/uprobe/tracepoint 不同。</details>

3. 描述两条标准数据通路，并说明各自的适用场景与天花板。
   <details><summary>答案</summary>通路 A：内核 map 聚合 + 用户态定时读，适合常驻（开销与事件率解耦）；通路 B：perf 缓冲逐事件推送 + 回调打印，适合低频排障（天花板是用户态消费速度，满了静默覆盖旧事件）。</details>

4. 为什么说"输出量不可控时换通路 A"？
   <details><summary>答案</summary>通路 B 的丢事件是环形缓冲静默覆盖——不报错、不留痕，基于不完整数据的统计结论会悄悄失真；通路 A 的输出量只由 interval 参数决定，与事件率无关，量恒可控。</details>
</details>
