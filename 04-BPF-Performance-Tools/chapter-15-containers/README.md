# Ch 15 容器 · Containers

> **BPF Performance Tools** · Brendan Gregg · **跳过 ⚪**

> 本章定位：**Docker/K8s 下的 BPF 观测** — 底层仍是 CPU/内存/磁盘/网（Ch 6–10 工具 **大多仍适用**），但 **cgroups 软限制** 与 **namespace 隔离** 带来新坑：**吵闹邻居**、节流、宿主机跑工具、容器 ID 过滤。  
> **HFT：** 生产 **tick 路径多为裸金属/专用 VM** — 本章 **⚪ 默认跳过**；若 **风控/网关/监控** 跑在 K8s，incident 时用 **`runqlat --pidnss`、`blkthrot`、`pidnss`**。  
> **上一章：** [chapter-14-内核.md](../chapter-14-kernel/) · **下一章：** [chapter-16-虚拟机管理程序.md](../chapter-16-hypervisors/)

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1 容器没改变什么 / 改变了什么 | [notes/section-1-容器没改变什么改变了什么.md](./notes/section-1-容器没改变什么改变了什么.md) |
| 2 容器技术基础 | [notes/section-2-容器技术基础.md](./notes/section-2-容器技术基础.md) |
| 3 容器环境下 BPF 的挑战 | [notes/section-3-容器环境下BPF的挑战.md](./notes/section-3-容器环境下BPF的挑战.md) |
| 4 传统容器分析工具 | [notes/section-4-传统容器分析工具.md](./notes/section-4-传统容器分析工具.md) |
| 5 核心 BPF 容器工具 | [notes/section-5-核心BPF容器工具.md](./notes/section-5-核心BPF容器工具.md) |
| 6 与前文章节的组合 | [notes/section-6-与前文章节的组合.md](./notes/section-6-与前文章节的组合.md) |
| 7 HFT 部署建议（与本章关系） | [notes/section-7-HFT部署建议与本章关系.md](./notes/section-7-HFT部署建议与本章关系.md) |

---

## 大白话

> Docker/K8s 下的 BPF 观测

下面按原书小节展开；细节见 **小节笔记** 表。

---

## 本章 Checklist

- [ ] **⚪ 默认跳过**— 裸金属 HFT 主路径；容器化 **非 tick 服务** 才需本章。
- [ ] **Ch 6–10 工具仍有效**— 但要在 **宿主机** 跑，并加 **pidns/cgroup 过滤**。
- [ ] **`runqlat --pidnss` + `blkthrot`**— 证 **cgroup 节流** 而非硬件瓶颈。
- [ ] **`pidnss`**— 多容器 **CPU 交错** 竞争。
- [ ] **容器内 top/free 误导**— 用 `docker stats`/cgroup BPF。
- [ ] **`overlayfs`**— 容器文件 I/O 慢于 host 的 **一层原因**。

---

## 相关章节

- 上一章：[chapter-14-内核.md](../chapter-14-kernel/)
- 下一章：[chapter-16-虚拟机管理程序.md](../chapter-16-hypervisors/)
- CPU 调度：[chapter-06-CPU.md](../chapter-06-cpus/)
- 磁盘：[chapter-09-磁盘IO.md](../chapter-09-disk-io/)
- SysPerf 容器/cloud：[chapter-11-cloud-computing](../../03-Systems-Performance-2nd/chapter-11-cloud-computing/)（若存在）
