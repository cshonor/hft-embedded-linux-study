# crash 与 drgn：vmcore 事后分析

> 对标旧书: LKD3 Ch18 (基于 2.6 的 Oops 分析)
> 现代: crash + kdump + BTF + drgn

---

## kdump：自动捕获 vmcore

```bash
# kdump 配置
# /etc/default/kdump-tools
KDUMP_KERNEL=/boot/vmlinuz-rt
KDUMP_INITRD=/boot/initrd.img-rt
KDUMP_CORE=/var/crash/

# 启用 kdump
sudo systemctl enable kdump-tools
sudo systemctl start kdump-tools

# 手动触发 crash dump (测试用)
echo c > /proc/sysrq-trigger
# 系统崩溃 → kdump 内核启动 → 复制 vmcore → 重启
# vmcore 保存在 /var/crash/<timestamp>/
```

### kdump 工作原理

```
1. 内核崩溃 (panic)
2. kdump 预留的内核 (crashkernel) 启动
   - 预留内存: crashkernel=128M (bootargs)
3. crashkernel 内核通过 /proc/vmcore 访问崩溃内核的内存
4. 复制 vmcore 到磁盘
5. 重启系统
```

---

## crash 工具

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

# === 基础 ===
crash> bt                 # 崩溃线程栈
crash> bt -a              # 所有 CPU 的栈
crash> ps                 # 进程列表
crash> runq               # 运行队列

# === 内存 ===
crash> kmem -i            # 内核内存概况
crash> kmem -s            # SLUB 缓存统计
crash> vm <pid>           # 进程地址空间

# === 结构体 ===
crash> struct task_struct ffff888012345678  # 查看指定 task_struct
crash> struct -o task_struct  # 看结构体偏移
crash> list task_struct.tasks  # 遍历链表

# === 反汇编 ===
crash> dis -r ffffffff81234567  # 反汇编+寄存器
crash> dis ffffffff81234567 20  # 反汇编 20 条指令

# === 设备/网络 ===
crash> dev                # 设备列表
crash> net                # 网络接口
crash> search -t "ixgbe"  # 搜索内存中的字符串
```

### HFT 崩溃分析场景

```bash
# 1. 查看崩溃时的调用栈
crash> bt
# PID: 1234  TASK: ffff888012345678  CPU: 2  COMMAND: "trading_engine"
#  #0 [ffff800012345670] __crash_kexec at ffffffff81234567
#  #1 [ffff800012345680] panic at ffffffff81234567
#  #2 [ffff800012345690] do_exit at ffffffff81234567
#  ...

# 2. 查看崩溃时的运行队列 (是否有 RT 线程在等待)
crash> runq
# CPU 0 RUNQUEUE: ffff888012345678
#   RT: [PID: 1234] PRIO: 80
# CPU 1: [empty]
# CPU 2: [isolated]
# CPU 3: [isolated]

# 3. 查看崩溃进程的内存映射
crash> vm 1234
# VMA           START          END            FLAGS  FILE
# ffff88801234  00000000aaa0  00000000bbb0  rw-p-  /usr/lib/libc.so
# ffff88801235  00007fffabc0  00007fffdef0  rw-p-  [stack]

# 4. 检查 SLUB 缓存 (是否有内存泄漏)
crash> kmem -s
# CACHE            NAME             OBJSIZE  ALLOCATED  TOTAL
# ffff8880123456   kmalloc-256         256      1024   2048
# ffff8880123457   kmalloc-192         192       512   1024
# 如果 trading_engine 相关的缓存 ALLOCATED 异常多 → 泄漏
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

### crash vs drgn 对比

| 对比 | crash | drgn |
|------|-------|------|
| 使用方式 | 交互式命令 | Python 脚本 |
| 适合 | 快速查看 | 复杂分析、批量处理 |
| 灵活性 | 固定命令集 | 任意 Python 逻辑 |
| 依赖 | 调试符号 | BTF 或调试符号 |
| HFT 场景 | 快速定位崩溃点 | 分析全部交易线程状态 |
| 学习曲线 | 低 (命令行) | 中 (需要 Python) |

### drgn 实战：分析所有交易线程状态

```python
# analyze_trading_threads.py
import drgn

prog = drgn.Program()
prog.set_core_dump('/var/crash/2026-08-14/vmcore')

# 找到所有名为 "trading_engine" 的线程
for task in prog.for_each_task():
    comm = task.comm.string_().decode().rstrip('\x00')
    if 'trading' in comm:
        print(f"\n=== PID={task.pid.value_()} comm={comm} ===")
        print(f"  state={task.__state.value_()}")  # 0=running, 1=sleeping
        print(f"  prio={task.prio.value_()}")
        print(f"  policy={task.policy.value_()}")  # 1=FIFO, 2=RR, 0=NORMAL
        print(f"  on_cpu={task.on_cpu.value_()}")
        print(f"  cpus_allowed={task.cpus_mask}")

        # 查看线程的调用栈
        if task.__state.value_() != 0:  # 不在运行
            print("  Stack:")
            for frame in prog.stack_trace(task):
                print(f"    {frame}")

# 检查是否有线程在等待锁
for task in prog.for_each_task():
    if task.__state.value_() == 2:  # TASK_UNINTERRUPTIBLE
        comm = task.comm.string_().decode().rstrip('\x00')
        if 'trading' in comm:
            print(f"\nWARNING: {comm} (PID={task.pid.value_()}) in D state!")
            print("  May be waiting for a lock or I/O")
```

