# 现代内核调试与观测工具 — eBPF / ftrace / drgn / crash

> **对标旧书:** LKD3 Ch18 (基于 2.6 的 printk/Oops/kgdb)
> **现代替代:** ftrace (2.6+/5.x 增强)、eBPF (4.x+/5.10+ 成熟)、crash + kdump、drgn
> **核心变化:** LKD3 时代的内核调试以 printk + kgdb 为主；现代内核调试以**动态追踪** (ftrace/eBPF) 为主，printk 退居二线

---

## 核心观点

LKD3 Ch18 讲的调试手段（printk、Oops、kgdb、SysRq）在 6.x 内核仍然存在，但**不是主力**了。现代内核调试的核心转变是：

| LKD3 时代 (2.6) | 现代内核 (5.x/6.x) |
|-----------------|-------------------|
| printk 是第一手段 | ftrace/eBPF 是第一手段 |
| kgdb 双机调试 | eBPF 生产环境在线调试 |
| Oops + 栈解读 | crash + vmcore + drgn 结构体分析 |
| 手动加 printk | `bpftrace -e 'kprobe:xxx {...}'` 动态插桩 |
| 依赖编译时 CONFIG | 运行时动态启用/禁用 |

**关键：现代工具不需要重新编译内核，运行时即可启用。**

---

## 工具全景

```
静态分析（程序不用跑）
├── nm          符号表
├── readelf     ELF 结构
├── objdump     反汇编
└── crash       vmcore 事后分析

动态追踪（程序运行中）
├── ftrace      内核函数追踪（内置，零依赖）
├── eBPF        可编程追踪（BCC/bpftrace）
├── perf        采样 profiling + 硬件计数器
└── SystemTap   脚本化追踪（Red Hat 系，非主流）

源码级调试
├── gdb + /proc/kcore   只读窥探活内核
├── kgdb                双机远程断点调试
└── kdb                 本地控制台调试

事后分析
├── crash      vmcore 分析（kdump 捕获）
├── drgn       可编程 vmcore 分析（Python 接口）
└── dmesg      环形日志缓冲
```

---

## ftrace：内核内置动态追踪

ftrace (Function Tracer) 是内核内置的追踪框架，不需要安装任何用户态工具，通过 `/sys/kernel/debug/tracing/`（或 `/sys/kernel/tracing/`）接口操作。

### 6.x 相比 2.6 的增强

| 特性 | 2.6 (LKD3 时代) | 6.x |
|------|-----------------|-----|
| tracer 种类 | function, sched_switch | function, function_graph, wakeup, hwlat, irqsoff, preemptoff, wakeup_rt |
| 事件追踪 | 少量 tracepoint | 数百个 tracepoint（sched/irq/net/block/syscalls） |
| 过滤 | 基本函数过滤 | per-event filter、触发器 (trigger) |
| 实例 | 全局唯一 | per-instance (`trace_instances`) |
| BPF 集成 | 无 | ftrace event 可被 BPF 程序消费 |
| histogram | 无 | `hist` trigger（直方图统计） |

### 常用操作

```bash
# 1. 查看可用 tracer
cat /sys/kernel/tracing/available_tracers
# hwlat blk function_graph wakeup_rt wakeup function nocount ...

# 2. 追踪特定函数的调用者
echo function > /sys/kernel/tracing/current_tracer
echo ixgbe_xmit_frame > /sys/kernel/tracing/set_ftrace_filter
echo 1 > /sys/kernel/tracing/tracing_on
cat /sys/kernel/tracing/trace

# 3. 函数调用图（含子函数+执行时间）
echo function_graph > /sys/kernel/tracing/current_tracer
echo ixgbe_xmit_frame > /sys/kernel/tracing/set_graph_function
cat /sys/kernel/tracing/trace
# 输出示例：
# 1) ! 283.412 us  |  ixgbe_xmit_frame() {
# 2)   2.130 us    |    netdev_tx_sent_queue();
# 3) ! 285.631 us  |  }

# 4. 追踪调度切换事件
echo sched:sched_switch > /sys/kernel/tracing/set_event
echo 1 > /sys/kernel/tracing/tracing_on
cat /sys/kernel/tracing/trace_pipe

# 5. 中断关闭延迟（HFT 关键指标）
echo irqsoff > /sys/kernel/tracing/current_tracer
cat /sys/kernel/tracing/tracing_max_latency
# 如果 > 100μs 说明有中断被关闭太久

# 6. 直方图统计（6.x 新增）
echo 'hist:keys=common_pid:vals=hitcount' > /sys/kernel/tracing/events/sched/sched_switch/trigger
cat /sys/kernel/tracing/events/sched/sched_switch/hist
```

