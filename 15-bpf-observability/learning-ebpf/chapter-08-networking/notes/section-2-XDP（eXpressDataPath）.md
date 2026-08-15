# XDP（eXpress Data Path）

### 2.1 五种返回码

| 返回码 | 含义 |
|---|---|
| `XDP_PASS` | 放行，继续走正常协议栈 |
| `XDP_DROP` | 立即丢包，最快路径 |
| `XDP_TX` | 从**进入的同一网卡**发回去 |
| `XDP_REDIRECT` | 重定向到**另一网卡**或 CPU/map |
| `XDP_ABORTED` | 异常中止（程序错误），会触发异常追踪事件 |

### 2.2 上下文：`struct xdp_md`

```c
struct xdp_md {
    __u32 data;         // 包数据起始（以太网头开始）
    __u32 data_end;     // 包数据结束
    __u32 data_meta;    // 包前可写的元数据区，供后续层（如 TC）读取
    __u32 ingress_ifindex;
};
```

没有 `len` 字段——长度就是 `data_end - data`。

### 2.3 包解析范式（边界检查是验证器硬性要求）

```c
unsigned char lookup_protocol(struct xdp_md *ctx) {
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;
    struct ethhdr *eth = data;
    // 第一层：先确认以太网头完整落在包内，才能读 eth->h_proto
    if (data + sizeof(struct ethhdr) > data_end)
        return 0;
    if (bpf_ntohs(eth->h_proto) == ETH_P_IP) {
        // 第二层：iphdr 紧跟以太网头，同样先检查边界
        struct iphdr *iph = data + sizeof(struct ethhdr);
        if (data + sizeof(struct ethhdr) + sizeof(struct iphdr) <= data_end)
            return iph->protocol;   // 6=TCP, 17=UDP
    }
    return 0;
}
```

三条铁律（第 6 章验证器知识的直接应用）：

1. **每解一层头都要做 `指针 + 头长 <= data_end` 检查**，漏了直接被验证器拒载（`invalid mem access`）
2. **字节序**：网络上字段是网络序（大端），比较/使用前要 `bpf_ntohs`/`bpf_ntohl` 转主机序
3. 检查条件不能 off-by-one：是 `>` 还是 `>=` 要想清楚（第 6 章 `pkt_end` 案例重演）

### 2.4 XDP 丢包实验（书中 hello 示例）

对 ICMP 包直接 `XDP_DROP`，ping 立即不通——丢在驱动层，连协议栈的第一行代码都没执行。对照：iptables 的 DROP 要等包走完大半协议栈。这就是"early drop"的价值。

### 2.5 XDP 负载均衡（本章主战例）

思路：把到达的请求按随机数分给一组后端，**改包头**后原路 `XDP_TX` 发回。核心步骤：

```c
// 1. 随机选后端（bpf_get_prandom_u32 对 2 取模，模拟 RR）
__u32 backend = bpf_get_prandom_u32() % 2;

// 2. 改目的 MAC 为后端 MAC（后端通过 ARP 学到的 MAC 硬编码在数组里）
memcpy(eth->h_dest, backends[backend].mac, ETH_ALEN);

// 3. 改 IP：swap 源/目的地址（乒乓模式）
swap_src_dst_ip(iph);        // 借助结构体赋值语句一次拷贝 32bit

// 4. IP 头校验和必须重算（地址变了）
__u32 iph_csum = 0;
iph->check = 0;
#pragma unroll
for (int i = 0; i < sizeof(*iph) >> 1; i++)
    iph_csum += ((__u16 *)iph)[i];      // 16bit 一组累加
iph->check = ~((iph->sum & 0xffff) + (iph->sum >> 16));

return XDP_TX;
```

要点与坑：

- **校验和增量重算**：`iph_csum` 必须在最后移位折叠，先加后折叠顺序不能乱；ICMP 回显响应还要把 `type` 从 8 改 0 并重算 ICMP 校验和
- **XDP_TX 要求后端可达同一网段**（能直接 ARP 到），否则要改用 `XDP_REDIRECT` + `bpf_redirect` 到另一网卡的 ifindex
- **网络命名空间注意点**：书中实验在两个 netns 里做，`ip netns exec` 进去看后端；`bpftool prog load` 加载 XDP 程序要指定 netns（默认在 host ns，看不见另一 ns 的接口）
- XDP 程序附加用 `ip link set dev eth0 xdp obj xxx.o sec xdp` 或 `bpftool net attach xdp name xxx dev eth0`
- **一个接口只能附加一个 XDP 程序**（第 7 章）：想要"多个逻辑程序"要么用 `xdptools`/libxdp 的多程序分发，要么合成一个大程序

### 2.6 XDP offload：让网卡自己跑 eBPF

- 支持的网卡（如 Netronome nfp）可把 eBPF 程序**直接加载进网卡固件**执行
- 效果：主机 CPU 零开销——包在进入主机内存之前就被处理/丢弃
- 限制：能用的 helper 子集更小；某些返回码（如涉及 map 重定向的操作）取决于驱动支持
