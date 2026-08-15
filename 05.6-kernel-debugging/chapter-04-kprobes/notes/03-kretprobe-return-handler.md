# 4.3 kretprobe：函数返回探针

> 🔴 精读 · Part 2: Instrumentation & Memory Debugging

## 本节要点

kretprobe 在函数入口插入 kprobe，但回调中**替换返回地址**，使函数返回时跳转到 trampoline，执行 handler 后再返回原调用者。

## kretprobe 原理

```
正常函数调用:
  caller → callee → ret → caller

kretprobe 插入后:
  caller → callee(入口被替换为BRK)
                  ↓
           kprobe handler: 替换返回地址为 trampoline
                  ↓
           callee 执行函数体
                  ↓
           ret → trampoline → kretprobe handler → 原caller
```

## 编程式 kretprobe

```c
#include <linux/kprobes.h>

static int my_ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs) {
    // ARM64: 返回值在 regs->regs[0] (x0)
    unsigned long retval = regs_return_value(regs);
    pr_info("schedule() returned, retval=%ld\n", retval);
    return 0;
}

static struct kretprobe kr = {
    .handler = my_ret_handler,
    .maxactive = 20,  // 最多追踪 20 个并发实例
    .kp.symbol_name = "schedule",
};

static int __init my_init(void) {
    return register_kretprobe(&kr);
}
static void __exit my_exit(void) {
    unregister_kretprobe(&kr);
    pr_info("missed %d calls\n", kr.nmissed);
}
module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");
```

## kretprobe_instance 机制

- 函数可能被并发调用（如 schedule() 在多核上同时执行）
- 每个调用需要一个独立的 instance 保存返回地址
- `maxactive` 限制并发 instance 数量，超过则计数 `nmissed`

```
kretprobe_instance 池:

  instance[0] → 保存 CPU0 上 schedule() 的返回地址
  instance[1] → 保存 CPU1 上 schedule() 的返回地址
  instance[2] → 空闲
  ...
  instance[19] → 空闲

  如果 20 个 instance 全部被占用，新的调用被跳过 → nmissed++
```

### maxactive 选择建议

| 函数类型 | 建议 maxactive | 原因 |
|---------|---------------|------|
| 低频（init/open） | 10（默认） | 很少并发 |
| 中频（read/write） | 20-50 | 可能多线程并发 |
| 高频（schedule/timer） | 100+ | 多核高频并发 |
| 极高频（netif_rx） | 200+ | 网络风暴时大量并发 |

## 通过 kprobe_events 注册 kretprobe

```bash
# r: 表示 kretprobe
echo 'r:my_ret schedule $retval' > /sys/kernel/tracing/kprobe_events
echo 1 > /sys/kernel/tracing/events/kprobes/my_ret/enable
cat /sys/kernel/tracing/trace_pipe
```

## 测量函数耗时 (entry + return)

```bash
# 同时注册 kprobe 和 kretprobe，测量函数耗时
echo 'p:my_entry schedule' >> /sys/kernel/tracing/kprobe_events
echo 'r:my_exit schedule $retval' >> /sys/kernel/tracing/kprobe_events
echo 1 > /sys/kernel/tracing/events/kprobes/enable

# 使用 hist trigger 测量耗时分布
echo 'hist:keys=common_pid:vals=$wallclock_ns:sort=vals' > \
    /sys/kernel/tracing/events/kprobes/my_exit/trigger

# 查看直方图
cat /sys/kernel/tracing/events/kprobes/my_exit/hist
# { common_pid: 1234 }  wallclock_ns: 5000-10000  count: 42
# { common_pid: 1234 }  wallclock_ns: 10000-20000 count: 10
```

## kretprobe trampoline 实现