### HFT 关键 tracer

| Tracer | 追踪什么 | HFT 用途 |
|--------|----------|----------|
| `wakeup` | 进程唤醒到被调度的延迟 | 交易线程被唤醒后多久开始跑 |
| `wakeup_rt` | 实时线程唤醒延迟 | SCHED_FIFO 交易线程的调度延迟 |
| `irqsoff` | 中断关闭时长 | 关中断太久导致丢包/延迟 |
| `preemptoff` | 抢占关闭时长 | 不可抢占区段导致调度延迟 |
| `function_graph` | 函数调用链+耗时 | 驱动/内核热路径耗时分析 |
| `hwlat` | 硬件延迟（SMI 干扰） | BIOS/SMI 偷走 CPU 时间 |

> **HFT 生产必备：** `wakeup_rt` + `irqsoff` 是交易系统延迟排查的两大法宝。`wakeup_rt > 50μs` 说明调度器有问题；`irqsoff > 100μs` 说明关中断太久。

---

## eBPF：可编程动态追踪

eBPF (extended Berkeley Packet Filter) 是现代内核最重要的观测技术——可以写 C 代码（受限），编译成 BPF 字节码注入内核执行。

### 6.x 相比 4.x 的增强

| 特性 | 4.x | 6.x |
|------|-----|-----|
| BTF (BPF Type Format) | 实验性 | 默认开启（CONFIG_DEBUG_INFO_BTF=y） |
| bpf_loop() | 无 | 有（循环不用宏展开） |
| kfunc | 无 | 有（内核函数调用） |
| bpf_timer | 无 | 有（定时器） |
|ringbuf | 无 | 有（替代 perfbuf，性能更好） |
| CO-RE (Compile Once Run Everywhere) | 实验性 | 成熟（libbpf 支持跨内核版本） |

### 工具生态

| 层次 | 工具 | 说明 |
|------|------|------|
| **底层** | libbpf | C 库，写 BPF 程序 |
| **高级语言** | bpftrace | 一行命令/脚本追踪 |
| **工具集** | BCC (BPF Compiler Collection) | 100+ 现成工具 |
| **可视化** | perfetto / Grafana | 追踪数据可视化 |

### bpftrace 快速上手

```bash
# 1. 统计 write() 调用次数（按进程分组）
bpftrace -e 'tracepoint:syscalls:sys_enter_write { @[comm] = count(); }'

# 2. 追踪 TCP 接收延迟
bpftrace -e 'kprobe:tcp_recvmsg { @start[tid] = nsecs; }
             kretprobe:tcp_recvmsg /@start[tid]/ {
               @latency_us = hist((nsecs - @start[tid]) / 1000);
               delete(@start[tid]);
             }'

# 3. 追踪网卡发包延迟
bpftrace -e 'kprobe:ixgbe_xmit_frame { @start[tid] = nsecs; }
             kretprobe:ixgbe_xmit_frame /@start[tid]/ {
               @us = hist((nsecs - @start[tid]) / 1000);
               delete(@start[tid]);
             }'

# 4. 追踪进程调度延迟
bpftrace -e 'tracepoint:sched:sched_wakeup { @wake[args->pid] = nsecs; }
             tracepoint:sched:sched_switch /@wake[args->next_pid]/ {
               @sched_lat_us = hist((nsecs - @wake[args->next_pid]) / 1000);
               delete(@wake[args->next_pid]);
             }'

# 5. 追踪内核函数参数
bpftrace -e 'kprobe:dev_queue_xmit { printf("dev=%s len=%d\n", 
               ((struct sk_buff *)arg0)->dev->name,
               ((struct sk_buff *)arg0)->len); }'
```

