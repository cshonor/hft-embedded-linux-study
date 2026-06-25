# 2. 核心前端：为何需要 BCC / bpftrace

直接写 **内核 BPF 字节码** 极其繁琐。本书聚焦高级前端：

| 前端 | 角色 | 典型用法 |
|------|------|----------|
| **BCC** | Python/Lua/C 框架 + **成套预制工具** | `execsnoop`、`biolatency`、`runqlat` — 日常 runbook |
| **bpftrace** | 类 awk 的 **单行/短脚本语言** | 即兴 kprobe、tracepoint、USDT — ad hoc 根因 |
| **IO Visor** | 早期 eBPF 商业化/教育项目（BCC 生态背景） | 理解历史；生产以 **bcc-tools + bpftrace** 为主 |

→ 深入：[chapter-04-BCC.md](../../chapter-04-bcc/) · [chapter-05-bpftrace.md](../../chapter-05-bpftrace/)

---
