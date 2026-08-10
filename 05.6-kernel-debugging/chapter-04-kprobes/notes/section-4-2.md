# 4.2 kprobe：函数入口探针

> 🔴 煞读

## 本节要点

### 编程式 kprobe

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
```

### 通过 /sys 动态注册 (kprobe_events)

```bash
# 1. 添加 kprobe
echo 'p:my_probe schedule' > /sys/kernel/debug/tracing/kprobe_events
# 格式: p:<name> <symbol> [offset] [args]
# p = kprobe, r = kretprobe

# 2. 捕获参数
echo 'p:my_probe do_sys_openat2 dfd=%arg1 filename=+0(%arg2):string flags=%arg3' \
    > /sys/kernel/debug/tracing/kprobe_events

# 3. 启用
echo 1 > /sys/kernel/debug/tracing/events/kprobes/my_probe/enable

# 4. 查看输出
cat /sys/kernel/debug/tracing/trace_pipe
# ... my_probe: (... dfd=0xffffffff filename="/etc/hosts" flags=0x0)

# 5. 清除
echo > /sys/kernel/debug/tracing/kprobe_events
```

### ARM64 寄存器映射

```c
// ARM64 函数参数在 pt_regs 中:
// regs->regs[0]  = x0 (arg1)
// regs->regs[1]  = x1 (arg2)
// ...
// regs->regs[7]  = x7 (arg8)
// regs->sp       = 栈指针
// regs->pc       = 程序计数器
// regs->pstate   = 状态寄存器
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** kprobe_events 的 `+0(%arg2):string` 是什么意思？

> `+0(%arg2)` 表示取 arg2（第二个参数）的值作为地址，偏移 0 字节处。`:string` 表示从该地址读取以 null 结尾的字符串。这样可以把指针参数的内容（如文件名）打印出来。`+offset(%arg):type` 格式支持 string/u32/x64 等类型。

**Q2:** kprobe 探针的性能开销大约是多少？

> 单次 kprobe 触发的开销约 1-5μs（断点异常 + 保存上下文 + 回调 + 单步执行 + 恢复）。对于高频函数（如 schedule、timer），可能显著影响性能。如果需要更低开销，考虑 ftrace function tracer（约 100-300ns）或 eBPF（约 50-100ns）。

</details>
