## 14.1–14.2 核心能力与 tracefs

### Ftrace 概述

| 演进 | 说明 |
|------|------|
| **起源** | **Function Tracer** — 追踪内核函数调用 |
| **现状** | 多追踪器框架：function、function_graph、tracepoint、hwlat… |
| **优势** | **内核自带**、无用户态依赖、资源受限环境可用 |

### tracefs 接口

挂载点（发行版二选一）：

```
/sys/kernel/debug/tracing   # debugfs（需 debug 挂载）
/sys/kernel/tracing         # 新式 tracefs 挂载
```

**交互方式：** 读写虚拟文件 — 配置 tracer、启停、读 buffer。

```bash
# 常见路径检查
ls /sys/kernel/tracing 2>/dev/null || ls /sys/kernel/debug/tracing

# 查看可用 tracer
cat /sys/kernel/tracing/available_tracers

# 当前 tracer
cat /sys/kernel/tracing/current_tracer
```

| 文件/目录 | 作用 |
|-----------|------|
| `current_tracer` | 选择追踪器（nop/function/function_graph…） |
| `tracing_on` | 0/1 开关 |
| `trace` | 一次性读 buffer |
| `trace_pipe` | **阻塞流式读**（类似 tail -f） |
| `set_ftrace_filter` | 限制追踪哪些函数 |
| `events/` | tracepoint 事件树 |
| `set_event` | 启用哪些 tracepoint |

**权限：** 通常需 **root** 或 `CAP_SYS_ADMIN` — 生产限时长、限 filter。

→ Ch 4 [Ftrace 在工具链中的位置](../../chapter-04-observability-tools/)

---


---

← [本章导读](../README.md)
