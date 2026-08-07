# Bootlin 网络子系统训练讲义

> Bootlin 公开培训课程，跟随 LTS 内核迭代更新
> 官网：https://bootlin.com/training/networking/

## 课程模块

| 序号 | 模块 | 内容 | 对应 Rosen | HFT 关联 |
|------|------|------|-----------|---------|
| 01 | 网络栈架构 | Linux 网络栈全景、数据结构（sk_buff/net_device） | Ch1 | 基础框架 |
| 02 | 收包路径 | NAPI → page_pool → sk_buff → 协议栈 → socket | Ch1/Ch11 | 收包延迟分析 |
| 03 | 发包路径 | 协议栈 → qdisc → 驱动 → NIC DMA | Ch11 | 发包延迟分析 |
| 04 | XDP | XDP hook、AF_XDP、redirect | 无 | 内核旁路 |
| 05 | eBPF 网络 | tc-BPF、XDP-BPF、cgroup-BPF | 无 | 包处理可编程性 |
| 06 | Traffic Control | qdisc、class、filter、BPF filter | Ch6 | 流量调度 |
| 07 | Netfilter/nftables | nftables 架构、与 iptables 迁移 | Ch9 | 包过滤 |
| 08 | 调试工具 | tcpdump、ss、ethtool、dropwatch、perf | 无 | 网络延迟定位 |
| 09 | 性能调优 | RPS/RFS/XPS、busy polling、零拷贝 | Ch14 | 收发性能优化 |

## 实验指引

- 树莓派 5（BCM2712, AArch64）可用 ethtool 调整网口参数
- XDP 实验需网卡驱动支持（树莓派自带网卡有限制，可配合 veth 虚拟网卡实验）
- AF_XDP 实验：虚拟网卡 + xdp-tools 套件
