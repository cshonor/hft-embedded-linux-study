# 4.12 BCC 的调试

> 库本：《BPF之巅》第 4 章 BCC（印刷 p91–136），4.12 节（印刷 p128–135）

## 内容详解

除 printf 外，BCC 调试手段总结如下。图 4-6 按"编译流程各环节 × 可用调试工具"组织：

| 流程环节 | 程序调试工具 | 状态调试工具 |
|----------|--------------|--------------|
| Python 源码 | `cat file.py` | `bpflist` |
| BPF C | `BPF(debug=DEBUG_PREPROCESSOR)` | `bpftool map show` |
| LLVM IR | `BPF(debug=DEBUG_LLVM_IR)` | `bpftool prog show` |
| BPF 字节码 | `BPF(debug=DEBUG_BPF)` | `bpftool prog dump xlated` |
| 已挂载程序 | — | `bpflist -vv`（kprobes/uprobes） |
| 机器码 | — | `bpftool prog dump jitted` |
| 内核错误 | — | `dmesg` |

（常见问题：事件丢失、调用栈残缺、符号不完整 → 第 18 章）

### 4.12.1 printf 调试

简单高效："像黑客"，但管用。BPF C 里用 **`bpf_trace_printk()`**，输出到**Ftrace 缓冲区**，读取方式：

```bash
cat /sys/kernel/debug/tracing/trace_pipe
# 或 bpftool prog tracelog
```

书中例子：biolatency 输出可疑，在探针里加一行（`DBG` 前缀便于 grep 区分自己的调试输出）：

```c
u64 ts = bpf_ktime_get_ns();
start.update(&req, &ts);
bpf_trace_printk("DBG req=%llx ts=%lld\n", req, ts);   // 调试行
```

另一个终端 `cat trace_pipe` 即可看到每次命中；用 `grep DBG trace_pipe` 过滤。

**trace vs trace_pipe 的区别：**

| 文件 | 行为 |
|------|------|
| `trace` | 打印**文件头**，读后**不阻塞**、消息保留 |
| `trace_pipe` | **阻塞**读更多消息，消息**被读取后即清除** |

注意：Ftrace 缓冲区是全系统共享的，其他 Ftrace 工具的输出会混进来——过滤即可。

### 4.12.2 BCC 调试输出

- 部分工具自带 **`-D` 参数**打印调试信息（先查 `-h`/`--help`）；
- 许多工具有**未注明的 `--ebpf` 选项**：打印该工具最终生成的 BPF 程序源码：

```bash
opensnoop --ebpf     # 输出整段 BPF C：struct val_t/data_t、BPF_HASH、BPF_PERF_OUTPUT...
```

用途：**BPF 程序被内核拒绝**时，把整段程序打出来查问题。`--ebpf` 为支持 BCC PCP PMDA（第 17 章）而加，不面向最终用户，故 USAGE 不显示。

### 4.12.3 BCC 的调试标志位

工具源码里 `b = BPF(text=bpf_text)` 可改为 `b = BPF(text=bpf_text, debug=0x2)`。表 4-3（定义在 `src/cc/bpf_module.h`）：

| 标志位 | 名称 | 调试内容 |
|--------|------|----------|
| 0x1 | `DEBUG_LLVM_IR` | 打印编译好的 LLVM 中间表示 |
| 0x2 | `DEBUG_BPF` | 在**分支处**打印 BPF 字节码和寄存器状态 |
| 0x4 | `DEBUG_PREPROCESSOR` | 打印预处理结果（类似 `--ebpf`） |
| 0x8 | `DEBUG_SOURCE` | 打印源代码中内嵌的汇编指令 |
| 0x10 | `DEBUG_BPF_REGISTER_STATE` | 打印**所有**指令中的寄存器状态 |
| 0x20 | `DEBUG_BTF` | 打印 BTF 调试信息（否则 BTF 错误被忽略） |

`debug=0x1f` 打印全部（多屏输出）。

### 4.12.4 bpflist

