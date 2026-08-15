# 4.2 kprobe：函数入口探针

> 🔴 精读 · Part 2: Instrumentation & Memory Debugging

## 本节要点

kprobe 在函数入口插入探针，捕获入口寄存器和参数。支持编程式（内核模块）和动态式（kprobe_events）两种使用方式。

## 编程式 kprobe（内核模块）

```c
#include <linux/kprobes.h>

static int my_pre_handler(struct kprobe *p, struct pt_regs *regs) {
    // ARM64: regs->regs[0]-[7] = x0-x7 (函数参数)
    // x8 = 返回值地址 (indirect result location)
    pr_info("schedule() called, prev=%px\n", (void *)regs->regs[0]);
    return 0;
}

static struct kprobe kp = {
    .symbol_name = "schedule",
    .pre_handler = my_pre_handler,
};

static int __init my_init(void) {
    int ret = register_kprobe(&kp);
    if (ret < 0) {
        pr_err("register_kprobe failed: %d\n", ret);
        return ret;
    }
    pr_info("kprobe registered at %px\n", kp.addr);
    return 0;
}

static void __exit my_exit(void) {
    unregister_kprobe(&kp);
}
module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");
```

## 通过 /sys 动态注册 (kprobe_events)

```bash
# 1. 添加 kprobe
echo 'p:my_probe schedule' > /sys/kernel/tracing/kprobe_events
# 格式: p:<name> <symbol> [offset] [args]
# p = kprobe, r = kretprobe

# 2. 捕获参数
echo 'p:my_probe do_sys_openat2 dfd=%arg1 filename=+0(%arg2):string flags=%arg3' \
    > /sys/kernel/tracing/kprobe_events

# 3. 启用
echo 1 > /sys/kernel/tracing/events/kprobes/my_probe/enable

# 4. 查看输出
cat /sys/kernel/tracing/trace_pipe
# ... my_probe: (... dfd=0xffffffff filename="/etc/hosts" flags=0x0)

# 5. 清除
echo > /sys/kernel/tracing/kprobe_events
```

## ARM64 寄存器映射

```c
// ARM64 函数参数在 pt_regs 中:
// regs->regs[0]  = x0 (arg1)
// regs->regs[1]  = x1 (arg2)
// ...
// regs->regs[7]  = x7 (arg8)
// regs->sp       = 栈指针
// regs->pc       = 程序计数器
// regs->pstate   = 状态寄存器

// x86_64 函数参数在 pt_regs 中:
// regs->di = rdi (arg1)
// regs->si = rsi (arg2)
// regs->dx = rdx (arg3)
// regs->cx = rcx (arg4)
// regs->r8 = r8  (arg5)
```

## kprobe_events 参数格式详解

```bash
# 参数格式语法:
# <argname>=<type>             — 简单寄存器
# <argname>=+offset(%reg)      — 内存引用
# <argname>=+offset(%reg):type — 带类型的内存引用
# $retval                      — 返回值 (仅 kretprobe)
# $stack, $stackN              — 栈值
# $comm                        — 当前进程名
# $arg1, $arg2, ...            — 函数参数（自动解析）

# 类型支持:
# :u8  :u16 :u32 :u64  — 无符号整数
# :s8  :s16 :s32 :s64  — 有符号整数
# :x8  :x16 :x32 :x64  — 十六进制
# :string               — 字符串
# :char[NN]             — 字符数组
```

### 实用示例

```bash
# 追踪 kmalloc 的分配大小
echo 'p:my_alloc __kmalloc size=$arg1' > /sys/kernel/tracing/kprobe_events
echo 'size > 65536' > /sys/kernel/tracing/events/kprobes/my_alloc/filter
echo 1 > /sys/kernel/tracing/events/kprobes/my_alloc/enable
# 只追踪 > 64KB 的分配

# 追踪特定进程
echo 'p:my_sched schedule' > /sys/kernel/tracing/kprobe_events
echo 'common_pid == 1234' > /sys/kernel/tracing/events/kprobes/my_sched/filter
echo 1 > /sys/kernel/tracing/events/kprobes/my_sched/enable

# 追踪函数调用栈
echo 'p:my_probe schedule' > /sys/kernel/tracing/kprobe_events
echo 1 > /sys/kernel/tracing/options/stacktrace
echo 1 > /sys/kernel/tracing/events/kprobes/my_probe/enable
```

## pre_handler vs post_handler

| 回调 | 执行时机 | 可获取信息 | 典型用途 |
|------|---------|-----------|---------|
| pre_handler | 原始指令执行**前** | 入口寄存器（参数） | 记录入参、开始计时 |
| post_handler | 原始指令执行**后** | 执行后的寄存器 | 记录中间状态 |

