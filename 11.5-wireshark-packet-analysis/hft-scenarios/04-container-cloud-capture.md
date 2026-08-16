# HFT 场景 04：容器/云环境抓包

> [总览](./00-overview.md) · [基础：ch02 流量监控](../chapter-02-traffic-monitor/chapter-summary.md) · [基础：ch06 tshark/tcpdump](../chapter-06-tshark-tcpdump/chapter-summary.md)

**核心问题**：交易系统越来越多部署在容器/K8s 中，overlay 网络（VXLAN/ Geneve）和 eBPF 网络插件（Cilium/Calico）使抓包变得复杂。Wireshark 需要特殊技巧才能看到真实流量。

## 1. 容器网络模型

```
┌─────────────────────────────────────┐
│  Host                              │
│  ┌─────────┐  ┌─────────┐         │
│  │ Container│  │ Container│        │
│  │  eth0   │  │  eth0   │         │
│  └────┬────┘  └────┬────┘         │
│       │ veth0      │ veth1         │
│  ┌────┴───────────┴────┐          │
│  │  docker0 / cni0     │ ← 桥接   │
│  │  (or vxlan/calico)  │          │
│  └──────────┬──────────┘          │
│             │ eth0                 │
└─────────────┼─────────────────────┘
              │ 物理网络
```

| 网络模式 | 抓包位置 | 看到的包 |
|----------|---------|---------|
| **Bridge (docker0)** | docker0 或 veth | 容器间流量 + NAT 后的流量 |
| **Host 网络** | eth0 | 直接看到容器流量（无 NAT） |
| **Overlay (VXLAN)** | 物理网卡 | VXLAN 封装包（需解封装） |
| **Calico (BGP)** | 物理网卡 | 直接路由，无封装 |
| **Cilium (eBPF)** | 物理网卡 | 可能被 eBPF 重定向，看不到 |

## 2. Docker 抓包

### 方法 A：在容器内安装 tcpdump

```bash
# 进入容器抓包
docker exec -it <container> tcpdump -nni eth0 -w /tmp/capture.pcap

# 复制出来
docker cp <container>:/tmp/capture.pcap ./capture.pcap
wireshark capture.pcap
```

### 方法 B：找 veth 接口在宿主机抓（推荐）

```bash
# 找到容器的 veth 接口
docker inspect -f '{{.State.Pid}}' <container>
# 假设 PID=12345

# 通过 PID 找 veth
nsenter -t 12345 -n ip link
# 输出形如: 12: eth0@if13: <BROADCAST,MULTICAST,UP,LOWER_UP>

# 在宿主机上抓对应 veth
# @if13 表示对端接口 index=13
ip link | grep "^13:"
# 假设是 veth0a3b2c1

sudo tcpdump -nni veth0a3b2c1 -w container_traffic.pcap
```

### 方法 C：在 docker0 桥接上抓

```bash
# 抓所有容器流量（但看不到容器间直接的转发流量）
sudo tcpdump -nni docker0 -w all_containers.pcap

# 用 nsenter 在容器网络命名空间抓（最干净）
nsenter -t $(docker inspect -f '{{.State.Pid}}' <container>) -n \
  tcpdump -nni eth0 -w /tmp/capture.pcap
```

## 3. Kubernetes 抓包

### Pod 内抓包

```bash
# 找到 Pod 所在的 Node 和容器 ID
kubectl get pod <pod> -o wide

# SSH 到 Node，找到容器
crictl ps | grep <pod>

# 进入容器网络命名空间
crictl inspect <container_id> | grep pid
# 假设 PID=67890

nsenter -t 67890 -n tcpdump -nni eth0 -w /tmp/pod_capture.pcap
```

### 抓 cni0 / flannel.1 / calico 接口

