# 09 — tc-BPF：流量控制中的 eBPF

> **对应 Rosen:** Ch6（Advanced Routing）/ Ch9（Netfilter）
> **内核版本:** tc-BPF cls 4.1+；direct-action 4.1+

## tc-BPF 是什么

Linux Traffic Control（tc）子系统支持用 eBPF 程序做包分类和动作：
- **cls_bpf**：BPF 分类器，替代 u32/fw 等 传统分类器
- **direct-action**：BPF 程序直接返回动作（TC_ACT_OK/SHOT/REDIRECT），不需要单独的 filter action 模块

## 挂载点

| 挂载点 | 方向 | 作用 |
|--------|------|------|
| tc ingress | 收包入口 | 在协议栈之前分类/丢弃/修改包 |
| tc egress | 发包出口 | 在 qdisc 之后、驱动之前分类/修改包 |

## tc ingress vs XDP

| 维度 | XDP | tc ingress |
|------|-----|-----------|
| 挂载位置 | 驱动层（sk_buff 之前） | 协议栈入口（sk_buff 之后） |
| 数据结构 | xdp_buff（轻量） | sk_buff（完整元数据） |
| 可访问信息 | 包内容 + RX 队列索引 | 包内容 + sk_buff 全部元数据 |
| 能力 | 丢/转/改包 | 丢/转/改包 + 可设置 skb mark/priority |
| 性能 | 最高 | 略低（已分配 sk_buff） |

## 使用示例

```bash
# 加载 tc-BPF 程序到网卡 ingress
tc qdisc add dev eth0 clsact
tc filter add dev eth0 ingress bpf da obj filter.o sec tc-ingress

# 查看
tc filter show dev eth0 ingress
```

```c
// tc-BPF 程序：按目标端口标记包
SEC("tc-ingress")
int tc_ingress(struct __sk_buff *skb) {
    void *data_end = (void *)(long)skb->data_end;
    void *data = (void *)(long)skb->data;
    struct ethhdr *eth = data;
    // ... 解析 IP/TCP 头
    if (tcp->dest == htons(9090)) {
        skb->mark = 0x9090;  // 标记行情流
        return TC_ACT_OK;
    }
    return TC_ACT_OK;  // 放行
}
```

## HFT 关联

| 场景 | tc-BPF 用途 |
|------|------------|
| 行情流标记 | 按 DSCP/端口设置 skb->mark，配合路由策略 |
| 发包控制 | tc egress 限制非交易流量带宽 |
| 包丢弃 | 丢弃不符合规则的包，在协议栈前拦截 |
| 延迟测量 | tc-BPF 打时间戳到 skb->cb |
