# 2.9 跟踪点 tracepoints（内核静态插桩：添加 / 原理 / BPF / 原始跟踪点）

> 底本：《BPF之巅》第 2 章技术背景，2.9 节（印刷 p57–62，含 2.9.1–2.9.6）

## 是什么

内核开发者在代码中**有意放置**的静态插桩点，编译进内核二进制。2007 年 Mathieu Desnoyers 开发（初名 Kernel Markers），2009 年随 Linux 2.6.32 正式发布。

## 表 2-7：kprobes vs 跟踪点

| 细节 | kprobes | 跟踪点 |
|---|---|---|
| 类型 | 动态 | 静态 |
| 大致数量 | 50000+ | 100+ |
| 内核维护性 | 无要求 | 有维护成本 |
| 禁用后的开销 | — | 很小（NOP + 元数据） |
| 稳定性 | 不稳定 | 稳定（API 承诺） |

**选型原则：优先跟踪点，不满足时才用 kprobes。** 格式为"子系统:事件名"，如 `kmem:kmalloc`、`sched:sched_process_exec`。（作者称之为"尽最大努力保持稳定"——极少变，但确实变过。）

## 2.9.1 如何添加跟踪点（sched_process_exec 实例）

内核源码 include/trace/events/sched.h 中定义：

```c
TRACE_EVENT(sched_process_exec,
    TP_PROTO(struct task_struct *p, pid_t old_pid, struct linux_binprm *bprm),
    TP_ARGS(p, old_pid, bprm),
    TP_STRUCT__entry(
        __string(filename, bprm->filename)
        __field(pid_t, pid)
        __field(pid_t, old_pid)
    ),
    TP_fast_assign(
        __assign_str(filename, bprm->filename);
        __entry->pid = p->pid;
        __entry->old_pid = old_pid;
    ),
    TP_printk("filename=%s pid=%d old_pid=%d", ...)
)
```

运行时该元数据通过 Ftrace 露出——每个跟踪点有格式文件：

```bash
cat /sys/kernel/debug/tracing/events/sched/sched_process_exec/format
# field:unsigned short common_type; offset:0; size:2;
# field:pid_t pid; ...
# print fmt: "filename=%s pid=%d old_pid=%d", ...
```

跟踪器（BCC/bpftrace）读这个格式文件即可理解事件字段——**无需读内核源码**。调用点在 fs/exec.c 的 `trace_sched_process_exec()`。

## 2.9.2 工作原理（静态跳转补丁 static jump patching）

1. 编译期在跟踪点插入 **5 字节 nop**（长度为日后替换 5 字节 jump 预留）。
2. 函数尾部插入蹦床函数，遍历回调函数数组。
3. 启用时：回调以 RCU 同步方式加入数组；nop 重写为 jmp 跳到蹦床。
4. 禁用时：移除回调；最后一个回调移除后 jmp 重写回 nop。

**禁用开销接近零**（一条 nop）。依赖编译器 `asm goto` 支持；不可用时退化为读内存变量的条件分支。

## 2.9.3 接口

- Ftrace：/sys/kernel/debug/tracing/events（每个跟踪点一个文件，写 1/0 开关）
- perf_event_open()（perf_tracepoint PMU），BPF 工具主用

## 2.9.4 跟踪点和 BPF

- BCC：`TRACEPOINT_PROBE(category, event)` 宏
- bpftrace：`tracepoint:子系统:事件` 探针类型
- Linux 4.7 起 BPF 支持跟踪点（晚于 BCC 早期工具 → BCC 中跟踪点例子偏少）

经典案例 tcplife(8)：作者先用 `tcp_set_state()` 的 kprobe 写成，4.16 内核加入 `sock:inet_sock_set_state` 跟踪点后改为**双探针兼容**：

```python
if (BPF.tracepoint_exists("sock", "inet_sock_set_state")):
    bpf_text = bpf_text_tracepoint
else:
    bpf_text = bpf_text_kprobe
```

bpftrace 例：

```bash
bpftrace -e 'tracepoint:sched:sched_process_exec { printf("exec by %s\n", comm); }'
```

## 2.9.5 BPF 原始跟踪点（BPF_RAW_TRACEPOINT）

Alexei Starovoitov 开发，2018 年 Linux 4.17。向跟踪点暴露**原始参数**（绕过稳定化封装开销），类似"以 kprobe 方式使用跟踪点"：探针名稳定（比 kprobe 稳），参数不稳定（比标准跟踪点快、可访问更多字段）。

压测数据（1 CPU，事件/秒）：

| 场景 | base | tracepoint | raw tracepoint | kprobe |
|---|---|---|---|---|
| task rename | 1.1M | 769K | 947K | 750K |
| urandom read | 1.0M | 789K | — | 697K |

raw tracepoint 几乎追平无插桩基线，适合 7×24 常驻跟踪。

## 2.9.6 扩展阅读

内核 Documentation/trace/tracepoints.rst（Mathieu Desnoyers）。

## HFT 关联

- sched、net、block 子系统的 tracepoint 是 HFT 观测的中坚：调度迁移/唤醒延迟（sched:sched_wakeup → sched_switch 的间隔）、TCP 重传、IO 阻塞——全部稳定 API，内核升级不破。
- tcplife 的"双探针兼容"模式值得照抄：自研运维工具在老内核（kprobe）与新内核（tracepoint）间自动降级。
- BPF_RAW_TRACEPOINT 是常驻低开销观测的首选挂载点（7×24 挂跟踪点的最优解）。

## 陷阱

- 跟踪点数量只有 100+，大多数内核路径没有覆盖——不要期望"只用 tracepoint 就够了"。
- 常见误区：以为 tracepoint 字段永远不变。作者明说"尽最大努力保持稳定"，历史上改过。
- bpftrace 里 tracepoint 参数访问 `args->filename` 依赖 format 文件；自定义内核若裁剪了 events 目录会失败。

## 自测

<details>
<summary>1. 跟踪点禁用时开销为什么接近零？</summary>

静态跳转补丁：禁用态只是一条 5 字节 nop 指令（外加少量元数据），启用时才重写为 jmp。
</details>

<details>
<summary>2. BPF_RAW_TRACEPOINT 与标准跟踪点、kprobe 的权衡是什么？</summary>

名字稳定（优于 kprobe）+ 参数原始不稳定（快于标准跟踪点、字段更多）；压测中接近无插桩基线，适合常驻观测。
</details>

<details>
<summary>3. BCC 工具如何写出跨新旧内核兼容的探针？</summary>

tcplife 模式：BPF.tracepoint_exists() 探测，存在则用 tracepoint 程序，否则退回 kprobe 程序。
</details>
