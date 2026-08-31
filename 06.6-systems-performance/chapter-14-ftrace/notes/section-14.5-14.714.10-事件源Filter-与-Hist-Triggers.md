# 14.5–14.7、14.10 事件源、Filter 与 Hist Triggers

> [章节导航](../README.md) · 上一节：[14.3–14.4、14.8 函数追踪与 function_graph](./section-14.3-14.414.8-函数追踪与-function_graph.md) · 下一节：[14.9 硬件延迟检测 hwlat](./section-14.9-硬件延迟检测hwlat.md)

## 本节讲什么

函数追踪是"手术刀"，事件（event）体系才是日常主力：

1. 三种事件源：**tracepoint（静态）/ kprobe（内核动态）/ uprobe（用户态动态）**
2. **filter**：内核里按条件丢弃事件——只送你关心的
3. **trigger + hist trigger**：事件发生时在**内核内**执行动作——直方图聚合是精华

---

## 1. 三种事件源

| | tracepoint | kprobe | uprobe |
|---|---|---|---|
| 本质 | 内核源码里**静态埋点** `trace_xxx()` | 动态在任意内核函数入口下**软件断点** | 动态在用户态二进制偏移下断点 |
| 稳定性 | **ABI 稳定**，字段有承诺 | 随内核版本漂移 | 随二进制版本漂移 |
| 开销 | 低（静态分支，未启用近零） | 中（int3 陷阱 + 单步） | 中（int3 + 页面翻权限） |
| 字段 | 结构化（`format` 文件定义） | 手写取参表达式 | 手写取参表达式 |
| 配置入口 | `events/<sys>/<evt>/enable` | `kprobe_events` 文件 | `uprobe_events` 文件 |

事件目录树（v6.6 锚点：每个事件目录下 `filter`/`trigger`/`enable`/`id` 文件由 trace_events.c:2435–2459 创建，`set_event` 总开关在 :3632）：

```
events/
├── sched/
│   ├── sched_switch/       # 调度切换（字段：prev_comm/pid/prio, next_comm/pid/prio）
│   │   ├── enable  filter  format  id  trigger
│   ├── sched_wakeup/
│   └── ...
├── net/  irq/  timer/  ...
├── kprobes/                # 动态 kprobe 事件（kprobe_events 写入后出现）
└── raw_syscalls/
```

### 1.1 tracepoint：启用与 format

```bash
TR=/sys/kernel/tracing
cat $TR/events/sched/sched_switch/format    # 看字段定义（自动生成）
echo 1 > $TR/events/sched/sched_switch/enable
echo 1 > $TR/tracing_on
cat $TR/trace_pipe                          # 流式观察
```

`format` 文件给出每个字段的**名字、类型、字节偏移**——filter 语法和 hist 的 `keys=/vals=` 全靠它。**查字段先 cat format** 是铁律。

> tracepoint 静态分支未启用时开销是一个 `static_branch`（nop 跳转）——比 function tracer 的动态 patch 还轻。这是**生产可常开**的事件源（`sched_switch`、`raw_syscalls` 属于低风险常开集）。

### 1.2 kprobe：动态插桩任意内核函数

```bash
# p:自定义名:符号+偏移  取参：$retval / 寄存器 / 栈 / @地址
echo 'p:myrecv tcp_v4_rcv skb=%ax:s64' >> $TR/kprobe_events
echo 'r:myrecv_ret tcp_v4_rcv $retval' >> $TR/kprobe_events   # r: 返回探针
echo 1 > $TR/events/kprobes/myrecv/enable
```

kprobe_events 文件注册于 trace_kprobe.c:1909。语法要点：

| 前缀 | 含义 |
|---|---|
| `p` | 入口探针 |
| `r` | 返回探针（`$retval` 取返回值） |
| `-:名字` | 删除事件 |

参数取法（x86_64）：`%ax/%bx/…`（寄存器）、`+0(%ax)`（偏移取内存）、`$stackN`（栈槽）、`sym+offset`。**ABI 敏感**：参数在哪个寄存器随调用约定/内核版本变化——kprobe 事件是"能跑但会漂"的观测点。

### 1.3 uprobe：用户态插桩

```bash
echo 'p:libfoo_free /usr/lib/libfoo.so:0x11230 size=%ax' >> $TR/uprobe_events
```

