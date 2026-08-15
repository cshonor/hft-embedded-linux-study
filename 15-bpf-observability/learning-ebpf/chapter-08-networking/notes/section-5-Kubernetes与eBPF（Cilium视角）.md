# Kubernetes 与 eBPF（Cilium 视角）

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