列出**正在运行的 BPF 程序**及信息：

```bash
# bpflist
PID    COMM      TYPE  COUNT
30231  prog      2
30231  map       2
30231  opensnoop
```

→ opensnoop（PID 30231）在跑：**2 个 BPF 程序 + 2 个映射表**——对两个事件插桩（open 入口+返回）各一个程序；一个映射表在探针间传信息，另一个向用户态输出。

- `-v`：对 kprobes/uprobes **计数**；
- `-vv`：计数且**逐个列出**，如 `p:kprobes/pdo_sys_open_bcc_31364 do_sys_open`——注意 **PID 已编码进 kprobe 名字**。

### 4.12.5 bpftool

来自 Linux 源码树（第 2 章讲过）：`prog show`、`prog dump xlated`（BPF 指令）、`prog dump jitted`（机器码）、`map show`、`prog tracelog` 等。

### 4.12.6 dmesg

BPF 或事件源的**内核错误**会进系统日志：

```bash
# dmesg
[8470906.869945] trace_kprobe: Could not insert probe at vfs_rread+0: -2
```

（书中此例是笔误 `vfs_rread` 不存在 → -2 = 函数找不到。）

### 4.12.7 重置事件

开发中引入 bug → BCC 在激活跟踪后**崩溃** → 内核事件源**停留开启状态**且无人消费 → 无谓开销。

- 老内核：BCC 用 `/sys` 下的 Ftrace 接口做**除 perf_events 外**所有事件源——崩溃后 fd 不回收，事件残留；可用 BCC 自带 **`reset-trace.sh`** 清理 `/sys/kernel/debug/tracing/` 下全部激活事件（kprobe_events、uprobe_events、trace、current_tracer 等）。
  ⚠️ 只有确定**机器上没有任何事件消费者**（包括其他跟踪器）时才能跑——它会立即终止所有事件源。
- 新内核（**Linux 4.17+**）：BCC 改用 `perf_event_open()`（基于**文件描述符**）做**所有**事件源——进程崩溃时内核自动回收 fd 并清理事件源，**问题已根治**。

## HFT 关联

- 生产排障三板斧：`dmesg | tail`（内核拒绝原因）→ `bpflist -vv`（谁还挂着探针）→ 工具 `--ebpf`（看最终程序）；记忆图 4-6 的"环节 × 工具"表即可快速定位。
- `bpf_trace_printk` 调试只允许在测试机；`debug=0x2` 适合验证工具改造后验证器是否还过。

## 陷阱

- ⚠️ `trace_pipe` 读取后消息即清除且阻塞——长开一个 `cat trace_pipe` 窗口会"吃掉"其他调试者的输出。
- ⚠️ `reset-trace.sh` 是全局核弹：跑之前确认没有别的 trace 消费者（含生产监控 Agent）。
- ⚠️ 4.17 前的老内核才有事件残留问题；新内核上崩溃即自动清理，别再用 reset-trace"以防万一"。

<details>
<summary>自测题</summary>

1. `bpf_trace_printk()` 的输出去哪了？怎么读？
   <details><summary>答案</summary>Ftrace 缓冲区；`cat /sys/kernel/debug/tracing/trace_pipe` 或 `bpftool prog tracelog`。</details>

2. debug=0x4 打印什么？与哪个未注明选项效果类似？
   <details><summary>答案</summary>`DEBUG_PREPROCESSOR`：预处理结果，类似 `--ebpf`。</details>

3. opensnoop 为什么显示 2 个程序 + 2 个映射表？
   <details><summary>答案</summary>对两个事件（open 入口与返回）各插桩一个 BPF 程序；一个映射表在探针间传信息、一个向用户态输出。</details>

4. Linux 哪个版本起 BCC 崩溃不再遗留激活的事件源？机制是什么？
   <details><summary>答案</summary>4.17+：所有事件源改走 `perf_event_open()`，基于文件描述符，进程崩溃时内核自动回收 fd 并清理。</details>
</details>