```c
// 注意：post_handler 拿不到返回值（函数还没返回）
// 要拿返回值必须用 kretprobe

static int pre(struct kprobe *p, struct pt_regs *regs) {
    // 函数入口：可以拿到参数
    pr_info("entry: arg0=%lx\n", regs->regs[0]);
    return 0;
}

static void post(struct kprobe *p, struct pt_regs *regs, unsigned long flags) {
    // 原始指令执行后，函数还没返回
    // 这里 regs 可能被函数修改了
    pr_info("post: x0=%lx\n", regs->regs[0]);
}
```

## kprobe 性能开销

```bash
# 测量 kprobe 开销
echo 'p:my_probe schedule' > /sys/kernel/tracing/kprobe_events
echo 1 > /sys/kernel/tracing/events/kprobes/my_probe/enable
echo 1 > /sys/kernel/tracing/options/latency-format
echo 'hist:keys=common_pid:vals=$wallclock_ns' > \
    /sys/kernel/tracing/events/kprobes/my_probe/trigger

# 对比不同工具开销:
# kprobe (内核模块):  ~1-5μs/次
# kprobe_events:      ~1-5μs/次 (相同底层机制)
# ftrace function:    ~100-300ns/次
# eBPF kprobe:        ~50-100ns/次
# trace_printk:       ~100-200ns/次
```

## HFT 关联

kprobe 在 HFT 中的典型用途：
1. **延迟溯源**：测量 `schedule()`、`__netif_receive_skb()` 等函数耗时
2. **参数捕获**：捕获网卡驱动的关键参数（如 DMA 描述符地址）
3. **调用链追踪**：配合 stacktrace 查看谁调用了关键函数
4. **条件过滤**：只追踪特定 PID 或特定大小的分配

```bash
# HFT 热路径延迟溯源
echo 'p:hft_rx hft_process_packet' > /sys/kernel/tracing/kprobe_events
echo 'r:hft_rx_ret hft_process_packet $retval' > /sys/kernel/tracing/kprobe_events
echo 'hist:keys=common_pid:vals=$wallclock_ns:sort=vals:asc' > \
    /sys/kernel/tracing/events/kprobes/hft_rx_ret/trigger
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** kprobe_events 的 `+0(%arg2):string` 是什么意思？

> `+0(%arg2)` 表示取 arg2（第二个参数）的值作为地址，偏移 0 字节处。`:string` 表示从该地址读取以 null 结尾的字符串。这样可以把指针参数的内容（如文件名）打印出来。`+offset(%arg):type` 格式支持 string/u32/x64 等类型。

**Q2:** kprobe 探针的性能开销大约是多少？

> 单次 kprobe 触发的开销约 1-5μs（断点异常 + 保存上下文 + 回调 + 单步执行 + 恢复）。对于高频函数（如 schedule、timer），可能显著影响性能。如果需要更低开销，考虑 ftrace function tracer（约 100-300ns）或 eBPF（约 50-100ns）。

**Q3:** kprobe 的 pre_handler 和 post_handler 分别在什么时候执行？

> pre_handler：在原始指令执行**之前**调用，可以检查入参寄存器。post_handler：在原始指令执行**之后**调用，可以检查执行后的寄存器状态。对于函数入口的 kprobe，pre_handler 拿到的是参数，post_handler 拿不到返回值（函数还没返回）。要拿返回值用 kretprobe。

**Q4:** 如何只追踪特定 PID 的函数调用？

> 通过 ftrace 的 filter 机制：`echo 'common_pid == 1234' > /sys/kernel/tracing/events/kprobes/my_probe/filter`。只有 PID 1234 的进程触发 kprobe 时才会记录。支持 `==`, `!=`, `>`, `<`, `&&`, `||` 等操作符。

**Q5:** 编程式 kprobe 和 kprobe_events 各有什么优劣？

> 编程式 kprobe（内核模块）：可以在回调中执行任意逻辑（如修改数据结构），但需要编译和加载模块。kprobe_events：通过 /sys 接口动态配置，不需要编译模块，适合临时调试。但只能在 trace 中记录参数，不能执行复杂逻辑。HFT 调试优先用 kprobe_events（快速、不需重编译）。

</details>

## 交叉引用

- [05.6 ch04 kprobes 架构](../../chapter-04-kprobes/notes/01-kprobes-architecture.md)
- [05.6 ch04 kretprobe](../../chapter-04-kprobes/notes/03-kretprobe-return-handler.md)
- [05.6 ch04 动态注册](../../chapter-04-kprobes/notes/04-dynamic-registration-sysfs.md)
