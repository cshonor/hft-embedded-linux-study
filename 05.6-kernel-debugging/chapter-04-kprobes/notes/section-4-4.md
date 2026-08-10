# 4.4 动态注册 Kprobes (通过 /sys)

> 🔴 精读

## 本节要点

### kprobe_events 格式

```bash
# 语法:
# p:<event-name> <symbol> [offset] [<args>]
# r:<event-name> <symbol> [<args>]

# 参数格式:
# <argname>=<type>        — 简单寄存器
# <argname>=+offset(%reg) — 内存引用
# <argname>=+offset(%reg):string — 字符串
# $retval                 — 返回值 (仅 kretprobe)
# $stack, $stackN         — 栈值
# $comm                   — 当前进程名
```

### 实用示例

```bash
# 1. 追踪 open 系统调用的文件名
echo 'p:my_open do_sys_openat2 dfd=%arg1 file=+0(%arg2):string' \
    > /sys/kernel/debug/tracing/kprobe_events
echo 1 > /sys/kernel/debug/tracing/events/kprobes/my_open/enable
cat /sys/kernel/debug/tracing/trace_pipe

# 2. 追踪特定进程的函数调用
echo 'p:my_sched schedule' > /sys/kernel/debug/tracing/kprobe_events
echo 'common_pid == 1234' > /sys/kernel/debug/tracing/events/kprobes/my_sched/filter
echo 1 > /sys/kernel/debug/tracing/events/kprobes/my_sched/enable

# 3. 测量函数耗时（entry + return 配对）
echo 'p:my_in schedule' >> /sys/kernel/debug/tracing/kprobe_events
echo 'r:my_out schedule $retval' >> /sys/kernel/debug/tracing/kprobe_events
echo 'hist:keys=common_pid:vals=$wallclock_ns:sort=common_pid' > \
    /sys/kernel/debug/tracing/events/kprobes/my_out/trigger

# 4. 追踪内核函数的调用者（栈回溯）
echo 'p:my_ip_do_schedule schedule' > /sys/kernel/debug/tracing/kprobe_events
echo 1 > /sys/kernel/debug/tracing/options/stacktrace
echo 1 > /sys/kernel/debug/tracing/events/kprobes/my_ip_do_schedule/enable

# 5. 条件过滤
echo 'p:my_alloc __kmalloc size=%arg1' > /sys/kernel/debug/tracing/kprobe_events
echo 'size > 1048576' > /sys/kernel/debug/tracing/events/kprobes/my_alloc/filter
echo 1 > /sys/kernel/debug/tracing/events/kprobes/my_alloc/enable
# 只追踪分配 > 1MB 的请求
```

### 使用 perf probe 注册

```bash
# perf probe 提供更友好的接口
sudo perf probe --add 'schedule'
sudo perf probe --add 'schedule%return'
sudo perf probe --add 'do_sys_openat2 file=+0(%x1):string'

# 查看已注册探针
perf probe -l

# 使用 perf record 追踪
sudo perf record -e probe:schedule -a sleep 5
sudo perf report

# 删除探针
sudo perf probe --del 'schedule'
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 如何只追踪特定 PID 的函数调用？

> 通过 ftrace 的 filter 机制：`echo 'common_pid == 1234' > /sys/kernel/debug/tracing/events/kprobes/my_probe/filter`。只有 PID 1234 的进程触发 kprobe 时才会记录。支持 `==`, `!=`, `>`, `<`, `&&`, `||` 等操作符。

**Q2:** perf probe 和直接写 kprobe_events 有什么区别？

> perf probe 是 kprobe_events 的封装，提供更友好的命令行接口。它自动解析符号和行号（`perf probe --add 'schedule:15'` 可以在 schedule 函数第 15 行插入探针），支持 C 表达式（`perf probe --add 'kmalloc size'` 自动识别 size 参数）。底层仍使用 kprobe_events，但省去了手动计算偏移和参数位置的麻烦。


**Q:** 通过 /sys/kernel/debug/kprobes/ 动态添加探针的步骤是什么？

> echo "p my_probe symbol_name" > /sys/kernel/debug/kprobes/kprobe_events。然后 echo 1 > /sys/kernel/debug/tracing/events/kprobes/my_probe/enable 启用。结果在 trace 中查看。也可以添加参数：echo "p my_probe do_sys_open arg1=%di:u64" > kprobe_events。

</details>

## 交叉引用

- [05.6 ch04 perf probe](chapter-04-kprobes/notes/section-4-5.md)
