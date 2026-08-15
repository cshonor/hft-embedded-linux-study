## ⑥ 内核调试器

内核调试比用户态难得多：没有便利的 gdb 断点、一个空指针就 panic 整个系统、中断上下文不能睡眠。本节梳理内核调试工具的**能力边界**和**实际工作流**。

---

### 工具能力对比

| 工具 | 类型 | 能做什么 | 不能做什么 | 适用场景 |
|------|------|----------|-----------|----------|
| **gdb + /proc/kcore** | 只读窥探 | 查看运行中内核内存、结构体、链表 | 断点、单步、修改数据 | 线上排查，看数据结构状态 |
| **kgdb** | 远程调试 | 完整源码级调试：断点、单步、改变量 | 需要双机 + 串口 | 驱动开发、启动早期 |
| **kdb** | 本地调试 | 查看内存、寄存器、栈、断点 | 无源码级调试 | 紧急排障、无第二台机器 |
| **crash** | core 分析 | 分析 vmcore/kdump、反汇编、结构体 | 不能调试活系统 | 事后分析 panic/oops |
| **ftrace** | 动态追踪 | 函数调用链、延迟、事件 | 不能看变量值 | 性能分析、调用路径 |
| **eBPF** | 动态追踪 | 可编程追踪、过滤、统计 | 需要内核支持(4.x+) | 深度观测、性能热点 |

---

### gdb + /proc/kcore

`/proc/kcore` 是内核导出的**虚拟 ELF core dump**，gdb 可以像调试 core 文件一样查看运行中的内核。

```bash
# 1. 确认 kcore 存在
ls -l /proc/kcore          # 约 128TB（虚拟大小 = 物理内存映射）

# 2. 用 gdb 读取内核符号表 + kcore
gdb vmlinux /proc/kcore

# 3. 常用操作
(gdb) p init_task           # 查看 init 进程的 task_struct
(gdb) p *current            # 当前运行进程（需要内核符号）
(gdb) p init_task.tasks     # 遍历进程链表
(gdb) p jiffies             # 查看 jiffies 计数器
(gdb) info variables runqueue  # 查找全局变量
```

**关键限制：**

| 限制 | 原因 |
|------|------|
| 不能断点 | 没有暂停内核执行的机制 |
| 不能单步 | 同上 |
| 不能修改数据 | kcore 是只读映射 |
| 可能看到不一致数据 | 内核在运行，数据随时变化 |

> **HFT 场景：** 交易系统卡顿但没 crash 时，用 `gdb vmlinux /proc/kcore` 看 `runqueue` 负载、`softirq` 计数、网卡驱动私有数据结构——不需要停机。

---

### kgdb：双机远程调试

kgdb 让内核变成 gdb 的**远程目标**，通过串口或网络连接，支持完整源码级调试。

```
┌──────────────┐    串口/网线    ┌──────────────┐
│  开发机       │ ◄──────────► │  目标机       │
│  gdb vmlinux  │               │  kgdboc=ttyS0 │
│  target remote │               │  内核运行中    │
└──────────────┘               └──────────────┘
```

**目标机配置：**

```bash
# 1. 内核编译开启 KGDB
# CONFIG_KGDB=y
# CONFIG_KGDB_SERIAL_CONSOLE=y
# CONFIG_DEBUG_INFO=y
# CONFIG_FRAME_POINTER=y

# 2. 启动参数添加 kgdb 等待
# /etc/default/grub:
# GRUB_CMDLINE_LINUX="kgdboc=ttyS0,115200 kgdbwait"

# 3. 运行时激活
echo ttyS0 > /sys/module/kgdboc/parameters/kgdboc

# 4. 触发断点（目标机进入 kgdb 暂停）
echo g > /proc/sysrq-trigger
```

**开发机连接：**

```bash
gdb vmlinux
(gdb) target remote /dev/ttyS0
(gdb) break my_driver_probe
(gdb) continue
# 目标机恢复运行，命中断点时自动暂停
(gdb) next
(gdb) print dev->name
(gdb) continue
```

| 优势 | 劣势 |
|------|------|
| 源码级断点/单步/变量查看 | 需要物理串口/网线 + 第二台机器 |
| 可以调试启动早期 | 暂停整个内核（生产环境不可用） |
| 和用户态 gdb 体验一致 | 某些架构支持不完善 |

> **HFT 开发阶段：** 用 kgdb 调试定制网卡驱动——在 `ixgbe_xmit_frame` 设断点，逐步追踪发包路径，看 DMA 描述符链表状态。**生产环境禁止使用**（会冻结整个内核）。

---

### kdb：本地控制台调试器