```c
// kretprobe 的 trampoline 机制（架构相关）

// ARM64:
// 1. 函数入口的 kprobe handler 替换 LR (Link Register) 为 trampoline 地址
// 2. 函数执行 ret 指令 → 跳转到 trampoline
// 3. trampoline 保存寄存器，调用 kretprobe handler
// 4. 恢复原始 LR，跳回原调用者

// x86_64:
// 1. 函数入口的 kprobe handler 替换栈上的返回地址为 trampoline 地址
// 2. 函数执行 ret 指令 → 弹出 trampoline 地址 → 跳转到 trampoline
// 3. trampoline 保存寄存器，调用 kretprobe handler
// 4. 恢复原始返回地址，ret 跳回原调用者
```

## HFT 关联

kretprobe 是测量内核函数耗时的核心工具：

```bash
# 测量 schedule() 耗时分布（判断调度延迟）
echo 'p:sched_in schedule' > /sys/kernel/tracing/kprobe_events
echo 'r:sched_out schedule $retval' > /sys/kernel/tracing/kprobe_events
echo 'hist:keys=common_pid:vals=$wallclock_ns:sort=vals:asc' > \
    /sys/kernel/tracing/events/kprobes/sched_out/trigger

# 测量网卡收包耗时
echo 'p:rx_in __netif_receive_skb skb=%arg1' > /sys/kernel/tracing/kprobe_events
echo 'r:rx_out __netif_receive_skb $retval' > /sys/kernel/tracing/kprobe_events
echo 'hist:keys=common_pid:vals=$wallclock_ns' > \
    /sys/kernel/tracing/events/kprobes/rx_out/trigger
```

HFT 场景中 kretprobe 的应用：
1. **调度延迟**：测量 `schedule()` 实际耗时，判断是否在可接受范围
2. **收包延迟**：测量 `__netif_receive_skb()` 耗时，定位网络处理瓶颈
3. **中断处理**：测量 IRQ handler 耗时，确认是否影响实时性
4. **内存分配**：测量 `__kmalloc()` 耗时，确认是否在高频路径中分配

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** kretprobe 如何在函数返回时触发回调？

> kretprobe 在函数入口的 kprobe 回调中，将栈上的返回地址替换为 kretprobe trampoline 的地址，同时将原始返回地址保存在 kretprobe_instance 中。函数执行 `ret` 指令时跳转到 trampoline，trampoline 调用 handler，然后恢复原始返回地址并跳回。

**Q2:** `nmissed` 计数什么时候会增加？

> 当并发调用函数的实例数超过 `maxactive` 时，新的调用无法分配 kretprobe_instance，被跳过（不触发返回回调），`nmissed` 递增。对于高频函数应设置较大的 `maxactive`（如 20-50）以减少 miss。

**Q3:** kretprobe 如何保存和恢复返回地址？

> kretprobe 在函数入口替换返回地址为 trampoline 地址。当函数返回时跳到 trampoline，trampoline 调用 handler 处理返回值，然后恢复原始返回地址跳转。每个 kretprobe 实例用 kretprobe_instance 池避免每次分配。

**Q4:** kretprobe 的 maxactive 参数有什么作用？

> maxactive 指定同时活跃的 kretprobe 实例数（默认 10）。如果一个函数在多个 CPU 上并发执行且尚未返回，需要多个 instance。maxactive 太小会导致丢失某些调用（missed），太大浪费内存。高频函数应设大（如 100）。

**Q5:** 如何用 kprobe + kretprobe 测量函数耗时？

> 注册配对的 kprobe（入口）和 kretprobe（返回），在入口记录时间戳，在返回时计算差值。或使用 hist trigger：`echo 'hist:keys=common_pid:vals=$wallclock_ns' > .../trigger`，ftrace 自动配对 entry/return 并计算耗时分布。

</details>

## 交叉引用

- [05.6 ch04 kprobe 入口探针](chapter-04-kprobes/notes/02-kprobe-entry-handler.md)
- [05.6 ch04 动态注册](chapter-04-kprobes/notes/04-dynamic-registration-sysfs.md)
- [05.6 ch04 perf probe](chapter-04-kprobes/notes/05-perf-probe-relation.md)