### BCC 工具集（HFT 常用）

| 工具 | 用途 | HFT 场景 |
|------|------|----------|
| `runqlat` | 运行队列延迟分布 | 交易线程等待 CPU 的时间 |
| `runqslower` | 超过阈值的调度延迟 | 捕获 > 100μs 的调度延迟事件 |
| `biosnoop` | 块 I/O 延迟 | 磁盘 I/O 影响交易线程 |
| `tcplife` | TCP 连接生命周期 | 交易连接的建立/持续时间 |
| `tcpdrop` | TCP 丢包 | 行情数据丢包 |
| `hardirqs` | 硬中断分布 | 网卡中断占用 CPU 时间 |
| `softirqs` | 软中断分布 | NET_RX 软中断处理时间 |
| `cpudist` | CPU on-CPU 时间分布 | 交易线程每次上 CPU 跑多久 |
| `offcputime` | 离开 CPU 的原因+时长 | 交易线程为什么被切走 |
| `funclatency` | 指定函数延迟分布 | 内核热路径函数耗时 |

```bash
# HFT 延迟排查经典命令组合
# 1. 调度延迟
runqlat -P $(pidof trading_engine) 1

# 2. 超过 50μs 的调度延迟事件
runqslower -P 50 $(pidof trading_engine)

# 3. 网卡发包延迟
funclatency -m ixgbe_xmit_frame

# 4. 中断分布
hardirqs -d 1

# 5. 交易线程离开 CPU 原因
offcputime -p $(pidof trading_engine) 5
```

---

## crash 工具：vmcore 事后分析

### 现代 crash 相比 LKD3 时代

| 特性 | LKD3 时代 | 现代 |
|------|-----------|------|
| vmcore 获取 | 手动配置 | kdump 自动捕获 |
| 内核符号 | kallsyms | kallsyms + BTF + 调试符号 |
| 结构体解析 | 需要调试符号 | BTF 支持（不需要 -g） |
| 扩展模块 | 无 | crash-eppic 脚本、crash-ext |

### 关键命令

```bash
crash vmlinux /var/crash/2026-08-14/vmcore

# 基础
crash> bt                 # 崩溃线程栈
crash> bt -a              # 所有 CPU 的栈
crash> ps                 # 进程列表
crash> runq               # 运行队列

# 内存
crash> kmem -i            # 内核内存概况
crash> kmem -s            # SLUB 缓存统计
crash> vm <pid>           # 进程地址空间

# 结构体
crash> struct task_struct ffff888012345678  # 查看指定 task_struct
crash> struct -o task_struct  # 看结构体偏移
crash> list task_struct.tasks  # 遍历链表

# 反汇编
crash> dis -r ffffffff81234567  # 反汇编+寄存器
crash> dis ffffffff81234567 20  # 反汇编 20 条指令

# 设备/网络
crash> dev                # 设备列表
crash> net                # 网络接口
crash> search -t "ixgbe"  # 搜索内存中的字符串
```

---

## drgn：可编程 vmcore 分析

drgn 是一个 Python 库，可以**编程式**分析 vmcore（比 crash 的交互式命令更灵活）。

```python
# analyze_crash.py
from drgn import Object, cast
import drgn

prog = drgn.Program()
prog.set_core_dump('/var/crash/2026-08-14/vmcore')

# 遍历所有进程
for task in prog.for_each_task():
    print(f"PID={task.pid.value_()} comm={task.comm.string_().decode()}")

# 查看崩溃进程的调度统计
current = prog.crashed_thread()
print(f"Crashed: PID={current.pid.value_()}")

# 查看网络接口
for netdev in prog.for_each_net_device():
    print(f"{netdev.name.string_().decode()}: tx_packets={netdev.stats.tx_packets}")

# 查看运行队列
for cpu in range(prog['nr_cpu_ids'].value_()):
    rq = prog['runqueues'].per_cpu(cpu)
    cfs = rq.cfs
    print(f"CPU{cpu}: nr_running={cfs.nr_running.value_()}")
```

