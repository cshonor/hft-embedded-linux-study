# 14.1–14.2 核心能力与 tracefs

> [章节导航](../README.md) · 上一节：— · 下一节：[14.3–14.4、14.8 函数追踪与 function_graph](./section-14.3-14.414.8-函数追踪与-function_graph.md)

## 本节讲什么

Ftrace 的「核心能力地图」和它的用户态接口 tracefs。重点不是罗列命令——而是回答三个问题：

1. Ftrace **到底是什么**（一个框架，不是一个 tracer）
2. 它在**内核里怎么做到**零安装、近乎零闲置开销（动态插桩 text patching）
3. tracefs 的文件模型为什么长这样（per-CPU ring buffer + 快照读/流式读两种语义）

---

## 1. Ftrace 是什么：从 Function Tracer 到框架

| 演进阶段 | 说明 |
|---|---|
| 起源（2.6.27） | **Function Tracer**——只做一件事：记录内核函数调用序列 |
| 现状 | **多追踪器框架**：`function`、`function_graph`、`hwlat`、tracepoint/kprobe/uprobe 事件、hist trigger 全挂在同一套 tracefs + ring buffer 上 |
| 定位变化 | 它成了内核的**观测基础设施**——perf tracepoint、BPF kprobe/uprobe、livepatch 底层都复用同一套插桩点 |

**关键认知**：`echo function_graph > current_tracer` 里的"tracer"只是框架里的一个插件。事件（tracepoint）体系甚至不走 `current_tracer`——设成 `nop` 也能收事件。这是初学者最常见的混淆点。

## 2. 内核里怎么实现（v6.6 源码级）

这是本章值得深挖的部分——Ftrace 的「零闲置开销」是**动态代码修补**换来的。

### 2.1 编译期：每个函数开头埋一个调用点

```bash
# 内核编译配置开启：
CONFIG_FUNCTION_TRACER=y     # 打开 -pg / -mfentry
CONFIG_DYNAMIC_FTRACE=y      # 动态修补（几乎所有发行版都开）
```

- x86_64 用 `-mfentry`：编译器在**每个函数第一行**插 `call __fentry__`（5 字节）
- ARM64 用 `-pg` + patchable-function-entry：等价机制
- 所有调用点地址被链接器收集进 `__mcount_loc` section

### 2.2 启动期：全部打成 NOP

`ftrace_init()` 扫描 `__mcount_loc`，为每个调用点建一个 `struct dyn_ftrace` 记录（ftrace.c 的 `ftrace_process_locs`），然后**把所有 `call __fentry__` 改写成 5 字节 `nop`**。

结果：**没有 tracer 启用时，每个内核函数只多执行一条 nop**——这就是"闲置近零开销"的来源。

### 2.3 启用 tracer：stop_machine 批量换指令

写 `current_tracer` 触发的事件链（v6.6 锚点）：

| 步骤 | 函数 | v6.6 位置 |
|---|---|---|
| ① 准备 | `ftrace_arch_code_modify_prepare()`——拿 `text_mutex`、令 CoreSight/RO 页表可写 | ftrace.c:2841 |
| ② 批量 patch | `ftrace_modify_all_code()`——按 filter 把选中调用点的 nop 换回 `call ftrace_regs_caller` | ftrace.c:2866 |
| ③ 同步 | 没有静态调用点时的兜底：`ftrace_run_stop_machine()` → `stop_machine(__ftrace_modify_code…)` 全核停下换指令 | ftrace.c:2930–2932 |
| ④ 状态记录 | `ftrace_check_record()` 维护每个 `dyn_ftrace` 的 enabled/disabled 标志（`FTRACE_FL_DISABLED`） | ftrace.c:2183 |

### 2.4 ftrace_ops 多路复用

运行时回调不是直接绑 tracer——而是一个 `ftrace_ops` 链表（ftrace.c:124 `ftrace_ops_list`）。`perf probe`、BPF kprobe、livepatch、function tracer 各注册一个 ops，每个 ops 带自己的 filter hash。这就是「BPF 复用 Ftrace 插桩点」的机制根源（→ [Ch 15 BPF](../../chapter-15-bpf/)）。

> **HFT 视角**：`stop_machine` 换指令本身是**全核停顿**（毫秒级）——在生产热路径机器上启用/关闭 function tracer 属于"自己给自己注入延迟尖刺"。开窗口要短，开关动作要避开交易时段。

## 3. tracefs 接口

挂载点（发行版二选一，内容相同）：

```
/sys/kernel/debug/tracing   # 老式：挂在 debugfs 下
/sys/kernel/tracing         # 新式：独立 tracefs（v4.1+，推荐）
```

### 3.1 核心文件清单（v6.6 注册位置）

| 文件 | 作用 | v6.6 注册锚点 |
|---|---|---|
| `available_tracers` | 列出可用 tracer | trace.c:9768 附近批量创建 |
| `current_tracer` | 选择 tracer | trace.c:9768 |
| `tracing_on` | 总开关 0/1（**buffer 全局闸门**） | trace.c |
| `trace` | **快照读** buffer（读完不消耗，`ls` 式） | trace.c |
| `trace_pipe` | **流式读**（阻塞、读走即消耗） | trace.c:9780（全局）/ :8918（per-CPU） |
| `buffer_size_kb` | ring buffer 每 CPU 大小 | trace.c:9783 |
| `snapshot` | 对 max buffer 做快照交换 | trace.c:8935 |
| `tracing_max_latency` | 记录到的最大延迟（irqoff/preemptoff/hwlat 用） | trace.c:1773 |
| `set_ftrace_filter` | function tracer 只追这些函数 | ftrace.c:6414 一带 |
| `events/` | tracepoint/kprobe/uprobe 事件树 | → [14.5–14.7](./section-14.5-14.714.10-事件源Filter-与-Hist-Triggers.md) |

