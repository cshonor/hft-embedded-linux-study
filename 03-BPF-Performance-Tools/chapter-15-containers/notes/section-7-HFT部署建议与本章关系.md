# 7. HFT 部署建议（与本章关系）

| 实践 | 原因 |
|------|------|
| **策略裸金属 / 专用 VM** | 避免 cgroup 软限制与 noisy neighbor |
| 辅助服务容器化 | 可接受 — 用本章工具 **隔离诊断** |
| BPF 在 **host** | 权限 + 全栈可见 |
| 不用容器内 `free` 判断内存 | 误读 limit |

**若已在 K8s 跑交易相关组件：** 检查 **CPU set / dedicated node pool / Guaranteed QoS** — 本章工具验证 **是否仍被 throttle**。

---
