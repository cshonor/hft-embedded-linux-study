# 14.3–14.4、14.8 函数追踪与 function_graph

> [章节导航](../README.md) · 上一节：[14.1–14.2 核心能力与 tracefs](./section-14.1-14.2-核心能力与-tracefs.md) · 下一节：[14.5–14.7、14.10 事件源、Filter 与 Hist Triggers](./section-14.5-14.714.10-事件源Filter-与-Hist-Triggers.md)

## 本节讲什么

Ftrace 的两个招牌 tracer：

- **`function`**：记录函数**入口**——谁、何时、被谁调用。海量输出，必须 filter。
- **`function_graph`**：入口 + 出口都追——输出**带每函数耗时、带层级的调用图**。查"内核这条路径到底慢在哪一层"的手术刀。

外加 graph tracer 的调优选项（14.8 的内容）与输出精读方法。

---

## 1. function tracer

### 1.1 最小可用流程

```bash
TR=/sys/kernel/tracing
echo function > $TR/current_tracer
echo tcp_v4_rcv > $TR/set_ftrace_filter    # 只追这个函数（关键！）
echo 1 > $TR/tracing_on
# ... 产生负载 ...
echo 0 > $TR/tracing_on
cat $TR/trace | head -50
```

输出形态：

```
# tracer: function
#
          <idle>-0       [003] d..3. 40231.812: tcp_v4_rcv <-__netif_receive_skb_one_core
```

列含义：`任务-PID [CPU] 上下文标志 时间戳: 函数 <- 调用者`。上下文标志（d/h/s…）表示抢占/中断/软中断状态——`d` 是 delayed preempt 计数。

### 1.2 回调侧实现（v6.6）

function tracer 的 ops 回调 `function_trace_call()` 在 trace_functions.c:172（静态 FTRACE 版本在 :25，带 DYNAMIC_FTRACE 的版本在 :172）。它做的事极薄：算好时间戳、调用者/被调者 IP，往 per-CPU ring buffer 写一条事件。**每调用点开销约几百 ns + buffer 写**——这就是不打 filter 会拖垮系统的原因。

### 1.3 filter 三兄弟

| 文件 | 作用 |
|---|---|
| `set_ftrace_filter` | **白名单**：只追这些函数（支持通配 `tcp_*`、`__schedule`） |
| `set_ftrace_notrace` | **黑名单**：追其他所有、跳过这些 |
| `available_filter_functions` | 当前内核**可追的函数全集**（`CONFIG_FUNCTION_TRACER` 编译进来的、未被 notrace 标记的） |

```bash
# 通配 + 追加语法
echo 'tcp_*' > $TR/set_ftrace_filter        # 覆盖写
echo 'udp_rcv' >> $TR/set_ftrace_filter     # 追加
echo > $TR/set_ftrace_filter                # 清空（= 追全部，危险）
grep -c . $TR/available_filter_functions    # 通常 3~5 万个函数
```

> **纪律**：生产上 `set_ftrace_filter` 空着 + `function` tracer = 事件风暴。先 `cat available_filter_functions | grep 目标` 确认函数存在，再窄 filter + 短窗口（秒级）。

---

## 2. function_graph tracer

### 2.1 原理：入口 push、出口 pop

function tracer 只能看到入口（`__fentry__` 只在函数头）。graph tracer 的魔法在于**出口也能追**：

| 步骤 | 机制 | v6.6 锚点 |
|---|---|---|
| ① 函数入口 | `trace_graph_entry()` 被调，把返回地址替换成 `return_to_handler`，并把原返回地址压入 per-task shadow 栈 | trace_functions_graph.c:132 |
| ② 函数返回 | CPU 跳到 `return_to_handler` → `trace_graph_return()`（:241）：弹 shadow 栈拿原返回地址 + 入口时间戳，算出**本函数耗时** | :222–:241 |
| ③ 输出 | 写 `funcgraph_entry` / `funcgraph_exit` 事件到 buffer | — |

shadow 栈深度上限 `FTRACE_RETFUNC_DEPTH = 50`（include/linux/ftrace.h:1089）——超过 50 层递归的路径截断，日志里出现 `UNDERFLOW`/深度省略标记。

全局 graph 配置单例 `graph_array`（trace_functions_graph.c:86）——这也是 v6.6 只支持**一个 trace_array 同时跑 graph** 的根源（多实例支持要到 6.12+ 的 multi-fgraph 重构）。

### 2.2 用法

```bash
TR=/sys/kernel/tracing
echo function_graph > $TR/current_tracer
echo tcp_recvmsg > $TR/set_graph_function   # 只对以它为根的子树画图
echo 1 > $TR/tracing_on
sleep 2
echo 0 > $TR/tracing_on
cat $TR/trace | head -80
```

### 2.3 输出精读（会读 = 会用）

```
# tracer: function_graph
#
 CPU  DURATION       FUNCTION CALLS
#     |     |         |   |   |   |
  3)   0.550 us    |  tcp_v4_rcv() {
  3)             |  ip_rcv_finish() {
  3)   1.204 us    |    ip_route_input_noref();
  3)   2.831 us    |  }                        /* 累计耗时 */
  3)   ! 58.31 us  |  }                        /* tcp_v4_rcv 总耗时 */
```

| 标记 | 含义 |
|---|---|
| 普通数字 | 本函数**叶子耗时**（µs） |
| `}` 后数字 | 本函数**子树累计**耗时（含所有下层调用） |
| `+` | > 10 µs |
| `!` | > 100 µs |
| `#` | 超过 buffer 精度上限 |
| `{` `}` | 调用层级缩进 |
| `....` 空心列 | **被 trace 点亮期间在睡眠**（DURATION 为空 + 点填充） |

