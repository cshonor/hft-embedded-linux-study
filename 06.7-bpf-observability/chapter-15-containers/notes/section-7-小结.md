# 7. 小结（15.6）

> 底本：《BPF之巅》第 15 章 容器，15.6 节（印刷 p718）

## 原书小结

本章概述了 Linux 容器，并说明了 BPF 跟踪如何暴露：

- **容器 CPU 争用**（runqlat --pidnss、pidnss）
- **cgroup 节流时间**（blkthrot；CPU 侧用 /sys/fs/cgroups 的 cpu.stat）
- **覆盖文件系统的延迟**（overlayfs）

## 本章工具速查

| 工具 | 回答的问题 | 键/探针 |
|------|-----------|---------|
| runqlat --pidnss | 哪个容器的运行队列延迟高 | PID 命名空间 / sched 跟踪点 |
| pidnss | 容器是否在同一 CPU 上互相打断 | [pidns, nodename] / kprobe finish_task_switch |
| blkthrot | 哪个 cgroup 被 blkio 限流多少次 | css.id / kprobe blk_throtl_bio |
| overlayfs | 容器 OverlayFS 读写延迟分布 | pidns 过滤 / kprobe ovl_*_iter |
| cgroupid() 单行 | 按 cgroup v2 过滤任意跟踪点 | cgroup ID |

## HFT 部署建议（与本章关系）

| 实践 | 原因 |
|------|------|
| tick 路径不上 K8s 或至少独占 cpuset | 软限制/CFS 配额会在硬限制前静默限流（cpu.stat 的 nr_throttled 是证据） |
| 容器监控从主机侧采 cgroup 接口 | 容器内 free/iostat/mpstat 显示的是宿主机数据（非容器感知） |
| K8s 基础设施服务排障三件套 | runqlat --pidnss + pidnss + blkthrot |
| 自研观测按 pidns+nodename 归因 | 内核无容器 ID，这是书中的标准替代键 |

<details>
<summary>自测题</summary>

1. 本章四个 BPF 工具分别暴露什么？
   <details><summary>答</summary>runqlat --pidnss：按容器的运行队列延迟；pidnss：CPU 容器切换次数；blkthrot：blkio cgroup 限流计数；overlayfs：Overlay 文件系统读写延迟。</details>

2. 为什么容器观测普遍要"用户态桥"？
   <details><summary>答</summary>容器 ID 由用户态管理，内核只有命名空间/cgroup；需要脚本（docker inspect、cgroupid()）做容器→内核键的换算。</details>
</details>
