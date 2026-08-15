# 1. 背景知识（15.1 节）

> 底本：《BPF之巅》第 15 章 容器，15.1–15.1.1 节（印刷 p701–704）

容器是 Linux 上部署服务的常用方法：安全隔离、更短启动时间、资源控制、简化部署。**分析容器中的应用所需的大部分知识和工具前面章节已有**——容器世界里 CPU 还是 CPU、文件系统还是文件系统、磁盘还是磁盘；本章只讲容器**特有**的部分：命名空间与 cgroup。

## 两种实现容器的方法

| 方法 | 机制 | 代表 |
|------|------|------|
| **操作系统级虚拟化** | **命名空间**分区系统 + **cgroup** 资源控制；所有容器**共享同一个内核** | Docker、Kubernetes |
| **硬件级虚拟化** | 轻量级虚拟机，每个 VM 有**自己的内核** | Kata Containers（Intel Clear Containers）、AWS Firecracker |

本章讲操作系统级；硬件虚拟化见第 16 章。

图 15-1：主机上多个容器，每个有自己的 PID 1、命名空间（矩形）、cgroup（梯形，含共享的系统 cgroup），共享同一个内核。

## 命名空间与 cgroup

- **命名空间限制系统的视图**：cgroup、ipc、mnt、net、pid、user、uts
  - pid 命名空间：容器 /proc 只见自己的进程
  - mnt 命名空间：限制可见的挂载点
  - uts 命名空间：隔离 uname(2) 返回的信息（源自 UNIX Time-sharing System 命名）
- **cgroup 限制资源使用**：
  - v1：blkio、cpu、cpuacct、cpuset、devices、hugetlb、memory、net_cls、net_prio、pids、rdma；Kubernetes 等仍在用
  - v2：解决 v1 各种缺点，容器技术未来将迁移，v1 最终弃用
  - 可设**硬限制**（CPU/内存上限）与**软限制**（基于比例）；cgroup 可层次化

## 吵闹的邻居（noisy neighbor）

容器性能分析常见问题：某容器大量耗资源导致其他容器紧张。容器进程全在同一内核上，**从主机上可同时分析**——与传统分时系统多应用分析无本质不同。区别：**cgroup 软限制可能在硬限制之前被触达**，而未更新支持容器的监控工具**看不到**软限制及其导致的性能问题。

## 15.1.1 BPF 的分析能力

容器分析工具一般基于指标（容器/cgroup/命名空间的设置与大小）；BPF 跟踪可回答：

- **每个容器的运行队列延迟**是多少？
- 调度器正在**同样的 CPU 上切换容器**吗？
- 目前是否遇到了 **CPU 或磁盘的软限制**？

手段：调度器事件跟踪点 + kprobe 内核函数。调度器等事件高频 → **不适合长期监控，适合临时分析**。

- 内核有 **cgroup 事件跟踪点**（cgroup:cgroup_setup_root、cgroup:cgroup_attach_task 等）——调试容器启动的高级事件
- BPF 还可挂到 **cgroup 入口/出口点处理网络数据包**

## HFT 关联

- 交易核心多为裸金属/专用 VM，但风控、行情网关、回测服务常跑 K8s——"吵闹的邻居"就是共置集群里的回测任务抢核
- **软限制先于硬限制被触达**：限流抖动常来自软限制，metrics-server 未必能看到，BPF 才能证实时刻

<details>
<summary>自测题</summary>

1. 操作系统级与硬件级容器虚拟化的本质区别？
   <details><summary>答</summary>OS 级共享同一内核（namespace+cgroup，Docker/K8s）；硬件级每 VM 有自己的内核（Kata/Firecracker）。</details>

2. uts 命名空间隔离什么？
   <details><summary>答</summary>uname(2) 系统调用返回的信息（nodename 等）——这也是 pidnss 工具取容器名的依据。</details>

3. 容器与传统分时系统分析的主要差异？
   <details><summary>答</summary>cgroup 软限制可能在硬限制前被触达，不支持容器的监控工具看不到这些软限制导致的性能问题。</details>
</details>