**判读纪律**（对照 [ch16 直方图判读](../../chapter-16-case-studies/notes/section-16.0-案例背景An-Unexplained-Win.md) 的「平移 vs 尾部」）：

1. 先看**最外层总量**对不对得上宏观症状（syscall 级 P99）
2. 再找 `!`/`+` 标记——**逐层下钻**到第一个「叶子巨慢」或「层级异常深」的函数
3. 区分「一个函数慢」vs「循环调了太多次」——后者要看调用计数

### 2.4 graph 选项（14.8 精读）

tracer 选项经 `trace_options`（或 `options/` 目录）逐个开关，v6.6 注册在 trace_functions_graph.c:60–80：

| 选项 | 作用 | 默认 |
|---|---|---|
| `funcgraph-tail` | 每层都显示 `}` 尾注（否则只有超阈值层显示） | 关 |
| `funcgraph-retval` | **显示函数返回值**（v6.6 新，需 `CONFIG_FUNCTION_GRAPH_RETVAL`） | 关 |
| `funcgraph-retval-hex` | 返回值十六进制显示 | 关 |
| `sleep-time` | **含睡眠时间**（entry→return 之间被调度出去的时间算进耗时） | **开** |
| `graph-time` | 含嵌套函数时间（关掉则父耗时不含子树，用于算"自耗时"） | 开 |
| `funcgraph-irq` | 中断里发生的调用也画进图 | 开 |
| `funcgraph-proc` | 显示任务名/PID 列 | 关 |

```bash
echo nofuncgraph-irq > $TR/trace_options     # 关掉中断内调用（只看进程上下文）
echo nofuncgraph-sleep-time > $TR/trace_options  # 排除睡眠，只看 CPU 时间
```

> **HFT 杀手组合**：默认 `sleep-time` 开着会把"被调度出去等 CPU"的时间算进函数耗时——你以为内核慢，其实在排队。判读延迟问题时先 `echo nofuncgraph-sleep-time` 分离 **CPU 时间 vs 调度等待**，再决定往 [runqlat](../../chapter-13-perf/) 还是内核路径查。

### 2.5 开销对比

| tracer | 每函数开销（量级） | 说明 |
|---|---|---|
| `function` | ~100–500 ns | 写一条 entry 事件 |
| `function_graph` | ~1–2 µs | entry + shadow 栈操作 + exit 事件 ×2 |

都远大于 nop（<1 ns）。**graph 必须配 `set_graph_function` 收窄根节点**——不设则全内核画图，系统直接跪。

---

## HFT / 嵌入式关联

| 场景 | 用法 |
|---|---|
| 内核收包路径慢 | `set_graph_function = tcp_v4_rcv`（或 udp_rcv）→ 逐层看 IP/TCP 处理、NAPI softirq 里谁耗时——对照 [12-kernel-networking](../../../12-kernel-networking/) 的路径图 |
| syscall 尾延迟归因 | graph 根设为目标 syscall 入口（如 `__x64_sys_recvmmsg`），配合 `nofuncgraph-sleep-time` 分离排队时间 |
| 对比 DPDK 旁路价值 | 把内核收包全路径的 graph 总耗时画出来，就是 DPDK 省掉的那部分（→ [13-dpdk](../../../13-dpdk/)） |
| 调度链审计 | `set_graph_function = __schedule`，看每次切换的代价分布 |
| 嵌入式弱机 | graph 开销可控（窄根 + 短窗），比采样 profiler 语义更确定 |

---

## 衔接

- 上一节：[tracefs 与动态插桩机制](./section-14.1-14.2-核心能力与-tracefs.md)
- 下一节：[事件源、Filter 与 Hist Triggers](./section-14.5-14.714.10-事件源Filter-与-Hist-Triggers.md)——不需要 text patching 的轻量观测面
- 实战案例：[ch16 动态追踪环节的判读法](../../chapter-16-case-studies/notes/section-16.1.7-16.1.8-动态追踪与结论.md)

---

## 代码自测

<details><summary>Q1：function_graph 是怎么知道函数"返回了"的？__fentry__ 只在函数开头啊。</summary>

入口时把函数真实返回地址偷换成 `return_to_handler` 并压入 per-task shadow 栈；CPU 执行 `ret` 时跳进 handler，弹出原返回地址恢复执行，同时拿到入口时间戳算耗时。shadow 栈深度上限 50（`FTRACE_RETFUNC_DEPTH`，ftrace.h:1089）。
</details>

<details><summary>Q2：graph 输出里一个函数显示 2 ms，但 CPU 时间只占 3 µs，发生了什么？</summary>

默认 `sleep-time` 开：耗时含"被调度出去"的等待。关掉该选项后数字会掉到 CPU 时间。这是"内核慢 vs 排队慢"的一刀切分法。
</details>

<details><summary>Q3：`set_ftrace_filter` 和 `set_graph_function` 有什么区别？</summary>

前者管 `function` tracer（决定哪些**函数入口**记事件）；后者管 `function_graph` 的**根节点**（从哪些函数开始画子树，图内的所有下层调用自动收录）。graph 的正确姿势是用后者，而不是给 function filter 塞一堆函数。
</details>

<details><summary>Q4：为什么 graph tracer 比 function tracer 慢好几倍？</summary>

每函数两次事件（entry+exit）+ shadow 栈的压弹开销 + 出口还要查时间差。入口还有返回地址替换的间接跳转成本。
</details>

<details><summary>Q5：`available_filter_functions` 里找不到 `tcp_v4_rcv`，可能是什么原因？</summary>

① 函数被 `notrace`/`noinstr` 标记（如 entry code、NMI 路径）；② 所在模块未加载；③ 内核编译没开 `CONFIG_FUNCTION_TRACER`；④ 拼写/符号版本差异（用 `grep -i tcp_rcv` 模糊查确认）。
</details>
