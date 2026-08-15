# Learning eBPF · 第 8 章：eBPF 与网络

> 底本：`../LEARNING-EBPF-BILINGUAL.pdf`。网络是 eBPF 最出成绩的领域：XDP 在网卡驱动层丢包、TC 在流量控制层改包、uprobe 钩 SSL 看明文、Cilium 用 eBPF 重写整个 K8s 数据面。本章回答：eBPF 能挂在网络路径的哪些位置？每个位置能做什么、怎么做？

## 本章目标

1. 掌握 XDP：五种返回码、`xdp_md` 上下文、包解析边界检查范式、负载均衡实战、硬件 offload
2. 掌握 TC 层：`sk_buff` 上下文、TC_ACT 返回码、与 XDP 的分工、多程序串联
3. 学会用 uprobe 钩 SSL_read/SSL_write 观测加密流量的明文（entry/retprobe 配对模式）
4. 理解 K8s 场景：iptables 的 O(n) 困境 vs eBPF hash map O(1)、Cilium 多程序协作、NetworkPolicy、透明加密与身份认证

## 1. eBPF 在网络路径上的位置

```
数据包到达网卡
   │
   ▼
┌──────────────┐ 最早的挂点，此时还没建 sk_buff
│  XDP         │ ← 驱动层/网卡硬件，只收 xdp_md（裸包）
└──────────────┘
   │
   ▼
┌──────────────┐ 流量控制层，包已封装为 sk_buff
│  TC ( ingress)│ ← 可改包、可重定向、可多程序顺序执行
└──────────────┘
   │
   ▼
  协议栈 → socket 层（SCHED_CLS / SOCK_FILTER 等）
```

- **XDP 收到的是"裸包"**：只有 data/data_end 指针，没有解析好的字段，一切自己动手——换来的是最早、最快的处理时机
- **TC 收到的是 `sk_buff`**：内核已解析过包头，含元数据（mark、优先级等），改动更方便但位置更靠后、开销更大
- 同一功能往往两层都能做：DDoS 丢包放 XDP（早丢省 CPU），精细策略放 TC（信息全）

## 2. XDP（eXpress Data Path）

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

## 3. TC（Traffic Control）层

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

## 4. uprobe 钩 SSL：看加密流量的明文

### 4.1 思路

TLS 加密后，tcpdump 看到的全是密文。但任何程序最终都要调用 SSL 库收发明文——**在 `SSL_read`/`SSL_write` 上挂 uprobe，明文自然到手**：

- `SSL_write(SSL *ssl, const void *buf, int num)`：entry 时 buf 已是**待发送的明文**，直接读
- `SSL_read` 不同：**entry 时 buf 还是空的**，明文要等函数返回后才填进去——所以必须 entry + retprobe **配对**：

```c
// 全局 map：暂存 "SSL 指针 → 缓冲区指针"
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __type(key,   __u64);            // SSL*（第一参数，两个探针都能看到）
    __type(value, __u64);            // buf 指针（第二参数 PT_REGS_PARM2）
    __uint(max_entries, 1024);
} ssl_read_context SEC(".maps");

SEC("uprobe/SSL_write")
int BPF_KPROBE(ssl_write, const void *ssl, const void *buf, int num) {
    process_SSL_data(ctx, ssl, false, buf, num);   // entry 直接读明文
    return 0;
}

SEC("uprobe/SSL_read")
int BPF_KPROBE(ssl_read_enter, const void *ssl, void *buf) {
    // 只记下 buf 指针，现在还没数据
    bpf_map_update_elem(&ssl_read_context, &ssl, &buf, 0);
    return 0;
}

SEC("uretprobe/SSL_read")
int BPF_KPROBE(ssl_read_exit) {
    __u64 ret = PT_REGS_RC(ctx);          // 返回值 = 实际读取字节数
    // 以 ssl* 为 key 反查 buf 指针，再从 buf 读 ret 字节 → 明文
    ...
    bpf_map_delete_elem(&ssl_read_context, &ssl);
    return 0;
}
```

