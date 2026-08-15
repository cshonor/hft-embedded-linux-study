# 1. 容器没改变什么 / 改变了什么

| 未变 | 新增 |
|------|------|
| CPU、内存、块 I/O、网络栈 | **cgroups 限额/份额/节流** |
| 前文 BPF 工具原理 | **namespace 视图** — 容器内 `top`/`free` **误导** |
| 内核路径 | **OverlayFS** 层 |

```
同一宿主机
  ├─ 容器 A（策略？）  ── cgroup CPU quota / blkio throttle
  ├─ 容器 B（日志）    ── noisy neighbor
  └─ 宿主机上跑 BPF    ── 需 pidns/uts 过滤
```

**HFT 原则：** **低延迟策略不进共享 cgroup 容器**；若必须共宿，本章工具证 **是否被 throttle**。

---
