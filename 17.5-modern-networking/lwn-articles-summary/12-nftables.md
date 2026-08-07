# 12 — nftables 架构与迁移

> **对应 Rosen:** Ch9（Netfilter/iptables）
> **内核版本:** nftables 3.13+（初始）；4.1+（用户态工具成熟）

## 为什么需要 nftables

iptables 的问题：
- 每个 table/chain 是独立的内核模块（filter/nat/mangle/raw）
- 规则匹配线性扫描，大量规则时性能差
- IPv4 和 IPv6 需要两套规则（iptables / ip6tables）
- 每个匹配条件（-m tcp / -m state）是独立模块

## nftables 架构

nftables 统一了 Netfilter 前端：
- **统一语法**：IPv4/IPv6/ARP/Bridge 一套规则
- **虚拟机**：规则编译为字节码，在内核 nft VM 中执行
- **集合和映射**：原生支持 IP 集合，无需额外 ipset
- **无状态表**：table/chain 由用户态定义，不需要内核模块

## nftables vs iptables

| 维度 | iptables | nftables |
|------|---------|---------|
| 语法 | -A INPUT -p tcp --dport 80 -j ACCEPT | add rule inet filter input tcp dport 80 accept |
| 地址族 | ipv4/ip6 分开 | inet 统一（同时匹配 v4/v6） |
| 规则集 | 不支持 | 原生支持（集合/映射/区间） |
| 性能 | 线性扫描 | 可优化（集合用哈希/区间树） |
| 表/链 | 内核预定义 | 用户自定义 |
| 模块 | 每个匹配条件一个内核模块 | nft VM 统一执行 |

## 迁移示例

```bash
# iptables 规则
iptables -A INPUT -p tcp --dport 9090 -s 10.0.0.0/24 -j ACCEPT

# 等效 nftables 规则
nft add rule inet filter input tcp dport 9090 ip saddr 10.0.0.0/24 accept

# 集合用法（替代 ipset）
nft add set inet filter whitelist { type ipv4_addr \; flags interval \; }
nft add element inet filter whitelist { 10.0.0.0/24, 192.168.1.0/24 }
nft add rule inet filter input ip saddr @whitelist accept
```

## HFT 关联

| 场景 | nftables 用途 |
|------|-------------|
| 行情源白名单 | 集合管理交易所 IP，一条规则匹配 |
| 交易端口保护 | 只允许特定 IP 连接交易端口 |
| 速率限制 | limit 规则限制 ICMP/DNS 等非交易流量 |
| 兼容性 | iptables-nft 兼容层，旧脚本可平滑迁移 |
