# Rosen《Linux Kernel Networking》过时章节 → 现代替代映射

> Rosen 基于Linux 3.x（2014）。设计思想可借鉴，但大量数据结构、API、架构已变。

## 过时映射表

| Rosen 章节 | 书中内容 | 现代变化 | 替代资料 |
|-----------|---------|---------|---------|
| Ch1 Introduction | 网络栈全景、sk_buff、net_device | sk_buff 持续演进，新增 xdp_buff/page_pool；net_device 结构变化 | kernel-docs: `sock-sk-buff.rst` + `txrx.rst` |
| Ch2 Netlink | Netlink socket API | 基本可用，但新增 genetlink 家族 | kernel-docs: `Documentation/userspace-api/netlink/` |
| Ch3 ICMP | ICMP 协议处理 | 基本稳定 | — |
| Ch4 IPv4 | IPv4 协议栈实现 | 路由查找结构变化（fib_trie → fib_info 重构） | LWN 02: page_pool |
| Ch5 IPv4 Routing | 路由子系统 | 路由表数据结构重构，multi-path 路由改进 | kernel-docs: `Documentation/networking/route-policies.rst` |
| Ch6 Advanced Routing | 策略路由、traffic control | tc-BPF 取代部分 tc filter | LWN 09: tc-BPF |
| Ch7 Neighbouring | ARP/邻居子系统 | 基本稳定，有性能优化 | — |
| Ch8 IPv6 | IPv6 协议栈 | 基本稳定 | — |
| Ch9 Netfilter | Netfilter/iptables 架构 | **完全过时**：nftables 替代 iptables | LWN 12-13: nftables |
| Ch10 IPsec | IPsec/XFRM 框架 | 基本稳定，有性能优化 | — |
| Ch11 Layer 4 | TCP/UDP/sk_buff/socket | **大量过时**：无零拷贝、无 io_uring、无 UDP GRO | LWN 16-20: TCP/UDP 性能 |
| Ch12 Wireless | 无线网络子系统 | 不在 HFT 关注范围 | 跳过 |
| Ch13 InfiniBand | RDMA/InfiniBand | RDMA 持续演进（Soft-RoCE） | kernel-docs: `Documentation/infiniband/` |
| Ch14 Advanced Topics | NAPI/RPS/RFS/XPS | **部分过时**：无 XDP、无 threaded NAPI、无 busy polling 深入 | LWN 01/05-08: XDP + NAPI |

## HFT 核心路径：Rosen → 现代补充

```
Rosen Ch1  收包路径全景
  → LWN 01-04: NAPI现代化 + page_pool + XDP收包路径
  → kernel-docs: napi.rst + page_pool.rst

Rosen Ch11 传输层（socket/sk_buff/TCP/UDP）
  → LWN 16-20: MSG_ZEROCOPY + TCP zero-copy + UDP GRO
  → kernel-docs: msg_zerocopy.rst

Rosen Ch14 高级主题（NAPI/RPS）
  → LWN 05-08: XDP全景 + AF_XDP + XDP vs DPDK
  → kernel-docs: scaling.rst

Rosen Ch9 Netfilter（已过时）
  → LWN 12-13: nftables 架构与迁移
```

## 与 18-DPDK 的衔接

| 维度 | 17.5 XDP/AF_XDP（内核态旁路） | 18 DPDK（用户态旁路） |
|------|----------------------------|---------------------|
| 数据路径 | 内核网络栈内，XDP hook 处早处理 | 完全绕过内核，用户态驱动 |
| 零拷贝 | AF_XDP 到用户态 | VFIO + 大页 + 用户态驱动 |
| 适用场景 | 内核仍参与部分处理，灵活性高 | 极致延迟，完全旁路 |
| HFT 选型 | 中低频策略 / 行情过滤 | 超低延迟交易 / co-location |