`process_SSL_data` 是共用的输出函数：从 `buf` 用 `bpf_probe_read_user_bytes` 拷贝到栈/PERFBUF，发给用户态。

### 4.2 uprobe 的四大坑（第 7 章埋的，这里全踩）

1. **架构相关**：`SSL_write` 的 buf 是第二参数，x86 用 `PT_REGS_PARM2`，ARM 上可能不同——`BPF_KPROBE` 宏 + `bpf_trace_printk` 打印各参数实测确认
2. **库不可控**：目标进程用哪个 libssl.so、路径在哪，取决于发行版/编译方式，SEC 名里的库路径要写对（可用 `ldd` 查）
3. **静态链接**：二进制把 SSL 静态链进去就没有 `.so` 可挂——只能对二进制本身的符号挂 uprobe（要求非 strip）
4. **Go < 1.17 栈传参**：旧版 Go 编译的程序参数不走寄存器走栈，`PT_REGS_PARMx` 拿不到——Go 程序要么升级 1.17+，要么改挂系统调用层

### 4.3 更通用的视角

同一模式适用于一切"函数边界即明文边界"的场景：压缩库（zlib 的 `deflate`/`inflate` 前后）、数据库驱动、行情解码函数——**只要符号可见，uprobe 就能看到进出函数的数据**。

## 5. Kubernetes 与 eBPF（Cilium 视角）

### 5.1 iptables 的困境

- K8s 每个 Service 的每个端口 = 一长链 iptables 规则；Service 变更要**全量重写整张规则表**——书中数据：2 万个服务时一次重写要 **5 小时**
- 查找是 **O(n) 线性扫描**规则链；IPVS 模式改善了 LB 但仍有 conntrack 竞争等问题
- kube-proxy 本身还消耗大量 CPU 处理规则同步

### 5.2 eBPF 的解法

- Service 后端列表放 **hash map**：查找 O(1)，后端变更只 `bpf_map_update_elem` 单个键——秒级生效，无全量重写
- 数据面直接在 XDP/TC/socket 层做 DNAT，跳过 iptables/netfilter 大部分路径
- DSR（Direct Server Return）：响应不从 LB 绕行，后端直接回客户端

### 5.3 Cilium：多程序协作的样板

同一台节点上多个 eBPF 程序分工：

```
出向：socket 层程序（cgroup/connect4 改目标地址，做服务发现式重定向）
入向：XDP 程序（LB 后端流量，early processing）
每个接口：TC 程序（NetworkPolicy 执行、标记、转发）
```

- 传统做法里这些功能分散在 iptables/TC/ipvs/conntrack 多个子系统，每个都是独立瓶颈；Cilium 用一套 eBPF map 存共享状态（endpoint、identity、policy），程序间通过 map 协作
- **NetworkPolicy**：K8s 原生策略只有 L3/L4 且实现依赖 iptables；Cilium 扩展到**基于标签身份**（每个 endpoint 有 identity，策略跟身份走，IP 变了策略不变）、**基于 DNS**（如"只允许访问 *.api.twitter.com"）、**L7 规则**（HTTP method/path，靠 eBPF 驱动的代理逐层裁定）
- 这就是"身份"与"位置"解耦——云原生环境 Pod IP 频繁变化下的正确抽象

### 5.4 透明加密与身份认证

- **节点间透明加密**：Cilium 可在 XDP/TC 层对全部节点流量做 IPsec 或 WireGuard 加密，应用零改造（流量经过时按节点身份自动加解密）
- **TLS + 身份**：加密只保证"管道安全"，还要证明"你是谁"——SPIFFE/SPIRE 给每个工作负载发 SVID 身份证书，cert-manager 配合 K8s 签发管理；eBPF 层既能观测 TLS 握手（uprobe SSL），也能在 L3/L4 先行校验对端身份
- 双栈（IPv4+IPv6）注意：书中实验暴露 IPv6 路径下 XDP 重定向与邻居发现的问题——生产双栈环境要分别验证两条路径

## 6. 坑点清单

