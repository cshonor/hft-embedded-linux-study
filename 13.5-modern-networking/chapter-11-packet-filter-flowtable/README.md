# Chapter 11: 包过滤与 Flowtable

> 来源：kernel-docs（packet filter + nf_flowtable）
> 对标：Rosen（无 flowtable，3.x 仅 Netfilter 慢路径）

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [packet-filter](notes/01-packet-filter.md) | kernel-docs：BPF 包过滤、socket filter、sk_filter |
| 2 | [nf-flowtable](notes/02-nf-flowtable.md) | kernel-docs：flowtable 快速转发、连接状态、卸载到硬件 |

## HFT 关联

- **flowtable 快速路径**：已建立连接的包跳过 Netfilter 规则匹配，直接转发，延迟降低 50%+
- **硬件卸载**：flowtable 可卸载到 SmartNIC，实现线速转发
- **socket filter**：BPF socket filter 在收包路径提前过滤，减少无关包到达应用层
- **HFT 应用**：HFT 服务器一般不用 flowtable（不做转发），但 BPF socket filter 可用于过滤行情数据

## 交叉引用

- `13.5-modern-networking/chapter-08-ebpf-cgroup-bpf/`：BPF 包过滤基础
- `13.5-modern-networking/chapter-10-nftables/`：nftables 与 flowtable 协同