### 3.2 `trace` vs `trace_pipe`——两种读语义

| | `cat trace` | `cat trace_pipe` |
|---|---|---|
| 读后行为 | 内容**保留**，下次读还是全部 | 读走即**消费**，流式 |
| 阻塞性 | 立即返回 | 无数据时阻塞（`tail -f` 式） |
| 开销 | 大 buffer 时反复全量拷贝 | 逐条搬，适合管道处理 |
| 典型用法 | 事后 dump 分析 | `trace_pipe \| grep …` 实时观察 |

**实现根源**：两者读的是同一块 per-CPU ring buffer，差别只在读指针是否推进（消费型读会更新 `buffer` 的 tail 指针）。per-CPU 意味着**无跨 CPU 锁**——这是 Ftrace 高事件率下仍可用的根本原因（对照 [06-linux-mm](../../../06-linux-mm/) 里 per-CPU pageset 的同款思路：能 per-CPU 就不共享）。

### 3.3 上手六步

```bash
TR=/sys/kernel/tracing        # 或 /sys/kernel/debug/tracing

cat $TR/available_tracers     # ① 看有什么：nop function function_graph wakeup ... hwlat
cat $TR/current_tracer        # ② 当前是 nop
echo function_graph > $TR/current_tracer   # ③ 选 tracer（下一节）
echo 1 > $TR/tracing_on       # ④ 开总闸
# ... 产生负载 ...
echo 0 > $TR/tracing_on       # ⑤ 关总闸（先关再读，防止边读边写）
cat $TR/trace | head -50      # ⑥ 读 buffer
```

> `current_tracer` 与 `tracing_on` 是两级开关：tracer 决定**写什么**，`tracing_on` 决定**写不写**。排查时常见错误：`tracing_on` 默认是 0，忘了开导致 `trace` 文件空的。

**权限**：root 或 `CAP_SYS_ADMIN`（可配 `tracefs` gid 让特定组只读）。生产环境纪律：**限时长、限 filter、限 buffer**——见 [ch16 案例纪律](../../chapter-16-case-studies/notes/section-16.0-案例背景An-Unexplained-Win.md)。

---

## HFT / 嵌入式关联

| 场景 | 用法 |
|---|---|
| 交易机不许装 BCC/bpftrace（供应链/合规） | tracefs 是**零依赖**观测面——`echo` + `cat` 就是全部客户端 |
| 嵌入式/救援环境（initramfs） | 同上，busybox 就够 |
| 启用 tracer 的瞬间抖动 | `stop_machine` patch 是毫秒级全核停顿——**避开交易时段**开关 |
| buffer 尺寸规划 | `buffer_size_kb` 按 CPU 配；事件风暴时环形覆盖最老数据，**不 backpressure 生产路径**（丢日志不丢交易） |
| 与 BPF 的关系 | BPF kprobe 底层就是 ftrace_ops——[06.7](../../../06.7-bpf-observability/) 学到 kprobe 时回看本节的 ops 链表 |

---

## 衔接

- 下一节：[14.3–14.4、14.8 函数追踪与 function_graph](./section-14.3-14.414.8-函数追踪与-function_graph.md)——function/function_graph 两个 tracer 的实操与输出精读
- Ch 4 [Ftrace 在工具链中的位置](../../chapter-04-observability-tools/)
- Ch 13 [perf 与 tracepoint 关系](../../chapter-13-perf/)

---

## 代码自测

<details><summary>Q1：为什么 Ftrace 未启用时几乎零开销？这个性质是谁给的？</summary>

`CONFIG_DYNAMIC_FTRACE` 的启动期处理：`ftrace_process_locs` 把每个函数开头的 `call __fentry__` 改写成 5 字节 nop。代价是一条 nop 指令（<1 cycle）。这个性质是**动态修补**给的——没有 `CONFIG_DYNAMIC_FTRACE` 的老内核，`-pg` 会在所有函数里留一个真实调用，闲置开销显著。
</details>

<details><summary>Q2：切换 tracer 时内核做了什么危险的事？为什么 HFT 机器上要避开交易时段？</summary>

`ftrace_modify_all_code()`（ftrace.c:2866）批量改写内核代码段，必要时经 `stop_machine()`——**所有 CPU 停在自旋等待**直到 patch 完成，量级毫秒。对 P99 敏感的进程这就是一次注入的尾延迟。
</details>

<details><summary>Q3：`trace` 和 `trace_pipe` 读的是同一块 buffer，区别在哪？</summary>

读指针是否推进：`trace` 是快照读（内容保留、可反复读），`trace_pipe` 是消费读（读走释放 ring 空间、可阻塞）。后者适合 `grep` 管道实时处理，前者适合事后 dump。
</details>

<details><summary>Q4：`current_tracer=nop` 时 events/ 下的事件还有输出吗？</summary>

有。tracepoint 事件体系不依赖 `current_tracer`——`nop` 恰恰是"只用事件、不用函数 tracer"的标准姿势。`tracing_on` 和 `events/.../enable` 才是事件的两级开关。
</details>

<details><summary>Q5：为什么 ring buffer 设计成 per-CPU？</summary>

消除跨 CPU 写锁竞争。高事件率下单 CPU 只写自己的 buffer，读取端（用户态 cat）天然按 CPU 序切分。这是内核 per-CPU 数据结构的一贯模式（同款思想见 buddy pcp / SLUB cpu_slab）。
</details>
