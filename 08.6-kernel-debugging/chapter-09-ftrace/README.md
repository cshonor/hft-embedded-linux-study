# Ch9 Tracing the Kernel Flow

> Part 3: Diagnostics & Advanced Tools · 🔴 精读

Ftrace 体系：tracefs 接口、函数追踪 / 函数图追踪、事件追踪、trace-cmd 命令行前端、KernelShark GUI 前端、perf-tools ftrace wrapper。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 9.1 Ftrace 架构与 tracefs 接口 | `notes/section-9-1.md` |
| 9.2 函数追踪 (function tracer) | `notes/section-9-2.md` |
| 9.3 函数图追踪 (function_graph tracer) | `notes/section-9-3.md` |
| 9.4 事件追踪 (trace events) | `notes/section-9-4.md` |
| 9.5 trace-cmd：命令行前端 | `notes/section-9-5.md` |
| 9.6 KernelShark：GUI 前端 | `notes/section-9-6.md` |
| 9.7 perf-tools ftrace wrapper | `notes/section-9-7.md` |
| 9.8 Ftrace 与 eBPF 的关系 | `notes/section-9-8.md` |

---

## HFT 关联

精读。Ftrace 是 HFT 延迟分析的关键工具链。与 19-systems-performance Ch14 (Ftrace) 互补：本书侧重调试视角，19 侧重性能视角。