| 对比 | crash | drgn |
|------|-------|------|
| 使用方式 | 交互式命令 | Python 脚本 |
| 适合 | 快速查看 | 复杂分析、批量处理 |
| 灵活性 | 固定命令集 | 任意 Python 逻辑 |
| 依赖 | 调试符号 | BTF 或调试符号 |
| HFT 场景 | 快速定位崩溃点 | 分析全部交易线程状态 |

---

## 与旧书差异

| LKD3 Ch18 讲的 | 6.x 现代实现 | 差异 |
|----------------|-------------|------|
| printk 是第一调试手段 | ftrace/eBPF 是第一手段 | printk 退居二线 |
| kgdb 双机调试 | eBPF 在线调试 | 不需要第二台机器 |
| Oops 栈手动解读 | crash + BTF 自动解析结构体 | 不需要手动查 System.map |
| 无 BPF 概念 | eBPF 是核心观测工具 | LKD3 完全没提 |
| 无 ftrace function_graph | ftrace 是内置基础设施 | LKD3 时代 ftrace 刚出现 |
| git bisect | 仍然有效 | 不变 |

---

## HFT 关联

| 场景 | 工具 | 操作 |
|------|------|------|
| **交易延迟毛刺** | ftrace wakeup_rt | 测量 SCHED_FIFO 线程唤醒到运行的时间 |
| **中断干扰** | ftrace irqsoff | 测量关中断时长 |
| **网卡发包慢** | bpftrace kprobe/kretprobe | ixgbe_xmit_frame 入退时间差直方图 |
| **调度延迟事件** | BCC runqslower | 捕获 > 50μs 的调度延迟事件 |
| **内核 panic** | crash + kdump | 分析 vmcore，bt 看崩溃栈 |
| **内存泄漏** | crash kmem -s | SLUB 缓存分配统计 |
| **批量分析** | drgn | Python 脚本遍历所有线程状态 |
| **CPU 热点** | perf record + report | 采样 profiling |

> **HFT 生产原则：** ① ftrace/eBPF 是生产环境安全的（零开销或极低开销）。② kgdb/gdb+kcore 尽量避免在生产环境使用（会暂停内核）。③ crash 只用于事后分析 vmcore。④ perf record 有少量开销，短时间采样可以。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** ftrace 和 eBPF 的核心区别是什么？HFT 优先用哪个？

> ftrace 是内核内置的**预定义**追踪框架（function/function_graph/event），不需要安装用户态工具但功能固定。eBPF 是**可编程**的——可以写 C 代码编译成 BPF 字节码注入内核执行，支持任意逻辑（条件判断、聚合统计、直方图）。HFT 优先用 eBPF（更灵活，有 BCC 工具集），但如果只是快速看函数调用链，ftrace function_graph 更简单（不需要装 bpftrace）。

**Q2:** LKD3 说的 kgdb 在现代内核调试中还重要吗？

> kgdb 仍然存在但不是主力了。现代内核调试以 ftrace/eBPF 动态追踪为主（不需要暂停内核、生产可用）。kgdb 主要用于：① 驱动开发阶段的源码级断点调试（实验室环境）。② 启动早期调试（eBPF/ftrace 还没初始化时）。③ 硬件相关问题（需要看寄存器状态）。日常内核调试和性能分析用 eBPF/ftrace 就够了。

**Q3:** 交易系统出现间歇性 100μs 延迟毛刺，排查流程是什么？

> ① `ftrace wakeup_rt` 确认是否调度延迟。② `ftrace irqsoff` 确认是否关中断太久。③ `BCC runqslower 50` 捕获具体延迟事件。④ `bpftrace` 追踪网卡驱动 `ixgbe_xmit_frame` 延迟。⑤ `BCC offcputime` 看交易线程离开 CPU 的原因。⑥ 如果是内核 panic 导致的，看 `/var/crash/` 的 vmcore。

**Q4:** drgn 相比 crash 的优势是什么？

> drgn 是 Python 库，可以编程式分析 vmcore——写脚本批量处理、复杂条件过滤、跨多个 vmcore 对比分析。crash 是交互式命令行，适合快速查看但难以做复杂分析。例如：分析 100 个交易线程的状态，crash 要手动逐个 `struct task_struct`，drgn 一行 `for task in prog.for_each_task()` 就完成。

</details>
