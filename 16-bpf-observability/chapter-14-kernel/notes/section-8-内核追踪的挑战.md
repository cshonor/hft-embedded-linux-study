# 8. 内核追踪的挑战

| 挑战 | 对策 |
|------|------|
| **kprobe 绑内部函数名** | 内核版本变 → 工具 **碎** |
| 结构体布局变 | 验证器/脚本失败 |
| **优先 tracepoint** | 稳定 ABI — `syscalls:*`、`block:*`、`sched:*` |
| CO-RE / BTF | 新版本工具链 — [Ch 2 BTF](../../chapter-02-technology-background/) |

**原则：** 生产 runbook **优先 BCC 维护工具 + tracepoint**；adhoc kprobe 仅 **短跑验证**。

---
