## 14.5–14.7、14.10 事件源、Filter 与 Hist Triggers

### 多数据源

| 源 | 说明 | 配置入口 |
|----|------|----------|
| **Tracepoints** | 内核静态观测点 | `events/.../enable` |
| **kprobes** | 内核动态函数插桩 | `events/kprobes/...` |
| **uprobes** | 用户态动态插桩 | `events/uprobes/...` |

```bash
# 启用 sched 切换 tracepoint
echo 1 > /sys/kernel/tracing/events/sched/sched_switch/enable
echo 1 > /sys/kernel/tracing/tracing_on
cat /sys/kernel/tracing/trace_pipe
```

### Filters 与 Triggers

| 机制 | 作用 |
|------|------|
| **Filter** | 只记录满足条件的事件（如 `pid == 1234`） |
| **Trigger** | 事件发生时执行动作（snapshot、stacktrace、**histogram**） |

**Hist Triggers（直方图触发器）：**

- 在 **内核内** 对事件字段做 **直方图聚合** — 不需把每条事件送到用户态。
- 支持：单键/多键、PID 过滤、**stacktrace**、**synthetic events**（合成事件链）。

**价值：** 高频率事件（如 sched_switch、net receive）— hist 比 raw trace **低开销**。

```bash
# 概念示例（语法随内核版本见 Documentation/trace/histogram.rst）
# echo 'hist:keys=pid:vals=latency' > .../trigger
```

**HFT：** 调度迁移 histogram — 看 hot thread 是否被 **频繁 cpu_migrations**（Ch 6/7）。

→ Ch 15 [BPF 可编程聚合](../../chapter-15-bpf/) — 新系统优先 BPF maps histogram

---


---

← [本章导读](../README.md)
