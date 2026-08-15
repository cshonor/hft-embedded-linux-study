# 2.2 仪表化方法概览

> ⬜ 跳读 · Part 1: Introduction & Approaches

## 本节要点

仪表化 (Instrumentation) 是在运行时观察程序行为的方法——从最简单的 printk 到最先进的 eBPF。

## 仪表化方法分类

### 按侵入性排序

| 方法 | 工具 | 侵入性 | 适用阶段 | 需要重编译 |
|------|------|--------|---------|-----------|
| eBPF | bpftrace/BCC | 极低 | 开发/生产 | ❌ |
| ftrace | tracefs | 低 | 开发/生产 | ❌ |
| 动态调试 | dyndbg | 低 | 开发/测试 | ❌ |
| kprobes | /sys/kernel/kprobe | 中 | 开发/生产 | ❌ |
| trace_printk | 源码内嵌 | 低 | 开发 | ✅ |
| printk | 源码内嵌 | 高 | 开发 | ✅ |
| KASAN | 编译选项 | 极高 | 开发 | ✅ |
| KCSAN | 编译选项 | 高 | 开发 | ✅ |
| KFENCE | 编译选项 | 极低 | 开发/生产 | ✅ |

### 按功能分类

| 类别 | 工具 | 检测内容 |
|------|------|---------|
| 日志输出 | printk/dev_dbg/dyndbg | 自定义信息 |
| 函数追踪 | ftrace function/graph | 调用链/耗时 |
| 事件追踪 | tracepoints/ftrace events | 预定义事件 |
| 动态探针 | kprobes/kretprobes | 任意函数入口/返回 |
| 可编程追踪 | eBPF/bpftrace | 自定义逻辑 |
| 内存检测 | KASAN/KFENCE/kmemleak | 越界/UAF/泄漏 |
| 并发检测 | LOCKDEP/KCSAN | 死锁/竞态 |
| 性能采样 | perf stat/record | CPU 热点/缓存命中 |

## 工具选择原则

```
调试流程与工具选择:

Step 1: 了解调用流程
├── ftrace function tracer（全局调用链）
├── ftrace function_graph（带耗时）
└── bpftrace one-liner（快速验证）

Step 2: 在关键点捕获数据
├── kprobes（函数入口参数）
├── kretprobes（函数返回值）
└── trace_printk（热路径细节）

Step 3: 精确定位
├── printk（最终确认）
├── KGDB 单步（逻辑验证）
└── dyndbg（按文件/函数开关）

Step 4: 系统性检测
├── KASAN（内存越界/UAF）
├── KCSAN（数据竞争）
├── LOCKDEP（死锁）
└── kmemleak（内存泄漏）

Step 5: 性能分析
├── perf record（CPU 热点）
├── ftrace function_graph（函数耗时）
└── perf stat（硬件计数器）
```

## 插桩 vs 采样

| 维度 | 插桩 (Instrumentation) | 采样 (Sampling) |
|------|----------------------|----------------|
| 原理 | 在代码中插入检查点 | 周期性检查状态 |
| 精度 | 精确（每次事件） | 统计性（部分事件） |
| 开销 | 与事件频率成正比 | 固定（与频率无关） |
| 代表工具 | kprobes/ftrace/eBPF | perf record |
| 适用 | 追踪调用链/捕获参数 | 性能分析/热点定位 |
| 漏检 | 不会 | 可能错过短事件 |

## eBPF：下一代仪表化

```bash
# bpftrace：一行命令追踪任意函数
# 追踪 vfs_read 的调用次数
bpftrace -e 'kprobe:vfs_read { @[comm] = count(); }'

# 追踪函数参数
bpftrace -e 'kprobe:do_sys_open { printf("%s: %s\n", comm, str(arg1)); }'

# 追踪函数耗时
bpftrace -e 'kprobe:vfs_read { @start[tid] = nsecs; }
             kretprobe:vfs_read /@start[tid]/ {
               @us = hist((nsecs - @start[tid]) / 1000);
               delete(@start[tid]);
             }'
```

eBPF 的优势：
- 不修改源码
- 安全验证（verifier 确保不会崩溃）
- 可编程（自定义过滤和聚合）
- 生产环境可用

## HFT 关联

HFT 调试工具选择策略：

1. **热路径调试**：trace_printk + ftrace（不改变时序）
2. **生产环境监控**：eBPF/bpftrace（安全、低开销）
3. **内存检测**：开发用 KASAN，生产用 KFENCE
4. **性能分析**：perf record + ftrace function_graph
5. **按需调试**：dyndbg（pr_debug 运行时开关）

原则：**从最低侵入性开始**，逐步升级到高侵入性工具。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么推荐先用 ftrace 而非 printk 调试？

> ftrace 无侵入——不改变时序、不需重编译、可动态开关。printk 会序列化输出改变时序（可能掩盖竞争条件），且需要重新编译代码修改打印点。先用 ftrace 了解整体调用流程，再用 kprobes/printk 在精确定位后深入。

**Q2:** 插桩 (instrumentation) 和采样 (sampling) 调试有什么区别？

> 插桩在代码中插入检查点（kprobes/ftrace），精确但开销大。采样周期性检查状态（perf stat/PMU），开销低但可能错过事件。内核调试中插桩用于精确追踪，采样用于性能分析。

**Q3:** eBPF 相比 kprobes 有什么优势？

> eBPF 在内核中运行经过验证的字节码，可以自定义过滤、聚合和输出逻辑，不需要用户空间处理。kprobes 只是触发点，数据需要传到用户空间处理。eBPF 更安全（verifier 防止崩溃）、更灵活（可编程）、更适合生产环境。

**Q4:** trace_printk 为什么比 printk 更适合热路径？

> trace_printk 写入 per-CPU 环形缓冲区（无锁、无 I/O），开销约 100-200ns。printk 输出到控制台可能阻塞数十毫秒（串口），且持有全局自旋锁影响所有 CPU。对 HFT 纳秒级热路径，trace_printk 可接受，printk 不可接受。

**Q5:** 什么时候应该用 dyndbg 而不是 ftrace？

> 需要输出特定变量值和自定义消息时用 dyndbg（pr_debug 已在代码中）。需要追踪调用链、函数耗时、事件流时用 ftrace。dyndbg 更适合"在特定代码位置输出信息"，ftrace 更适合"了解整体执行流程"。两者经常配合使用。

</details>

## 交叉引用

- [05.6 ch03 dynamic debug](chapter-03-printk/notes/03-dynamic-debug.md)
- [05.6 ch04 kprobes](chapter-04-kprobes/notes/01-kprobes-architecture.md)
- [05.6 ch09 ftrace](chapter-09-ftrace/notes/01-ftrace-architecture-tracefs.md)
