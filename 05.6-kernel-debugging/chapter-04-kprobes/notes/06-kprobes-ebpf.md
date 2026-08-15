# 4.6 Kprobes 与 eBPF 的关系

> 🔴 精读 · Part 2: Instrumentation & Memory Debugging

## 本节要点

eBPF 是 kprobes 的进化——底层仍使用 kprobes 机制，但在其之上提供安全沙箱、可编程聚合和更低开销。

## eBPF 如何使用 Kprobes

```
bpftrace / BCC / libbpf
         ↓
    eBPF 程序 (BPF bytecode)
         ↓
    bpf() 系统调用加载到内核
         ↓
    verifier 验证安全性
         ↓
    JIT 编译为原生指令
         ↓
    attach 到 kprobe / kretprobe / tracepoint
         ↓
    kprobe 触发 → 执行 eBPF 程序（原生速度） → 收集数据到 map
```

## Kprobes vs eBPF 对比

| 特性 | kprobes (内核模块) | eBPF (bpftrace/BCC) |
|------|-------------------|---------------------|
| 编程方式 | C 内核模块 | C (BCC) / AWK-like (bpftrace) |
| 编译 | 需编译 .ko | JIT 编译 |
| 安全性 | 可能导致 panic | 验证器保证安全 |
| 性能 | ~1-5μs/次 | ~50-100ns/次 |
| 数据处理 | 回调中处理 | map 聚合 + 用户态读取 |
| 持久性 | 模块加载即生效 | 程序终止即清理 |
| 生产可用 | ⚠️ 需谨慎 | ✅ 安全 |

## bpftrace 示例

```bash
# 等价于 kretprobe 测量 schedule() 耗时
sudo bpftrace -e '
kretprobe:schedule {
    @sched_ns[pid] = nsecs;
}
kretprobe:schedule /@sched_ns[pid]/ {
    $dur = nsecs - @sched_ns[pid];
    @sched_us = hist($dur / 1000);
    delete(@sched_ns[pid]);
}'

# 追踪 open 系统调用（等价于 kprobe_events）
sudo bpftrace -e '
kprobe:do_sys_openat2 {
    printf("%s opened %s\n", comm, str(arg2));
}'

# 统计函数调用次数
sudo bpftrace -e 'kprobe:__kmalloc { @[comm] = count(); }'

# 测量函数耗时分布
sudo bpftrace -e '
kprobe:hft_process_packet { @start[tid] = nsecs; }
kretprobe:hft_process_packet /@start[tid]/ {
    @us = hist((nsecs - @start[tid]) / 1000);
    delete(@start[tid]);
}'

# 追踪大于 64KB 的分配
sudo bpftrace -e '
kprobe:__kmalloc /arg1 > 65536/ {
    printf("%s allocated %lu bytes\n", comm, arg1);
}'
```

## eBPF 安全性：verifier

```
eBPF 程序加载时的验证流程:

1. 控制流图 (CFG) 构建
   └── 确保没有不可达代码

2. 类型检查
   └── 寄存器类型追踪（scalar/ptr/ctx）

3. 边界检查
   └── 所有内存访问必须有边界验证

4. 终止性
   └── 确保程序不会无限循环（DAG 检查）

5. 资源限制
   └── 指令数限制（4096 条，6.x 放宽到 100万）
   └── 栈大小限制（512 字节）

结果: 通过验证 → JIT 编译加载
      失败 → 拒绝加载，返回错误
```

## eBPF 数据收集方式

| 方式 | 说明 | 适用场景 |
|------|------|---------|
| printf() | 直接输出到 trace_pipe | 调试、实时查看 |
| map (hash) | 键值对聚合 | 统计计数、分类 |
| map (hist) | 直方图 | 延迟分布 |
| map (stack) | 栈追踪 | 调用链分析 |
| ringbuf | 高性能环形缓冲区 | 事件流 |
| bpf_get_stackid() | 栈 ID 去重 | 大规模栈追踪 |

## 何时用 Kprobes vs eBPF