给**无源码/无重编**的二进制插观测点（第三方库 malloc、闭源采集进程）。注意：uprobe 断点走 int3 + 页表操作，**高热函数上开销明显**——malloc 级别的高频点慎用，优先 BPF uprobe（有批量优化）。

---

## 2. Filter：内核内条件丢弃

```bash
echo 'pid == 4242' > $TR/events/sched/sched_switch/filter
echo 'prev_pid == 4242 || next_pid == 4242' > $TR/events/sched/sched_switch/filter
echo 0 > $TR/events/sched/sched_switch/filter      # 清除
```

字段名来自 `format`。支持 `== != < > <= >= && ||`。**过滤在内核里逐事件求值**（trace_events.c 的 filter 引擎），不合格事件根本不进 ring buffer——比"全收再 grep"省一个数量级的 buffer 带宽。

## 3. Trigger：事件驱动动作

写 `events/<evt>/trigger` 文件，让某事件发生时执行动作：

| 动作 | 语法（简） | 用途 |
|---|---|---|
| 开关追踪 | `traceon` / `traceoff` | 条件触发停止，保住现场 |
| 抓栈 | `stacktrace` if filter | 只给命中条件的抓调用栈 |
| 快照 | `snapshot` if filter | 命中时交换 snapshot buffer |
| 级联使能 | `enable_event:子系统:事件` | 事件 A 发生才开始收 B（缩小窗口） |
| 直方图 | `hist:keys=…:vals=…` | 下节主角 |

例：只在目标 PID 切换时抓栈：

```bash
echo 'stacktrace if next_pid == 4242' > $TR/events/sched/sched_switch/trigger
```

## 4. Hist Triggers：内核内直方图聚合（本章精华）

### 4.1 为什么重要

原始 trace 模式的成本模型：**每事件一次 ring buffer 写 + 一次用户态拷贝**。sched_switch 每秒上万次、syscall 更高——把每条记录送用户态做 `awk` 统计，观测本身就成了负载。

hist trigger 把聚合搬进内核：**每事件只更新一个桶计数**（bucket++），buffer 里零记录，用户态只在最后读一次汇总表。解析入口 `event_hist_trigger_parse()`（trace_events_hist.c:6543）。

### 4.2 语法与实例

```bash
# 按 PID 统计 sched_switch 次数
echo 'hist:keys=next_pid' > $TR/events/sched/sched_switch/trigger
cat $TR/events/sched/sched_switch/hist

# 输出形态（内核内聚合的直方表）：
# next_pid        count
# --------        -----
# 4242            18203
# 12              542
```

进阶语法块：

| 语法 | 作用 |
|---|---|
| `keys=a,b` | 复合键（多维表） |
| `vals=runtime` | 对字段求和（不只计数） |
| `latency=u64($ts)` | 表达式变量 |
| `.usecs` | 时间戳按 µs 显示 |
| `if filter` | 内置过滤 |
| `onmatch(事件)` / synthetic | **跨事件关联**（见下） |
| `sort=…` `size=…` | 排序与桶数 |

### 4.3 合成事件：测"任意两点间延迟"

hist 的杀手锏——把 A 事件的字段**搬运**到 B 事件的直方图里，合成第三个事件：

**例：每个任务的唤醒→上 CPU 延迟（调度延迟）直方图**：

```bash
TR=/sys/kernel/tracing
cd $TR/events/sched

# ① 合成事件定义：wakeup 时记时间戳，sched_switch 时求差
echo 'wakeup_lat u64 lat; pid_t pid' > $TR/synthetic_events
echo 'hist:keys=pid:ts0=common_timestamp.usecs' > sched_wakeup/trigger
echo 'hist:keys=next_pid:wakeup_lat=common_timestamp.usecs-$ts0:\
onmatch(sched.sched_wakeup).wakeup_lat($wakeup_lat,next_pid)' > sched_switch/trigger

# ② 看结果：调度延迟直方图（P50/P99 一目了然）
cat sched_wakeup/hist  # 或对合成事件再套一层 hist
```

`common_timestamp` 是 trace 子系统给每条事件打的内核时戳（`CONFIG_HIST_TRIGGERS` + `CONFIG_TRACER_SNAPSHOT` 相关）。**这是不写一行 C 代码、不装任何 BPF 工具就能拿到的调度延迟直方图**——老内核机器上的救命稻草。