1. **XDP/TC 包解析漏边界检查** → 验证器直接拒载；每层头一个 if，别嫌啰嗦
2. **改了 IP 地址忘记重算校验和** → 对端静默丢包，ping 都不通，最难查的一类；改 ICMP type 同理要修 ICMP 校验和
3. **校验和折叠顺序**：先 16bit 累加全部字段，最后 `~((sum & 0xffff) + (sum >> 16))` 折叠，顺序错结果错
4. **XDP_TX 只能从进入的网卡发出**；跨网卡必须 `XDP_REDIRECT` + `bpf_redirect(ifindex, flags)`
5. **一个接口只能挂一个 XDP 程序**；需要多程序用 libxdp 或 TC（TC 原生支持链式多程序）
6. **netns 隔离**：XDP 程序加载进哪个网络命名空间，就只能看见那个 ns 的接口和流量；实验环境常用 `ip netns exec` + 指定 ns 加载
7. **uprobe 拿 SSL_read 明文必须 entry+ret 配对**：entry 存 buf 指针到 hash map，retprobe 按 SSL* 取出——单挂 entry 只能看空缓冲区
8. **Go <1.17 程序挂 uprobe 参数取不到**（栈传参）；静态链接无符号；库路径架构相关——uprobe 三坑先排查
9. **性能对比要公平**：XDP 丢包快是因为没建 sk_buff 没进协议栈，拿 XDP 和 iptables 比"每秒丢包数"差几个量级是正常的，不是测量错误

## 7. HFT 关联

- **行情多播入口过滤**：行情 feeds 以 UDP 多播涌入，XDP 在驱动层按源 IP/端口白名单 early drop 无关多播流，不占用协议栈与 CPU——网卡 offload 后甚至不占主机资源
- **交易网卡入口防护**：XDP 一行 `XDP_DROP` 挡掉非交易源的扫描/重放流量，延迟代价是纳秒级；比 iptables（微秒级路径）低两个数量级
- **uprobe 钩行情解码库计时**：在解码函数 entry/exit 挂 uprobe，测"原始字节进 → 订阅回调出"的解码耗时分布，比用户态埋点侵入性小（不用改业务代码）
- **TC egress 做 PTP/心跳优先级**：用 TC 程序给 PTP 报文打 skb->priority，配合 qdisc 保证时钟同步包优先出队
- **跨机东西向加密的性能权衡**：IPsec/WireGuard 透明加密的加解密路径开销要用第 10 章的工具实测，交易面通常专网明文+审计面加密分离

## 8. 自测题

1. XDP 的五种返回码各是什么？`XDP_TX` 和 `XDP_REDIRECT` 的本质区别？
2. 为什么 `xdp_md` 里没有 len 字段？包长怎么算？
3. 写出 XDP 解析到 TCP 头为止需要的三次边界检查。
4. XDP 负载均衡改完目的 IP 后必须做什么？为什么？
5. TC 的上下文和 XDP 有什么不同？TC 哪两个方向可挂程序？
6. `bpf_clone_redirect` 和 `bpf_redirect` 的区别？ping-pong 示例最后为什么返回 TC_ACT_SHOT？
7. 钩 SSL_read 看明文为什么必须 uprobe+uretprobe 配对？用什么做两个探针间的关联 key？
8. 2 万 Service 时 iptables 全量重写要 5 小时，eBPF 方案为什么是秒级？
9. Cilium 在一台节点上分别在哪些层挂了程序、各负责什么？
10. Cilium NetworkPolicy 比 K8s 原生策略强在哪三点？

## 9. 交叉引用

- 第 7 章 `07-程序类型.md`：XDP/SCHED_CLS 程序类型与附加方式、uprobe 基础、一接口一 XDP 程序的限制
- 第 6 章 `06-验证器.md`：边界检查为什么是验证器硬性要求、`pkt_end prohibited` 报错
- 第 9 章 `09-eBPF安全.md`：uprobe 观测与安全审计的关系、syscall 探针可被绕过的问题
- 第 10 章 `10-eBPF编程.md`：用性能工具实测 XDP/TC 的处理开销