---

## 与旧书差异

| LKD3 Ch18 讲的 | 6.x 现代实现 | 差异 |
|----------------|-------------|------|
| printk 是第一调试手段 | ftrace/eBPF 是第一手段 | printk 退居二线 |
| kgdb 双机调试 | eBPF 在线调试 | 不需要第二台机器 |
| Oops 栈手动解读 | crash + BTF 自动解析结构体 | 不需要手动查 System.map |
| 无 BPF 概念 | eBPF 是核心观测工具 | LKD3 完全没提 |
| git bisect | 仍然有效 | 不变 |

---

## HFT 关联

| 场景 | 工具 | 操作 |
|------|------|------|
| **内核 panic** | crash + kdump | 分析 vmcore，bt 看崩溃栈 |
| **内存泄漏** | crash kmem -s | SLUB 缓存分配统计 |
| **批量分析** | drgn | Python 脚本遍历所有线程状态 |
| **锁等待死锁** | drgn | 遍历 D 状态线程，检查等待链 |

> **HFT 生产原则：** ① ftrace/eBPF 是生产环境安全的（零开销或极低开销）。② kgdb/gdb+kcore 尽量避免在生产环境使用（会暂停内核）。③ crash 只用于事后分析 vmcore。④ drgn 适合批量分析多个 vmcore。

---

## 自测题

<details>
<summary>Q1: kdump 的工作原理是什么？为什么需要预留内存？</summary>

kdump 在系统崩溃时启动一个预预留的 crashkernel 内核（通过 crashkernel=128M 参数预留 128MB 内存）。崩溃后，crashkernel 内核启动，通过 /proc/vmcore 接口访问崩溃内核的物理内存，将 vmcore 写入磁盘。需要预留内存是因为崩溃内核不能使用崩溃内核可能已经损坏的内存分配器，必须有自己独立的内存空间。
</details>

<details>
<summary>Q2: drgn 相比 crash 的优势是什么？</summary>

drgn 是 Python 库，可以编程式分析 vmcore——写脚本批量处理、复杂条件过滤、跨多个 vmcore 对比分析。crash 是交互式命令行，适合快速查看但难以做复杂分析。例如：分析 100 个交易线程的状态，crash 要手动逐个 `struct task_struct`，drgn 一行 `for task in prog.for_each_task()` 就完成。drgn 还可以利用 Python 生态（pandas 数据分析、matplotlib 可视化）。
</details>

<details>
<summary>Q3: 交易系统内核 panic 后，crash 分析的标准流程是什么？</summary>

① `crash vmlinux vmcore` 打开 vmcore。② `bt` 看崩溃线程的调用栈，定位崩溃函数。③ `ps` 查看进程列表，确认交易线程状态。④ `runq` 查看运行队列，确认是否有 RT 线程在等待。⑤ `kmem -s` 检查 SLUB 缓存，排除内存泄漏。⑥ `struct task_struct <addr>` 查看崩溃线程的详细信息（优先级、CPU 绑定、信号等）。⑦ 如果涉及网络，`net` 和 `dev` 查看网络接口状态。⑧ 如果需要复杂分析，用 drgn 写 Python 脚本。
</details>

<details>
<summary>Q4: LKD3 说的 kgdb 在现代内核调试中还重要吗？</summary>

kgdb 仍然存在但不是主力了。现代内核调试以 ftrace/eBPF 动态追踪为主（不需要暂停内核、生产可用）。kgdb 主要用于：① 驱动开发阶段的源码级断点调试（实验室环境）。② 启动早期调试（eBPF/ftrace 还没初始化时）。③ 硬件相关问题（需要看寄存器状态）。日常内核调试和性能分析用 eBPF/ftrace 就够了。详见 05.6-kernel-debugging/chapter-11-kgdb。
</details>

---

## 交叉引用

- [02-ftrace-modern.md](./02-ftrace-modern.md) — ftrace 追踪框架
- [03-ebpf-observability.md](./03-ebpf-observability.md) — eBPF 可编程追踪
- [05.6-kernel-debugging/chapter-07-oops](../../05.6-kernel-debugging/chapter-07-oops/) — Oops 分析
- [05.6-kernel-debugging/chapter-10-panic-lockup](../../05.6-kernel-debugging/chapter-10-panic-lockup/) — Panic/kdump 详解
