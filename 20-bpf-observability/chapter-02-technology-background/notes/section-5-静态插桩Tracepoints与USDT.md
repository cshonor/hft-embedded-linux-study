# 5. 静态插桩：Tracepoints 与 USDT

比动态插桩 **API 稳定、可预期**。

### Tracepoints（内核）

| 要点 | 说明 |
|------|------|
| **定义** | 内核开发者 **预埋** 的观测点（如 `sched:sched_process_exec`、`syscalls:sys_enter_openat`） |
| **优势** | **稳定名称**、有 **format** 文件描述字段 — bpftrace/BCC 首选 |
| **优先序** | **Tracepoint > kprobe**（当两者都能表达同一事件时） |

```bash
ls /sys/kernel/debug/tracing/events/sched/
cat /sys/kernel/debug/tracing/events/sched/sched_process_exec/format
```

### USDT（用户态静态探针）

| 要点 | 说明 |
|------|------|
| **定义** | 应用编译期插入探针 — 无 tracer attach 时多为 **`nop`**，**零开销** |
| **例子** | MySQL、Node.js、部分 C++ 框架 |
| **JIT 语言** | Java 等需 **动态 USDT** / 特殊 agent — 见 [chapter-12-语言.md](../../chapter-12-languages/) |

```bash
# 列出进程 USDT（若有）
sudo bpftrace -l 'usdt:*' 2>/dev/null | head
```

---
