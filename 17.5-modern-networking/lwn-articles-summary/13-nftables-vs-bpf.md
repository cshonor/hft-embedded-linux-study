# 13 — nftables 与 eBPF 的关系

> **对应 Rosen:** 无
> **内核版本:** nftables 3.13+；eBPF 网络 4.x+

## 两种内核包过滤机制

| 维度 | nftables | eBPF（XDP/tc-BPF） |
|------|---------|-------------------|
| 设计目标 | 通用防火墙/NAT | 可编程数据路径 |
| 执行位置 | Netfilter hook（协议栈内） | XDP hook 或 tc 层 |
| 编程模型 | 声明式规则（nft 语法） | 命令式程序（C→BPF） |
| 灵活性 | 规则匹配（有限） | 任意逻辑（图灵完备） |
| 性能 | 比 iptables 快，但比 XDP 慢 | XDP 最快，tc-BPF 次之 |
| 适用场景 | 防火墙/NAT/安全策略 | 高性能包处理/过滤/监控 |

## 何时用哪个

| 场景 | 推荐 | 原因 |
|------|------|------|
| 防火墙规则（端口/IP过滤） | nftables | 声明式简单，维护方便 |
| NAT/路由 | nftables | Netfilter NAT 成熟稳定 |
| 超低延迟包过滤 | XDP-BPF | sk_buff 之前处理 |
| 行情早过滤/早丢弃 | XDP-BPF | 不分配 sk_buff |
| 包标记/分类 | tc-BPF | 可设置 skb 元数据 |
| 速率限制 | nftables 或 tc-BPF | nftables limit 简单，tc-BPF 更灵活 |
| 包内容修改 | XDP-BPF | 可增删头部 |
| 可观测/监控 | eBPF（tracing） | 可 hook 任意内核函数 |

## HFT 实践

HFT 系统通常两者都用：
- **nftables**：基础防火墙（管理通道保护、白名单、NAT）
- **XDP-BPF**：行情流早过滤（组播地址过滤、端口过滤）
- **tc-BPF**：发包控制（交易流标记、带宽控制）

```
行情包到达 → XDP-BPF（早过滤）→ 协议栈 → nftables（安全规则）→ socket
交易包发送 → socket → tc-BPF（标记）→ qdisc → nftables（NAT）→ 驱动
```
