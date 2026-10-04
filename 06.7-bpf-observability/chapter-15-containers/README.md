# Ch 15 容器 · Containers

> **BPF Performance Tools** · Brendan Gregg · **跳过 ⚪**（用 K8s 时 🟡）

> 本章定位：**Docker/K8s 下的 BPF 观测** — 底层仍是 CPU/内存/磁盘/网（Ch 6–10 工具 **大多仍适用**），但 **cgroups 软限制** 与 **namespace 隔离** 带来新坑：**吵闹邻居**、节流、宿主机跑工具、容器 ID 过滤。
> **HFT：** 生产 **tick 路径多为裸金属/专用 VM** — 本章 **⚪ 默认跳过**；若 **风控/网关/监控** 跑在 K8s，incident 时用 **`runqlat --pidnss`、`blkthrot`、`pidnss`**，CPU 限流先 cat `cpu.stat` 看 nr_throttled。
> **上一章：** [chapter-14-kernel](../chapter-14-kernel/) · **下一章：** [chapter-16-hypervisors](../chapter-16-hypervisors/)

---

## 小节笔记（按原书真实小节）

| 原书小节 | 笔记 | 覆盖工具 |
|----------|------|----------|
| 15.1 背景 + 15.1.1 BPF 能力 | [notes/section-1-背景知识.md](./notes/section-1-背景知识.md) | 两种虚拟化、namespace/cgroup v1v2、吵闹邻居 |
| 15.1.2 挑战 | [notes/section-2-BPF的挑战.md](./notes/section-2-BPF的挑战.md) | BPF 特权、容器 ID、nsproxy 提取法、kubectl-trace、FaaS |
| 15.1.3 策略 + 15.2 传统工具 | [notes/section-3-分析策略与传统工具.md](./notes/section-3-分析策略与传统工具.md) | systemd-cgtop、kubectl top、docker stats、/sys/fs/cgroups、perf -G |
| 15.3.1–15.3.2 | [notes/section-4-BPF工具-runqlat与pidnss.md](./notes/section-4-BPF工具-runqlat与pidnss.md) | runqlat --pidnss、pidnss |
| 15.3.3–15.3.4 | [notes/section-5-BPF工具-blkthrot与overlayfs.md](./notes/section-5-BPF工具-blkthrot与overlayfs.md) | blkthrot、overlayfs |
| 15.4–15.5 | [notes/section-6-BPF单行程序与练习.md](./notes/section-6-BPF单行程序与练习.md) | cgroupid() 单行、3 道练习 |
| 15.6 小结 | [notes/section-7-小结.md](./notes/section-7-小结.md) | 工具速查 + HFT 部署建议 |

---

## 大白话

容器没有新资源，只有新"围墙"（namespace 视图 + cgroup 配额）。排障三问：被限流了吗（cpu.stat/blkthrot）？邻居抢核了吗（pidnss）？队列排到谁了（runqlat --pidnss）？
