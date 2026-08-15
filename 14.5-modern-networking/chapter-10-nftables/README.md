# Chapter 10: nftables

> 来源：Bootlin（nftables 概述）+ LWN（nftables + vs BPF）
> 对标：Rosen Ch9（iptables → nftables）

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [nftables-bootlin](notes/01-nftables-bootlin.md) | Bootlin：nftables 架构、table/chain/rule、set/map |
| 2 | [nftables-lwn](notes/02-nftables-lwn.md) | LWN：nftables 设计、虚拟机、规则编译 |
| 3 | [nftables-vs-bpf](notes/03-nftables-vs-bpf.md) | LWN：nftables vs BPF、适用场景、性能对比 |

## HFT 关联

- **iptables → nftables**：6.x 内核 nftables 全面替代 iptables，规则匹配更快
- **规则集大小**：nftables set/map 原生支持集合查找，大量规则时性能优于 iptables 线性遍历
- **nftables vs BPF**：nftables 适合固定规则（访问控制），BPF 适合动态逻辑（流量分析）
- **HFT 场景**：交易服务器用 nftables 做基础防火墙，BPF 做精细流量控制

## 交叉引用

- `14.5-modern-networking/chapter-08-ebpf-cgroup-bpf/`：BPF 替代方案
- `14.5-modern-networking/chapter-11-packet-filter-flowtable/`：flowtable 快速转发
