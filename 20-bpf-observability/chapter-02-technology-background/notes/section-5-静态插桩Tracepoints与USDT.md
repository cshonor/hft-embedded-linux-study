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


### 常见陷阱

1. **不知道如何查找可用的 tracepoint** — tracepoint 在 /sys/kernel/debug/tracing/events/ 下按子系统组织；用 `bpftrace -l 'tracepoint:*'` 或 `cat /sys/kernel/debug/tracing/available_events` 查找
2. **USDT 探针需要重新编译应用** — USDT 需在编译时用 dtrace 宏插入探针点；已有应用如果没有 USDT，只能用 uprobe 替代
3. **忽视 tracepoint format 文件** — 每个 tracepoint 有 format 文件描述字段名和类型，不看 format 直接写 args->fieldname 会出错

<details>
<summary>📝 自测题（点击展开）</summary>

1. **Tracepoint 相比 kprobe 的三个优势是什么？**

   <details>
   <summary>参考答案</summary>

   (1) 稳定的名称和 ABI——内核开发者承诺不随意改名；(2) 有 format 文件描述字段（`cat .../format`），知道 args 有哪些字段；(3) 可以用 `bpftrace -l 'tracepoint:sched:*'` 方便查找。优先级：tracepoint > kprobe。

   </details>

2. **如何查看某个 tracepoint 的可用字段？**

   <details>
   <summary>参考答案</summary>

   `cat /sys/kernel/debug/tracing/events/<子系统>/<事件>/format`。例如查看 sched_switch 的字段：`cat /sys/kernel/debug/tracing/events/sched/sched_switch/format`。bpftrace 中用 `args-><字段名>` 访问。

   </details>

3. **USDT 在 HFT 应用中如何使用？有什么前提？**

   <details>
   <summary>参考答案</summary>

   在关键路径代码中用 `DTRACE_PROBE()` 宏插入探针点（编译时为 nop，零开销）。排障时用 `bpftrace -e 'usdt:myapp:probe_name { ... }'` attach。前提：(1) 编译时开启 USDT 支持；(2) 二进制保留探针信息；(3) 知道探针名称。

   </details>

</details>

---
