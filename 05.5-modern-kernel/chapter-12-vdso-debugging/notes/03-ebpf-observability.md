# eBPF：可编程动态追踪

> 6.x 变化: BTF 默认开启, CO-RE 成熟, bpf_timer, ringbuf
> 对标旧书: LKD3 无 eBPF 概念

---

## eBPF 核心概念

eBPF (extended Berkeley Packet Filter) 可以写 C 代码（受限），编译成 BPF 字节码注入内核执行。不需要重新编译内核，运行时动态启用/禁用。

### 6.x 相比 4.x 的增强

| 特性 | 4.x | 6.x |
|------|-----|-----|
| BTF (BPF Type Format) | 实验性 | 默认开启 (CONFIG_DEBUG_INFO_BTF=y) |
| bpf_loop() | 无 | 有（循环不用宏展开） |
| kfunc | 无 | 有（内核函数调用） |
| bpf_timer | 无 | 有（定时器） |
| ringbuf | 无 | 有（替代 perfbuf，性能更好） |
| CO-RE | 实验性 | 成熟（libbpf 支持跨内核版本） |

### 工具生态

| 层次 | 工具 | 说明 |
|------|------|------|
| **底层** | libbpf | C 库，写 BPF 程序 |
| **高级语言** | bpftrace | 一行命令/脚本追踪 |
| **工具集** | BCC (BPF Compiler Collection) | 100+ 现成工具 |
| **可视化** | perfetto / Grafana | 追踪数据可视化 |

---

## bpftrace 快速上手

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

### bpftrace 语法要点

```
探针类型:
  kprobe:func_name        — 函数入口
  kretprobe:func_name     — 函数返回
  tracepoint:category:event — 内核 tracepoint
  profile:hz:99           — 定时采样
  interval:s:1            — 每秒触发

内置变量:
  comm    — 进程名
  pid/tid — 进程/线程 ID
  nsecs   — 纳秒时间戳
  arg0-argN / args->field — 函数参数
  retval  — 返回值 (kretprobe)

聚合函数:
  count()   — 计数
  hist(x)   — 直方图 (2的幂次方分桶)
  lhist(x, min, max, step) — 线性直方图
  sum(x)    — 求和
  avg(x)    — 平均值
  min(x)/max(x) — 最小/最大值
```

---

## BCC 工具集（HFT 常用）

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

### HFT 延迟排查工作流

```
交易延迟毛刺 100μs+
├── 步骤1: runqslower 50 → 捕获调度延迟事件
│   └── 确认是调度延迟还是其他原因
├── 步骤2: ftrace wakeup_rt → 确认唤醒到运行的时间
│   └── 如果 wakeup_rt 正常 → 不是调度问题
├── 步骤3: ftrace irqsoff → 确认关中断时长
│   └── 如果 irqsoff 正常 → 不是中断问题
├── 步骤4: funclatency ixgbe_xmit_frame → 网卡发包耗时
│   └── 如果正常 → 不是网卡驱动问题
├── 步骤5: offcputime → 交易线程为什么被切走
│   └── 如果显示 sched_switch → 正常调度
└── 步骤6: bpftrace 自定义追踪 → 追踪具体代码路径
```

---

## CO-RE (Compile Once Run Everywhere)

```bash
# 传统 BPF 程序需要为每个内核版本编译 (依赖内核头文件)
# CO-RE 通过 BTF (BPF Type Format) 实现跨内核兼容

# 检查内核是否支持 BTF
ls -la /sys/kernel/btf/vmlinux
# 如果存在, 内核有 BTF 支持

# CO-RE 程序编译
clang -target bpf -O2 -g -c my_bpf.c -o my_bpf.o
# my_bpf.o 可以在不同内核版本上运行 (只要都有 BTF)
```

### libbpf 编程模型

```c
// 1. 定义 BPF 程序 (BPF C)
SEC("kprobe/ixgbe_xmit_frame")
int trace_xmit(struct pt_regs *ctx) {
    u64 ts = bpf_ktime_get_ns();
    u32 pid = bpf_get_current_pid_tgid() >> 32;
    bpf_map_update_elem(&start, &pid, &ts, BPF_ANY);
    return 0;
}

// 2. 用户空间程序 (C + libbpf)
int main() {
    struct bpf_object *obj = bpf_object__open_file("my_bpf.o", NULL);
    bpf_object__load(obj);
    struct bpf_link *link = bpf_program__attach_kprobe(
        bpf_object__find_program_by_name(obj, "trace_xmit"),
        false, "ixgbe_xmit_frame");
    // ... 读取 map 数据 ...
}
```

---

## HFT 关联

| 场景 | 工具 | 操作 |
|------|------|------|
| 交易延迟毛刺 | BCC runqslower | 捕获 > 50μs 的调度延迟事件 |
| 网卡发包慢 | bpftrace kprobe/kretprobe | ixgbe_xmit_frame 入退时间差直方图 |
| 调度延迟事件 | BCC runqslower | 捕获 > 50μs 的调度延迟事件 |
| 中断分布 | BCC hardirqs | 网卡中断占用 CPU 时间统计 |

> **HFT 生产原则：** eBPF 是生产环境安全的（零开销或极低开销）。bpftrace 适合快速追踪，BCC 工具集适合常用场景，libbpf 适合自定义复杂逻辑。

---

## 自测题

<details>
<summary>Q1: ftrace 和 eBPF 的核心区别是什么？HFT 优先用哪个？</summary>

ftrace 是内核内置的**预定义**追踪框架（function/function_graph/event），不需要安装用户态工具但功能固定。eBPF 是**可编程**的——可以写 C 代码编译成 BPF 字节码注入内核执行，支持任意逻辑（条件判断、聚合统计、直方图）。HFT 优先用 eBPF（更灵活，有 BCC 工具集），但如果只是快速看函数调用链，ftrace function_graph 更简单（不需要装 bpftrace）。
</details>

<details>
<summary>Q2: BTF 是什么？为什么 6.x 默认开启它很重要？</summary>

BTF (BPF Type Format) 是内核结构体的类型描述信息，存储在 /sys/kernel/btf/vmlinux 中。有了 BTF，eBPF 程序不需要内核调试符号就能访问结构体字段（CO-RE 技术）。6.x 默认开启 BTF 意味着所有 eBPF 工具（bpftrace/BCC/libbpf）开箱即用，不需要额外安装 debuginfo 包。这对 HFT 生产环境很重要——不需要在生产机器上装内核调试包。
</details>

<details>
<summary>Q3: 交易系统出现间歇性 100μs 延迟毛刺，用 eBPF 排查的流程？</summary>

① `BCC runqslower 50 <pid>` 捕获具体延迟事件，确认是否调度延迟。② `bpftrace` 追踪网卡驱动 `ixgbe_xmit_frame` 延迟，确认是否网卡发包慢。③ `BCC offcputime -p <pid>` 看交易线程离开 CPU 的原因。④ `bpftrace` 追踪特定内核函数的耗时分布。⑤ 如果以上都正常，可能是用户空间代码问题，用 perf record 采样 profiling。
</details>

---

## 交叉引用

- [02-ftrace-modern.md](./02-ftrace-modern.md) — ftrace 内置追踪框架
- [04-crash-drgn-analysis.md](./04-crash-drgn-analysis.md) — crash/drgn 事后分析
- [06.7-bpf-observability](../../../06.7-bpf-observability/) — eBPF 完整教程