> 对应的 BPF 版本（`runqlat`）在 [Ch 15](../../chapter-15-bpf/) / [06.7](../../../06.7-bpf-observability/)——语义相同、可编程性更强。v6.6 上两者并存：hist 胜在**零依赖 + 已在主线多年**；BPF 胜在**任意聚合逻辑 + map 摘取**。

### 4.4 输出判读

hist 输出自带分位数行（`count`、总观测数 + percentile 列）。判读对照 [ch16 直方图判读纪律](../../chapter-16-case-studies/notes/section-16.0-案例背景An-Unexplained-Win.md)：看**形状**（双峰=两类负载混跑）而非只看均值；尾部桶单独追（filter + stacktrace trigger 抓长尾样本的调用栈）。

---

## HFT / 嵌入式关联

| 场景 | 组合 |
|---|---|
| **调度延迟基线** | 合成事件 wakeup→switch 延迟 hist——上生产前先采一晚，双峰/长尾即告警 |
| 迁移审计 | `keys=prev_cpu,next_cpu` 的 sched_switch hist——热线程是否被频繁迁核（对照 [ch16.1.7 动态追踪](../../chapter-16-case-studies/notes/section-16.1.7-16.1.8-动态追踪与结论.md) 的 runqlat） |
| 长尾抓栈 | `stacktrace if $lat > 50` 式 trigger——只在尾部命中时消耗 buffer |
| 事件窗口压缩 | `enable_event` 级联：异常事件出现才开详追，平时近零开销 |
| 老内核/无 BPF 机器 | tracepoint + hist 是**唯一**内核内聚合手段（[perf-tools](./section-14.11-14.13-前端工具.md) 都只是它的 bash 壳） |
| 采集进程自身审计 | uprobe 插闭源行情解码库的入口/出口，量测其耗时分布 |

---

## 衔接

- 上一节：[function_graph 手术刀](./section-14.3-14.414.8-函数追踪与-function_graph.md)
- 下一节：[hwlat——连事件都看不见的固件级停顿](./section-14.9-硬件延迟检测hwlat.md)
- BPF 对照版：[Ch 15 BPF](../../chapter-15-bpf/)（kprobe/uprobe 同源、聚合更强）
- perf 侧事件消费：[Ch 13 perf trace](../../chapter-13-perf/)

---

## 代码自测

<details><summary>Q1：tracepoint、kprobe、uprobe 三者稳定性怎么排序？为什么？</summary>

tracepoint > uprobe > kprobe。tracepoint 是内核 ABI 承诺的稳定接口（字段名不变）；uprobe 绑定二进制偏移（二进制不变就不变）；kprobe 绑定内核符号与调用约定，内核一升级就可能失效。
</details>

<details><summary>Q2：filter 和用户态 grep 都能筛事件，本质区别是什么？</summary>

filter 在**内核里逐事件求值**，不合格事件不进 ring buffer——省的是 buffer 写 + 拷贝 + 用户态解析三段成本。grep 是全收再筛，观测开销照付全额。
</details>

<details><summary>Q3：hist trigger 为什么能把观测开销压到"每事件一次桶自增"？</summary>

聚合发生在**内核事件回调路径**里：命中一个桶就 `count++`（或 val 累加），不写 ring buffer、无用户态拷贝。读取只发生一次（cat hist 文件读汇总表）。成本从 O(事件×拷贝) 变成 O(事件×自增)。
</details>

<details><summary>Q4：synthetic event 测唤醒延迟为什么需要两个 hist trigger 配合？</summary>

延迟 = 终点时间戳 − 起点时间戳，但起点在事件 A（sched_wakeup）、终点在事件 B（sched_switch），不同事件不能直接相减。第一个 hist 以 PID 为键把 ts0 存进**触发器内的哈希表**；第二个 hist 在 B 事件上 `onmatch(A)` 命中同键时取出 ts0 求差，生成合成事件 wakeup_lat——A 的字段被"搬运"到了 B 的语境。
</details>

<details><summary>Q5：kprobe 事件写了 `skb=%ax`，内核升级后数据变垃圾了，怎么回事？</summary>

`%ax` 是"按当前调用约定第 N 个参数在 RAX"的假设。内核版本变化可能改变参数寄存器分配/函数签名，断点还在原符号上，但取到的寄存器内容已不是你以为的参数。kprobe 事件没有类型契约——升级后必须重验。
</details>