```bash
# Bridge 模式（flannel）
tcpdump -nni cni0 -w cni_traffic.pcap

# VXLAN 模式（flannel）
tcpdump -nni flannel.1 -w vxlan_traffic.pcap

# Calico
tcpdump -nni tunl0 -w calico_traffic.pcap

# Cilium — 可能抓不到（eBPF 旁路），见下文
```

### VXLAN 解封装

```bash
# 抓到的 VXLAN 包外层是 UDP 4789
# Wireshark 自动解封装，但 tshark 需要指定
tshark -r vxlan.pcap -Y "vxlan" -V

# 提取内层包
tshark -r vxlan.pcap -Y "vxlan" -T fields \
  -e vxlan.inner_ip.src -e vxlan.inner_ip.dst -e vxlan.inner_tcp.dstport
```

| Wireshark 设置 | 说明 |
|---------------|------|
| Edit → Preferences → Protocols → VXLAN | 确保启用 |
| 右键 → Decode As → UDP → VXLAN | 手动指定解码 |
| `udp.port == 4789` | 过滤 VXLAN |

## 4. Cilium/eBPF 网络的挑战

Cilium 用 eBPF 在内核层处理网络，部分流量**不经过传统网络栈**，tcpdump 可能抓不到。

| 场景 | tcpdump 能否抓到 | 解决方案 |
|------|----------------|---------|
| Pod → Service | 不能（eBPF 重定向） | Cilium 内置抓包 |
| Pod → Pod（同 Node） | 不能（eBPF 快速路径） | Cilium 内置抓包 |
| Pod → 外部 | 能（经过 eth0） | 正常 tcpdump |

### Cilium 抓包

```bash
# Cilium 内置抓包工具
cilium monitor --type drop
cilium monitor --type trace

# 或使用 cilium tcpdump（Cilium 1.9+）
cilium tcpdump <pod-name> -w /tmp/cilium_capture.pcap
```

## 5. 云环境抓包

### AWS

| 方案 | 说明 |
|------|------|
| VPC Flow Logs | 只记录元数据，无 payload |
| Traffic Mirroring | 发送到指定 ENI，可抓完整包 |
| 在 EC2 上 tcpdump | 只能看到到达本机的包 |

```bash
# AWS Traffic Mirroring 配置后，在目标 EC2 上抓
sudo tcpdump -nni eth0 -w mirrored.pcap
```

### GCP / Azure

```bash
# GCP: Packet Mirroring
# Azure: Network Watcher Packet Capture
# 都需要在控制台配置镜像目标
```

## 6. HFT 容器化抓包策略

| 层级 | 抓什么 | 工具 |
|------|--------|------|
| 物理网卡 | 真实线缆流量 | tcpdump + SPAN |
| Host veth | 容器进出流量 | tcpdump on veth |
| Container eth0 | 容器视角流量 | nsenter + tcpdump |
| Overlay | 跨 Node 流量 | tcpdump on flannel.1/cni0 |
| Service | K8s Service 流量 | Cilium monitor 或 iptables TRACE |

## 7. HFT 自测题

1. Docker Bridge 模式下，容器访问外部 IP，在 docker0 上抓到的源 IP 是容器的还是宿主机的？为什么？
2. VXLAN 包在 Wireshark 中显示为 UDP 4789，如何看到内层的 TCP 包？
3. Cilium 环境下 `tcpdump -i cni0` 抓不到 Pod 间流量，原因是什么？如何抓到？
4. HFT 交易系统容器化后，为什么建议在**物理网卡**上抓包而不是容器 eth0？

## 交叉引用

- [基础：流量监控](../chapter-02-traffic-monitor/chapter-summary.md)
- [基础：tshark/tcpdump](../chapter-06-tshark-tcpdump/chapter-summary.md)
- [HFT 场景 03：内核旁路](./03-kernel-bypass-limitations.md)
- [HFT 场景 05：eBPF 对比](./05-ebpf-vs-wireshark.md)
- [HFT 模块：内核网络](../../12-kernel-networking/)
- [HFT 模块：BPF 可观测性](../../06.7-bpf-observability/)