kdb 是内核内置的调试器，不需要第二台机器，直接在目标机控制台操作。

```bash
# 进入 kdb
echo g > /proc/sysrq-trigger

# kdb 命令
kdb> bp my_function      # 设断点
kdb> go                  # 继续
kdb> rd                  # 查看寄存器
kdb> bt                  # 栈回溯
kdb> md 0xffffffff12345678  # 查看内存
kdb> ps                  # 查看进程
```

| 对比 | kgdb | kdb |
|------|------|-----|
| 连接方式 | 双机串口 | 本地控制台 |
| 源码级调试 | 支持 | 不支持 |
| 变量查看 | 支持 | 仅内存地址 |
| 紧急排障 | 不方便 | 快速可用 |
| HFT 场景 | 开发阶段 | 生产紧急 |

---

### crash 工具：事后分析 vmcore

当内核 panic 后（配 kdump），会生成 vmcore 文件。`crash` 是分析 vmcore 的标准工具。

```bash
# 1. 配置 kdump
# CONFIG_KDUMP=y / CONFIG_CRASH_DUMP=y
# /etc/default/grub: GRUB_CMDLINE_LINUX="crashkernel=256M"

# 2. panic 触发后分析
crash vmlinux /var/crash/2026-08-14/vmcore

# 3. 常用命令
crash> bt                 # 崩溃线程栈回溯
crash> ps                 # 所有进程
crash> runq               # 运行队列
crash> kmem -i            # 内核内存概况
crash> struct task_struct ffff888012345678  # 查看指定结构体
crash> dis -r ffffffff81234567  # 反汇编+寄存器
crash> dev                # 设备列表
crash> net                # 网络接口
```

**HFT panic 分析流程：**

```
panic 发生 → kdump 捕获 vmcore → crash 工具分析
                                        ↓
                               bt 看崩溃栈
                                        ↓
                               struct 看数据结构
                                        ↓
                               dis 反汇编看指令
                                        ↓
                               定位根因（空指针/UAF/死锁）
```

> **HFT 必备：** 交易系统内核 panic 是最严重事故。kdump + crash 是**唯一的事后分析手段**——相当于飞机黑匣子。生产环境必须配置 `crashkernel` 预留内存。

---

### ftrace：函数级动态追踪

ftrace 不需要 gdb，直接通过 `/sys/kernel/debug/tracing/` 接口操作。

```bash
# 1. 追踪某函数的调用者
echo function > /sys/kernel/debug/tracing/current_tracer
echo schedule > /sys/kernel/debug/tracing/set_ftrace_filter
cat /sys/kernel/debug/tracing/trace

# 2. 函数调用图（含子函数）
echo function_graph > /sys/kernel/debug/tracing/current_tracer
echo schedule > /sys/kernel/debug/tracing/set_graph_function

# 3. 追踪调度事件
echo sched:sched_switch > /sys/kernel/debug/tracing/set_event
cat /sys/kernel/debug/tracing/trace_pipe

# 4. 追踪延迟
echo wakeup_rt > /sys/kernel/debug/tracing/current_tracer  # 实时任务唤醒延迟
```

| 优势 | 劣势 |
|------|------|
| 零开销（不追踪时） | 只能看函数名，不能看变量 |
| 不需要断点/暂停 | 输出量大需要过滤 |
| 生产环境可用 | 需要内核编译支持 |

> **HFT 生产排查：** 交易线程延迟毛刺 → `wakeup` tracer 追踪调度延迟 → 看是否被中断/其他线程抢占。ftrace 是**唯一能在生产环境安全使用**的内核追踪工具。

---

### eBPF：可编程动态追踪

eBPF 是 ftrace 的升级版——可以写 C 代码（受限），编译成 BPF 字节码注入内核执行。

```bash
# 1. bpftrace 一行命令
bpftrace -e 'tracepoint:syscalls:sys_enter_write { @[comm] = count(); }'

# 2. 追踪内核函数 + 过滤
bpftrace -e 'kprobe:tcp_recvmsg { printf("%s recv tcp\n", comm); }'

# 3. 追踪延迟
bpftrace -e 'kprobe:ixgbe_xmit_frame { @start[tid] = nsecs; }
             kretprobe:ixgbe_xmit_frame /@start[tid]/ {
               @latency = nsecs - @start[tid];
               delete(@start[tid]);
             }'

# 4. BCC 工具集
biosnoop      # 追踪块 I/O 延迟
tcplife       # 追踪 TCP 连接生命周期
runqlat       # 运行队列延迟分布
```

