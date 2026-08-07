# LWN 文章摘要 — 现代内核网络栈

> 按 6 个主题域分类，每篇标注：对应 Rosen 哪章、现代变化、HFT 关联

## 1. 收包路径重构（NAPI / page_pool / GRO-GSO）

| 序号 | LWN 文章 | 对应 Rosen | 核心变化 | HFT 关联 |
|------|----------|-----------|---------|---------|
| 01 | NAPI 现代化：threaded NAPI 与 busy polling | Ch14 | 5.11+ threaded NAPI，SO_BUSY_POLL | 收包延迟优化 |
| 02 | page_pool API：现代 Rx buffer 管理 | Ch1/Ch4 | 4.18+ page_pool 取代旧 alloc_page | 内存分配延迟 |
| 03 | GRO/GSO 演进与性能 | Ch11 | 硬件 offload + 软件聚合 | 吞吐量 vs 延迟权衡 |
| 04 | sk_buff → xdp_buff：收包路径分流 | Ch1/Ch11 | XDP 路径在 sk_buff 分配前处理 | 早丢弃减少开销 |

## 2. XDP（eXpress Data Path）

| 序号 | LWN 文章 | 对应 Rosen | 核心变化 | HFT 关联 |
|------|----------|-----------|---------|---------|
| 05 | XDP 架构与 use case 全景 | 无（书出版时不存在） | 4.8+ 内核数据路径旁路 | 行情早过滤/早丢弃 |
| 06 | AF_XDP：零拷贝到用户态 | 无 | 4.18+ XSK socket 绕过 sk_buff | 内核态 DPDK 替代方案 |
| 07 | XDP redirect 与 cpumap | 无 | 4.15+ 跨网卡/CPU 重定向 | 多核行情分发 |
| 08 | XDP vs DPDK：内核旁路两条路 | 无 | XDP 内核态 vs DPDK 用户态 | HFT 选型决策 |

## 3. eBPF 网络（tc-BPF / XDP-BPF / cgroup-BPF）

| 序号 | LWN 文章 | 对应 Rosen | 核心变化 | HFT 关联 |
|------|----------|-----------|---------|---------|
| 09 | tc-BPF：流量控制中的 eBPF | Ch6/Ch9 | BPF 程序挂在 tc ingress/egress | 包分类/路由 |
| 10 | XDP-BPF：收包路径中的 eBPF | 无 | BPF 程序挂在 XDP hook | 最早点包处理 |
| 11 | cgroup-BPF：容器网络隔离 | 无 | cgroup 级别 socket filter | 多租户隔离 |

## 4. nftables（替代 Netfilter/iptables）

| 序号 | LWN 文章 | 对应 Rosen | 核心变化 | HFT 关联 |
|------|----------|-----------|---------|---------|
| 12 | nftables 架构与迁移 | Ch9 | 4.1+ nftables 取代 iptables | 防火墙规则管理 |
| 13 | nftables 与 eBPF 的关系 | 无 | 两种内核包过滤机制对比 | 过滤方案选型 |

## 5. io_uring 网络收发

| 序号 | LWN 文章 | 对应 Rosen | 核心变化 | HFT 关联 |
|------|----------|-----------|---------|---------|
| 14 | io_uring 网络收发接口 | 无 | 5.1+ 异步 IO，替代 epoll | 低延迟事件通知 |
| 15 | io_uring vs epoll：性能对比 | 无 | 多连接场景基准测试 | HFT socket 模型选型 |

## 6. TCP/UDP 性能优化

| 序号 | LWN 文章 | 对应 Rosen | 核心变化 | HFT 关联 |
|------|----------|-----------|---------|---------|
| 16 | MSG_ZEROCOPY：零拷贝发送 | Ch11 | 4.14+ sendmsg 零拷贝 | 减少发送路径拷贝 |
| 17 | TCP zero-copy 接收 | Ch11 | 5.0+ tcp_recvmsg 零拷贝 | 减少接收路径拷贝 |
| 18 | SO_REUSEPORT：多进程负载均衡 | 无 | 3.9+ 多进程/线程共享端口 | 多核行情分发 |
| 19 | TCP 内部优化（TSO/ pacing/ RACK） | Ch11 | 现代拥塞控制与发送节奏 | 交易报文发送时机 |
| 20 | UDP GRO：批量接收 | 无 | 5.0+ UDP 接收聚合 | 组播行情批量处理 |
