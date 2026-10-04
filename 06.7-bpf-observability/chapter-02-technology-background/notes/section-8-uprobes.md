# 2.8 uprobes（用户态动态插桩：机制 / 接口 / 开销）

> 底本：《BPF之巅》第 2 章技术背景，2.8 节（印刷 p52–57，含 2.8.1–2.8.5）

## 是什么

uprobes 提供用户态程序的动态插桩，2012 年 7 月合入 Linux 3.5（前身 utrace）。可插桩位置：函数入口、任意偏移、函数返回（uretprobes）。

**基于文件**：跟踪可执行文件/共享库中的函数时，所有使用该文件的进程（含尚未启动的）都被插桩——可全系统跟踪 libc 调用。

## 2.8.1 工作机制（gdb 实证）

与 kprobes 同理：目标指令替换为 int3，不需要时恢复。书中用 gdb 反汇编 bash 的 readline() 展示：

- 插桩前：`0x...5610 <+0>: cmpl $0xffffffff,...`
- 插桩后：`0x...5610 <+0>: int3` ← 第一条指令已被替换

uretprobes 同样用蹦床劫持返回地址。

## 2.8.2 uprobes 接口

- Ftrace：写 /sys/kernel/debug/tracing/uprobe_events
- perf_event_open()（4.17 起 perf_uprobe PMU），BPF 工具主用
- 内核内有 register_uprobe_event() 但未作为 API 暴露

## 2.8.3 BPF 与 uprobes

- BCC：`attach_uprobe()` / `attach_uretprobe()`（BCC 支持入口+任意地址；bpftrace 仅入口）
- bpftrace：`uprobe:` / `uretprobe:` 探针类型

BCC 例：gethostlatency(8) 跟踪 DNS 解析延迟：

```python
b.attach_uprobe(name="libc", sym="getaddrinfo",  fn_name="do_entry")
b.attach_uretprobe(name="libc", sym="getaddrinfo", fn_name="do_return")
```

bpftrace 例：统计 libc 中 gethost* 函数调用：

```bash
bpftrace -e 'uprobe:/lib/x86_64-linux-gnu/libc.so.6:gethost* { @[probe] = count(); }'
```

## 2.8.4 开销与未来（本节最重要的警告）

> **uprobes 可能挂在每秒百万次的事件上（如 malloc/free），BPF 已调优，但小开销 × 百万次 = 放大。跟踪 malloc/free 可能造成目标应用 10 倍以上性能损耗。**

纪律：识别高频事件，**避开高频事件，找低频事件回答同样的问题**。这类损耗只可接受于测试环境排查或生产已出问题时。

未来方向：用共享库（如 LTTng-UST 模式）替代需要内核往返的 uprobes，用户态内完成跟踪，快 10–100 倍。

## 2.8.5 扩展阅读

内核 Documentation/trace/uprobetracer.txt。

## 高频事件的替代方案对照（2.8.4 展开）

"避开高频事件，找低频事件回答同样的问题"——具体换法按问题类型：

| 想知道 | 高频挂法（危险） | 低频替代 |
|---|---|---|
| 内存是否泄漏 | uprobe:malloc/free | 周期读 /proc/<pid>/statm 水位 + brk/mmap 次数（memleak 工具的采样模式） |
| 分配热点在谁 | uprobe:malloc 带 size 分桶 | memleak 的采样模式（只跟踪部分分配）+ exit 时的未释放汇总 |
| 锁竞争 | uprobe:pthread_mutex_lock | futex tracepoint（syscall 边界，冲突时才有事件）+ offcputime |
| 函数调用计数 | uprobe:func | 若有编译期插桩条件改 USDT；或 funccount 限短窗口 |

共同模式：**把"每次都发生的事件"换成"只在异常/边界时发生的事件"**——futex 只在锁冲突时进内核（无冲突的 fast path 是纯用户态原子操作），brk/mmap 远稀于 malloc——低频事件天然自带过滤。

## HFT 关联

- HFT 交易软件是典型用户态程序：给自家撮合/风控二进制加 uprobe 探针（毫秒级热路径函数）可在不改代码的情况下量化内部耗时——但**绝不能上 malloc/free 这类每秒千万级事件**。
- 替代策略：对自研代码优先 USDT（见 2.10/2.11），uprobe 留给"无源码第三方库"（如行情解码库）的短窗口排查。
- 基于 uprobe 的全系统 libc 跟踪适合性能审计，不适合交易时段常驻。

## 陷阱

- uprobe 挂到高频函数 = 目标程序慢 10 倍起步；先估算事件频率再挂。
- 基于"文件"意味着库里所有进程都被插桩——跟踪 libc 时连 init/系统服务都中招，输出量爆炸且相互干扰，应加 pid 过滤。
- 符号被 strip 的二进制无法按函数名插桩（可用偏移，但极脆弱）。

## 自测

<details>
<summary>1. uprobe 与 kprobe 机制上最大的共同点是什么？</summary>

都是把目标首指令替换为 int3 断点、陷入执行处理函数后恢复；uretprobe/kretprobe 都用蹦床劫持返回地址。
</details>
<details>
<summary>2. "基于文件"的插桩意味着什么？</summary>

对该可执行文件/库插桩时，所有用到它的进程（包括之后启动的）都被插桩，可全系统跟踪库调用，但也需注意过滤。
</details>

<details>
<summary>3. 为什么跟踪 malloc/free 会带来 10 倍以上损耗？正确的做法是什么？</summary>

分配函数每秒百万级触发，每次 upobe 陷阱+上下文切换+处理的开销被频率放大。做法：改用低频事件（如 brk/mmap、周期性采样内存水位）回答同样问题。
</details>
