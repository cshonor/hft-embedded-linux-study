# 07 — Netfilter/nftables

> **Bootlin 课程模块：** Netfilter/nftables
> **对应 Rosen:** Ch9

## nftables 基本操作

```bash
# 创建表
nft add table inet filter

# 创建链
nft add chain inet filter input '{ type filter hook input priority 0 \; }'

# 添加规则
nft add rule inet filter input iif "lo" accept
nft add rule inet filter input tcp dport 22 accept
nft add rule inet filter input tcp dport 9090 ip saddr 10.0.0.0/24 accept
nft add rule inet filter input counter drop

# 查看规则
nft list ruleset

# 保存/恢复
nft list ruleset > /etc/nftables.conf
nft -f /etc/nftables.conf
```

## HFT 防火墙规则示例

```
# 行情源白名单
nft add set inet filter md_sources '{ type ipv4_addr \; flags interval \; }'
nft add element inet filter md_sources '{ 10.0.1.0/24, 10.0.2.0/24 }'
nft add rule inet filter input udp dport 9090 ip saddr @md_sources accept

# 交易端口保护
nft add set inet filter trade_clients '{ type ipv4_addr \; }'
nft add element inet filter trade_clients '{ 10.0.0.5, 10.0.0.6 }'
nft add rule inet filter input tcp dport 8001 ip saddr @trade_clients accept

# 速率限制（非交易流量）
nft add rule inet filter input icmp limit 10/second accept
```

## iptables → nftables 迁移

```bash
# iptables-nft 兼容层（旧 iptables 命令翻译为 nft 规则）
update-alternatives --set iptables /usr/sbin/iptables-nft

# 或使用 iptables-translate 直接翻译
iptables-translate -A INPUT -p tcp --dport 9090 -j ACCEPT
# 输出: nft add rule inet filter input tcp dport 9090 counter accept
```