| 场景 | 推荐 | 原因 |
|------|------|------|
| 快速临时探查 | bpftrace (eBPF) | 一行命令，无需编译 |
| 需要修改内核行为 | kprobes (内核模块) | eBPF 不能修改数据 |
| 需要复杂数据处理 | eBPF (map 聚合) | 内核中聚合，减少用户态开销 |
| 内核版本 < 5.x | kprobes | eBPF 功能不全 |
| HFT 生产环境 | eBPF | 更低开销、安全验证 |
| 需要精确参数捕获 | kprobe_events | 更直接、可控 |
| 需要长期监控 | eBPF (BCC 工具) | 安全、可编程 |

## eBPF 在 HFT 中的应用

```bash
# 1. 网卡收包延迟监控（长期运行，低开销）
bpftrace -e '
kprobe:__netif_receive_skb { @start[tid] = nsecs; }
kretprobe:__netif_receive_skb /@start[tid]/ {
    @rx_us = hist((nsecs - @start[tid]) / 1000);
    delete(@start[tid]);
}'

# 2. 系统调用延迟监控
bpftrace -e '
tracepoint:syscalls:sys_enter_epoll_wait { @start[tid] = nsecs; }
tracepoint:syscalls:sys_exit_epoll_wait /@start[tid]/ {
    @epoll_us = hist((nsecs - @start[tid]) / 1000);
    delete(@start[tid]);
}'

# 3. 调度延迟监控
bpftrace -e '
tracepoint:sched:sched_switch {
    @offcpu[prev_pid] = nsecs;
}
tracepoint:sched:sched_wakeup /@offcpu[args->pid]/ {
    @wakeup_us[args->pid] = (nsecs - @offcpu[args->pid]) / 1000;
    delete(@offcpu[args->pid]);
}'
```

## HFT 关联

eBPF 是 HFT 生产环境监控的**首选工具**：

1. **安全**：verifier 保证不会崩溃内核
2. **低开销**：JIT 编译，~50-100ns/次
3. **可编程**：自定义过滤、聚合、输出
4. **无需重编译**：动态加载/卸载
5. **长期运行**：适合 7×24 监控

eBPF 不能完全替代 kprobes——eBPF 的 kprobe attach 底层仍用 kprobes 机制。但 eBPF 在 kprobes 之上提供了安全沙箱和聚合能力。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么 eBPF 比 kprobes 内核模块更安全？

> eBPF 程序在加载前经过验证器 (verifier) 检查：确保不会越界访问内存、不会无限循环、不会持有锁过久。kprobes 内核模块没有验证，代码 bug 会导致内核 panic。eBPF 程序出错最多是探针失效，不会崩溃内核。

**Q2:** eBPF 的性能为什么比 kprobes 内核模块好？

> eBPF 程序经 JIT 编译为原生指令，在 kprobe 回调中直接执行，无需保存/恢复完整寄存器上下文（验证器保证安全）。kprobes 内核模块需要完整的异常处理流程（保存所有寄存器 → 回调 → 单步执行原始指令 → 恢复）。eBPF 开销约 50-100ns vs kprobes 1-5μs。

**Q3:** eBPF 能完全替代 kprobes 吗？

> 不能完全替代。eBPF 的 kprobe/kretprobe attach 底层仍用 kprobes 机制。eBPF 在 kprobes 之上提供安全沙箱和聚合能力，但断点注入和 out-of-line execution 仍由 kprobes 提供。eBPF 还可以 attach 到 tracepoint、perf event、XDP 等更多 hook 点。

**Q4:** bpftrace 中 hist() 和 printf() 有什么区别？

> `printf()` 每次事件都输出一行，适合实时查看但高频时会大量输出。`hist()` 在内核中聚合为直方图，只输出统计结果，适合高频事件的性能分析。HFT 调试中优先用 hist() 减少开销。

**Q5:** eBPF verifier 如何保证程序不会无限循环？

> verifier 构建控制流图 (CFG)，检查程序是否为有向无环图 (DAG)。对于循环，verifier 要求必须有明确的退出条件且循环次数有界。6.x 放宽了限制（支持有界循环），但仍检查终止性。指令数也有上限（4096 条，6.x 放宽到 100万）。

</details>

## 交叉引用

- [05.6 ch04 kprobes 架构](chapter-04-kprobes/notes/01-kprobes-architecture.md)
- [05.6 ch09 ftrace vs eBPF](chapter-09-ftrace/notes/08-ftrace-ebpf-relation.md)
