# Ch9 Tracing the Kernel Flow

> Part 3: Diagnostics & Advanced Tools · 🔴 精读

Ftrace 体系：tracefs 接口、函数追踪 / 函数图追踪、事件追踪、trace-cmd 命令行前端、KernelShark GUI 前端、perf-tools ftrace wrapper。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 9.1 Ftrace 架构与 tracefs 接口 | `notes/01-ftrace-architecture-tracefs.md` |
| 9.2 函数追踪 (function tracer) | `notes/02-function-tracer.md` |
| 9.3 函数图追踪 (function_graph tracer) | `notes/03-function-graph-tracer.md` |
| 9.4 事件追踪 (trace events) | `notes/04-trace-events.md` |
| 9.5 trace-cmd：命令行前端 | `notes/05-trace-cmd.md` |
| 9.6 KernelShark：GUI 前端 | `notes/06-kernelshark.md` |
| 9.7 perf-tools ftrace wrapper | `notes/07-perf-tools-ftrace.md` |
| 9.8 Ftrace 与 eBPF 的关系 | `notes/08-ftrace-ebpf-relation.md` |

---

## HFT 关联

精读。Ftrace 是 HFT 延迟分析的关键工具链。与 06.6-systems-performance Ch14 (Ftrace) 互补：本书侧重调试视角，19 侧重性能视角。
