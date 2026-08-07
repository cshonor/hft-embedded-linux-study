# 10 — XDP-BPF：收包路径中的 eBPF

> **对应 Rosen:** 无
> **内核版本:** XDP BPF 4.8+

## XDP-BPF vs tc-BPF

| 维度 | XDP-BPF | tc-BPF |
|------|---------|--------|
| 挂载位置 | 驱动层（sk_buff 之前） | tc ingress/egress（sk_buff 之后） |
| 数据结构 | xdp_buff | sk_buff |
| 能修改 skb 元数据 | 否（无 sk_buff） | 是（mark/priority/queue_mapping） |
| 性能 | 最高 | 次之 |
| 适用场景 | 早过滤/早丢弃/早分类 | 需要 skb 元数据的场景 |

## XDP-BPF 程序能做什么

1. **读取/修改包内容**：直接操作 xdp_buff 的 data/data_end
2. **丢弃包**：返回 XDP_DROP，不分配 sk_buff
3. **重定向**：CPUMAP / DEVMAP / AF_XDP
4. **反弹发送**：XDP_TX（修改 MAC 后原路返回）
5. **修改包**：增删头部（bpf_xdp_adjust_head）

## 实际 HFT 应用：行情组播早过滤

```c
SEC("xdp")
int xdp_md_filter(struct xdp_md *ctx) {
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;

    struct ethhdr *eth = data;
    if ((void*)(eth + 1) > data_end) return XDP_DROP;
    if (eth->h_proto != bpf_htons(ETH_P_IP)) return XDP_PASS;

    struct iphdr *ip = (void*)(eth + 1);
    if ((void*)(ip + 1) > data_end) return XDP_DROP;

    // 只放行特定组播地址的行情流
    if (ip->daddr == bpf_htonl(0xe1000001)) {  // 225.0.0.1
        return XDP_PASS;  // 目标行情流
    }
    return XDP_DROP;  // 丢弃非行情组播
}
```

## 性能数据（参考）

| 操作 | 延迟（cycles） |
|------|---------------|
| XDP DROP（空程序） | ~10 |
| XDP PASS（空程序） | ~20 |
| XDP + 包头解析 | ~50-100 |
| sk_buff 分配 + 协议栈 | ~300+ |

## HFT 关联

XDP-BPF 是 HFT 在内核态做**最早点包处理**的唯一方案：
- 比 tc-BPF 更早（不分配 sk_buff）
- 比 Netfilter 更早（不经过协议栈）
- 可与 AF_XDP 配合实现接近 DPDK 的零拷贝路径
