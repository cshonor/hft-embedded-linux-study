# 4.3 kretprobe：函数返回探针

> 🔴 精读

## 本节要点

### kretprobe 原理

kretprobe 在函数入口插入 kprobe，但回调中**替换返回地址**，使函数返回时跳转到 kretprobe 的 trampoline，执行 handler 后再返回原调用者。

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
```

### kretprobe_instance

- 函数可能被并发调用（如 schedule() 在多核上同时执行）
- 每个调用需要一个独立的 instance 保存返回地址
- `maxactive` 限制并发 instance 数量，超过则计数 `nmissed`

### 通过 kprobe_events 注册 kretprobe

```bash
# r: 表示 kretprobe
echo 'r:my_ret schedule $retval' > /sys/kernel/debug/tracing/kprobe_events
echo 1 > /sys/kernel/debug/tracing/events/kprobes/my_ret/enable
cat /sys/kernel/debug/tracing/trace_pipe
```

### 测量函数耗时 (entry + return)

```bash
# 同时注册 kprobe 和 kretprobe，测量函数耗时
echo 'p:my_entry schedule' >> /sys/kernel/debug/tracing/kprobe_events
echo 'r:my_exit schedule $retval' >> /sys/kernel/debug/tracing/kprobe_events
echo 1 > /sys/kernel/debug/tracing/events/kprobes/enable

# 使用 hist trigger 测量耗时分布
echo 'hist:keys=common_pid:vals=$wallclock_ns' > \
    /sys/kernel/debug/tracing/events/kprobes/my_exit/trigger
```

### HFT 关联

kretprobe 是测量内核函数耗时的核心工具。例如测量 `schedule()` 的实际耗时分布，判断调度延迟是否在可接受范围内。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** kretprobe 如何在函数返回时触发回调？

> kretprobe 在函数入口的 kprobe 回调中，将栈上的返回地址替换为 kretprobe trampoline 的地址，同时将原始返回地址保存在 kretprobe_instance 中。函数执行 `ret` 指令时跳转到 trampoline，trampoline 调用 handler，然后恢复原始返回地址并跳回。

**Q2:** `nmissed` 计数什么时候会增加？

> 当并发调用函数的实例数超过 `maxactive` 时，新的调用无法分配 kretprobe_instance，被跳过（不触发返回回调），`nmissed` 递增。对于高频函数应设置较大的 `maxactive`（如 20-50）以减少 miss。

</details>
