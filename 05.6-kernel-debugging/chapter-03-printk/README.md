# Ch3 Debug via Instrumentation - printk and Friends

> Part 2: Instrumentation & Memory Debugging · 🔴 精读

printk 体系：日志级别、速率限制、dynamic debug 框架 (pr_debug / dev_dbg 动态开关)、ftrace_printk。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 3.1 printk 基础与日志级别 | `notes/01-printk-basics-loglevel.md` |
| 3.2 速率限制与异步打印 | `notes/02-rate-limiting-async.md` |
| 3.3 dynamic debug 框架 | `notes/03-dynamic-debug.md` |
| 3.4 dev_dbg 与设备相关调试 | `notes/04-dev-dbg.md` |
| 3.5 ftrace_printk (trace_marker 前身) | `notes/05-ftrace-printk.md` |

---

## HFT 关联

精读。dynamic debug 框架在 6.x 内核中仍是主力调试手段，可在不重编译的情况下动态开关调试输出。HFT 自定义内核模块应大量使用 pr_debug。
