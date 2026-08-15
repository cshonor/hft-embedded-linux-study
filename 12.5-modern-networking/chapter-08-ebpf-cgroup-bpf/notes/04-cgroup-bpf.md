# 11 — cgroup-BPF：容器网络隔离

> **对应 Rosen:** 无
> **内核版本:** cgroup BPF 4.10+；SOCK_ADDR 4.18+

## cgroup-BPF 是什么

cgroup-BPF 允许在 cgroup 级别挂载 BPF 程序，对该 cgroup 内所有进程的网络操作进行过滤/修改：

| 程序类型 | 挂载点 | 作用 |
|---------|--------|------|
| BPF_CGROUP_INET_INGRESS | cgroup ingress | 过滤该 cgroup 进程收到的包 |
| BPF_CGROUP_INET_EGRESS | cgroup egress | 过滤该 cgroup 进程发出的包 |
| BPF_CGROUP_SOCK_ADDR | connect/bind | 拦截/修改 connect/bind 调用 |
| BPF_CGROUP_SOCK_ops | socket 操作 | 监控/修改 socket 状态 |

## 与 XDP-BPF / tc-BPF 的区别

| 维度 | XDP-BPF | tc-BPF | cgroup-BPF |
|------|---------|--------|-----------|
| 作用域 | 全局（网卡级） | 全局（网卡级） | cgroup 级（进程组） |
| 粒度 | 每个包 | 每个 sk_buff | 每个进程的网络操作 |
| 能力 | 包过滤/重定向 | 包分类/标记 | socket 级过滤/修改 |

## HFT 关联

cgroup-BPF 在 HFT 中主要用于**进程隔离和资源控制**：
- 交易进程和行情进程在不同 cgroup，各自有独立网络策略
- 限制非交易进程的网络带宽
- 监控每个进程的 socket 状态（重传/RTT 等）

> 注：cgroup-BPF 不是 HFT 网络路径的主力工具，主要用于运维/隔离层面。