| 对比 | ftrace | eBPF |
|------|--------|------|
| 编程能力 | 预定义 tracer | 可写 C 代码（BPF 程序） |
| 过滤 | 简单 filter | 任意条件判断 |
| 统计 | 原始 trace | 直方图/聚合统计 |
| 生态 | 内置 | BCC/bpftrace 工具集 |
| 内核版本 | 2.6+ | 4.x+（HFT 建议用 5.10+） |

> **HFT 深度排查：** bpftrace 追踪网卡驱动 `ixgbe_xmit_frame` 的入退时间差 → 发包延迟直方图 → 定位是驱动慢还是队列满。eBPF 是现代 HFT 系统性能分析的**核心工具**。

---

### 调试工具选择决策树

```
问题类型？
│
├─ 崩溃/Oops → 事后分析
│   ├─ 有 vmcore → crash 工具
│   └─ 无 vmcore → dmesg + kallsyms 解析栈
│
├─ 性能问题 → 动态追踪
│   ├─ 简单函数调用 → ftrace function_graph
│   ├─ 延迟/直方图 → eBPF (bpftrace/BCC)
│   └─ 深度分析 → perf + eBPF
│
├─ 数据结构异常 → 活系统窥探
│   ├─ 查看变量/链表 → gdb + /proc/kcore
│   └─ 紧急控制台 → kdb
│
├─ 驱动开发 → 源码级调试
│   ├─ 开发阶段 → kgdb 双机
│   └─ 简单断点 → kdb
│
└─ 回归定位 → git bisect
    └─ 二分提交找到引入 bug 的 commit
```

---

### 与 C 语言笔记的交叉引用

| 本节概念 | C 语言笔记对应 |
|----------|---------------|
| gdb 基本操作 | C 笔记 `05-Embedded/1.5-gdb/1.5.1~1.5.5`（GDB 详解） |
| core dump 分析 | C 笔记 `05-Embedded/1.5-gdb/1.5.4-GDB调试core-dump.md` |
| vmlinux/vmcore ELF 格式 | C 笔记 `02-Pointers-on-C/ch18/18.4~18.9`（ELF 二进制分析） |
| nm 看内核符号 | C 笔记 `02-Pointers-on-C/ch18/18.5-nm符号表查看.md` |
| objdump 反汇编内核 | C 笔记 `02-Pointers-on-C/ch18/18.7-objdump反汇编.md` |
| crash dis 反汇编 | 同上 |

> 内核调试本质是**用户态调试的超集**：gdb/kgdb 操作和用户态一样，但多了 kdb/crash/ftrace/eBPF 等内核特有工具。先掌握 C 笔记里的 GDB 和 ELF 工具，再来这里就只需要理解内核侧的差异。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** kgdb 和 kdb 的区别？HFT 用哪个？

<details><summary>答案</summary>

kgdb：通过串口/网络连接 GDB，支持源码级断点/单步/变量查看（需要两台机器）。kdb：内核内置调试器，在本地控制台操作，功能较弱（无源码级调试）。HFT 开发阶段用 kgdb 调试驱动（断点/变量查看），生产环境用 kdb（紧急排障，不需要第二台机器）。ftrace/eBPF 是更轻量的替代方案。

</details>

**Q2.** /proc/kcore 和 vmcore 有什么区别？

<details><summary>答案</summary>

/proc/kcore 是运行中内核的**虚拟** ELF core dump，gdb 可以实时窥探但只读、不能断点。vmcore 是 panic 后 kdump 捕获的**静态**内存快照，用 crash 工具分析。前者看"活系统"，后者看"尸体"。

</details>

**Q3.** HFT 交易系统出现间歇性延迟毛刺，用哪个工具排查？

<details><summary>答案</summary>

优先级：① ftrace `wakeup`/`wakeup_rt` tracer 看调度延迟（生产安全，零开销）。② eBPF `runqlat` 看运行队列延迟分布。③ eBPF 追踪网卡驱动 `ixgbe_xmit_frame` 延迟。④ `perf record` + `perf report` 看热点。**不用** kgdb（会冻结内核）、**不用** gdb+/proc/kcore（看不到时间维度的信息）。

</details>

**Q4.** 为什么生产环境必须配置 kdump？

<details><summary>答案</summary>

内核 panic 后系统完全不可用，没有 kdump 就只有 dmesg 文字日志（可能不完整）。kdump 预留一段内存，panic 时启动第二个内核捕获完整内存快照（vmcore）。事后用 crash 工具分析 vmcore：bt 看崩溃栈、struct 看数据结构、dis 反汇编——相当于飞机黑匣子。HFT 系统的 panic 根因分析完全依赖 vmcore。

</details>

</details>
---
