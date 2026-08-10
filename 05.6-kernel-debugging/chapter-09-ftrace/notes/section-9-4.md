# 9.4 事件追踪 (trace events)

> 🔴 精读

## 本节要点

### tracepoints (静态追踪点)

```bash
# 查看可用事件
ls /sys/kernel/debug/tracing/events/
# block/  ext4/  irq/  kmem/  net/  sched/  syscalls/  timer/  ...

# 查看某类事件
ls /sys/kernel/debug/tracing/events/sched/
# sched_switch  sched_wakeup  sched_process_fork  ...

# 启用事件
echo 1 > events/sched/sched_switch/enable
echo 1 > tracing_on
cat trace_pipe

# 输出:
# my_app-1234 [002] d..2 12345.678: sched_switch: prev_comm=my_app prev_pid=1234 prev_prio=120 prev_state=S ==> next_comm=swapper/2 next_pid=0 next_prio=120
```

### 过滤事件

```bash
# 按字段过滤
echo 'prev_pid == 1234' > events/sched/sched_switch/filter
echo 1 > events/sched/sched_switch/enable

# 启用整类事件
echo 1 > events/sched/enable

# 启用所有事件
echo 1 > events/enable
```

### 常用 HFT 相关事件

| 事件 | 子系统 | 用途 |
|------|--------|------|
| `sched_switch` | sched | 追踪调度切换 |
| `sched_wakeup` | sched | 追踪唤醒 |
| `irq_handler_entry/exit` | irq | 追踪中断处理 |
| `kmalloc/kfree` | kmem | 追踪内存分配 |
| `netif_receive_skb` | net | 追踪网络收包 |
| `timer expire` | timer | 追踪定时器 |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** tracepoints 和 kprobes 有什么区别？

> tracepoints 是**静态**的——开发者在代码中预埋的追踪点（`trace_sched_switch()`），性能开销极低（未启用时几乎为零）。kprobes 是**动态**的——可在任意函数入口插入探针，无需修改源码。tracepoints 数量有限（内核预定义），kprobes 灵活性更高但开销更大。

</details>
