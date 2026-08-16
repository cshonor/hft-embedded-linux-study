# TC（Traffic Control）层

### 3.1 与 XDP 的对照

| | XDP | TC |
|---|---|---|
| 上下文 | `xdp_md`（裸包） | `__sk_buff`（解析过的元数据 + 包） |
| 位置 | 驱动层，最早 | ingress/egress 两个方向都可挂 |
| 程序类型 | `BPF_PROG_TYPE_XDP` | `BPF_PROG_TYPE_SCHED_CLS` |
| 一接口程序数 | 1 个 | **可多个，按 classid 顺序执行**（`bpf_graft`） |
| 改包 | 手动改 raw 数据 | helper 辅助（`bpf_skb_store_bytes` 等） |
| 典型用途 | 早期丢包、LB、DDoS | 精细策略、带宽控制、包改写 |

`sk_buff` 是内核网络栈最重要的结构之一，字段极多（mark、priority、cb[]、协议头指针等）。eBPF 里不能直接访问所有字段，但比 `xdp_md` 信息丰富得多。

### 3.2 TC 返回码

| 返回码 | 含义 |
|---|---|
| `TC_ACT_SHOT` | 丢包 |
| `TC_ACT_UNSPEC` | 未指定（继续走分类器链） |
| `TC_ACT_OK` | 放行，继续协议栈 |
| `TC_ACT_REDIRECT` | 重定向（配合 `bpf_redirect`/`bpf_clone_redirect`） |

### 3.3 ping-pong 响应示例（TC 改包回送）

用 TC 在 ingress 对 ICMP echo request 就地改写成 echo response 并从同一口发回，请求根本不进协议栈：

```c
SEC("classifier")
int pingpong(struct __sk_buff *skb) {
    void *data = (void *)(long)skb->data;
    void *data_end = (void *)(long)skb->data_end;
    // 边界检查范式同 XDP（sk_buff 也有 data/data_end）
    struct ethhdr *eth = data;
    if (data + sizeof(*eth) > data_end) return TC_ACT_SHOT;
    if (eth->h_proto != bpf_htons(ETH_P_IP)) return TC_ACT_OK;
    struct iphdr *iph = data + sizeof(*eth);
    if (data + sizeof(*eth) + sizeof(*iph) > data_end) return TC_ACT_SHOT;
    if (iph->protocol != IPPROTO_ICMP) return TC_ACT_OK;

    // 1. 交换 MAC
    swap_mac(eth);
    // 2. 交换 IP
    swap_ip_addresses(iph);
    // 3. ICMP type: 8(request) → 0(response)，并更新 ICMP 校验和
    update_icmp_type(skb, 8, 0);   // helper：增量改字节并修校验和

    // 4. 克隆一份从进入的接口发回，本体丢弃
    bpf_clone_redirect(skb, skb->ifindex, 0 /*ingress 方向*/);
    return TC_ACT_SHOT;
}
```

- `bpf_clone_redirect`：**复制**一份 skb 重定向发送，原 skb 留给本程序处置（所以最后 SHOT 掉）——这是 TC 层"边转发边留痕"的常用手法
- 附加：`tc qdisc add dev eth0 clsact` + `tc filter add dev eth0 ingress bpf da obj pingpong.o sec classifier`

### 3.4 TC 的多程序协作

TC 的 clsact qdisc 允许在 filter 链上挂多个 eBPF 程序，**按顺序执行**，前一个的返回码决定是否继续走链。Cilium 等大型方案正是靠这个把"每个容器一个策略程序"串起来执行——XDP 做不到这一点（一接口一程序）。
