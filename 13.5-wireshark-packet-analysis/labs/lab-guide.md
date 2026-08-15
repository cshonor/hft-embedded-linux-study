# Wireshark 实验指引

> [README](../README.md) · [速查](../cheatsheet/notes.md) · [HFT 场景](../hft-scenarios/00-overview.md)

本文件提供从基础到 HFT 进阶的实验练习。`.pcap` 文件被 `.gitignore` 忽略，可放在本目录或各章目录。

## 实验 1：首次抓包与协议树（对应 ch01-ch04）

### 目标
- 熟悉 Wireshark 界面
- 理解协议树分层结构
- 掌握捕获过滤器和显示过滤器

### 步骤

```bash
# 1. 开始抓包
# Wireshark: 选择网卡 → Start

# 2. 产生流量
curl http://example.com

# 3. 停止抓包，保存
# File → Save As → labs/lab01-first-capture.pcapng

# 4. 练习过滤器
# 显示 HTTP: http
# 显示某个 IP: ip.addr == x.x.x.x
# 显示 TCP 握手: tcp.flags.syn == 1
```

### 验证清单

- [ ] 能看到 Ethernet → IP → TCP → HTTP 的分层
- [ ] 能用显示过滤器过滤 HTTP 流量
- [ ] 能找到 TCP 三次握手的三个包
- [ ] 能保存为 `.pcapng` 文件

## 实验 2：TCP 握手与挥手分析（对应 ch08）

### 目标
- 理解 TCP 连接建立和关闭过程
- 掌握 TCP 标志位含义
- 学会 Follow TCP Stream

### 步骤

```bash
# 1. 抓包
sudo tcpdump -nni eth0 -w labs/lab02-tcp-handshake.pcapng 'tcp port 80'

# 2. 产生流量
curl http://example.com

# 3. 分析
# Wireshark: 打开 pcap → 找到 SYN 包
# 右键 → Follow → TCP Stream
```

### 分析任务

| 任务 | 过滤器/操作 |
|------|-----------|
| 找三次握手 | `tcp.flags.syn == 1` |
| 找四次挥手 | `tcp.flags.fin == 1` |
| 查看 RTT | 选中 SYN-ACK → 看 `tcp.analysis.ack_rtt` |
| 统计流大小 | Statistics → Conversations |

### 验证清单

- [ ] 能识别 SYN / SYN-ACK / ACK 三个握手包
- [ ] 能识别 FIN / FIN-ACK 挥手过程
- [ ] 理解相对序号和绝对序号的区别
- [ ] 能用 Follow TCP Stream 查看完整流

## 实验 3：协议分析 — DNS + HTTP（对应 ch09）

### 目标
- 分析 DNS 解析过程
- 分析 HTTP 请求/响应结构
- 学会使用 I/O Graph

### 步骤

```bash
# 1. 抓包
sudo tcpdump -nni eth0 -w labs/lab03-dns-http.pcapng 'port 53 or port 80'

# 2. 产生流量
dig example.com
curl -v http://example.com

# 3. 分析 DNS
# 过滤器: dns
# 找 Query 和 Response
# 查看 Answer 中的 A 记录

# 4. 分析 HTTP
# 过滤器: http
# 查看 Request Method、Host、Response Code
# Statistics → HTTP → Requests
```

### 验证清单

- [ ] 能区分 DNS Query 和 Response
- [ ] 能找到 HTTP 请求行和响应状态码
- [ ] 能用 I/O Graph 查看流量趋势
- [ ] 理解 DNS 在 HTTP 之前发生

## 实验 4：TCP 重传与延迟分析（对应 ch11 + HFT 场景 01）

### 目标
- 识别 TCP 重传、快速重传、重复 ACK
- 分析 RTT 分布
- 理解延迟根因

### 步骤

```bash
# 1. 模拟丢包环境（用 tc/netem）
sudo tc qdisc add dev lo netem loss 5%
# 或在真实网络中抓包

# 2. 抓包并产生流量
sudo tcpdump -nni eth0 -w labs/lab04-retransmission.pcapng
iperf3 -c <server>

# 3. 恢复
sudo tc qdisc del dev lo netem

# 4. 分析
# Wireshark: 打开 pcap
# Expert Info: 看 Warnings/Errors
# 过滤器: tcp.analysis.retransmission
# 过滤器: tcp.analysis.fast_retransmission
# 过滤器: tcp.analysis.duplicate_ack
# Statistics → TCP Stream Graphs → Round Trip Time
```

### 分析任务

| 任务 | 操作 |
|------|------|
| 统计重传次数 | `tcp.analysis.retransmission` 看状态栏计数 |
| 分析重传根因 | 看 SEQ/ACK analysis → RTO 值 |
| RTT 分布图 | Statistics → TCP Stream Graphs → RTT |
| 导出 RTT CSV | RTT Graph → Save As CSV |

### 验证清单

- [ ] 能区分超时重传和快速重传
- [ ] 能找到触发快速重传的 3 个 Dup ACK
- [ ] 能看懂 RTT 图中的尖峰
- [ ] 能用 tshark 提取 RTT 数据

## 实验 5：tshark 命令行分析（对应 ch06）

### 目标
- 掌握 tshark 命令行工具
- 学会脚本化分析 pcap
- 统计协议分层

### 步骤

```bash
# 用实验 2 的 pcap
PCAP=labs/lab02-tcp-handshake.pcapng

# 1. 协议分层统计
tshark -r $PCAP -q -z io,phs

# 2. 提取所有 IP 地址
tshark -r $PCAP -T fields -e ip.src -e ip.dst | sort -u

# 3. HTTP 请求列表
tshark -r $PCAP -Y "http.request" -T fields -e http.request.method -e http.host -e http.request.uri

# 4. Follow TCP Stream（ASCII）
tshark -r $PCAP -q -z follow,tcp,ascii,0

# 5. 统计 TCP 标志分布
tshark -r $PCAP -T fields -e tcp.flags | sort | uniq -c | sort -rn
```

### 验证清单

- [ ] 能用 tshark 列出协议分层
- [ ] 能用 tshark 提取指定字段
- [ ] 能用 tshark Follow TCP Stream
- [ ] 能写一行 tshark 命令统计 TCP 标志

## 实验 6：NIC Offload 影响验证（对应 HFT 场景 02）

### 目标
- 观察 TSO/GRO 对抓包的影响
- 学会关闭 offload 后对比

### 步骤

```bash
# 1. 查看当前 offload 状态
ethtool -k eth0 | grep -E "tcp-segmentation|generic-receive|generic-segmentation"

# 2. 不关闭 offload 抓包
sudo tcpdump -nni eth0 -w labs/lab06-offload-on.pcapng -c 1000
# 产生流量: curl https://example.com

# 3. 关闭 offload
sudo ethtool -K eth0 tso off gro off gso off

# 4. 关闭后抓包
sudo tcpdump -nni eth0 -w labs/lab06-offload-off.pcapng -c 1000
# 产生相同流量: curl https://example.com

# 5. 恢复
sudo ethtool -K eth0 tso on gro on gso on

# 6. 对比分析
# 打开两个 pcap，比较：
# - 包数量（关闭后应该更多）
# - TCP 段大小（关闭后应 ≤ MSS 1448）
# - 时间戳精度
```

### 验证清单

- [ ] 能用 `ethtool -k` 查看 offload 状态
- [ ] 能用 `ethtool -K` 关闭/开启 offload
- [ ] 能在 Wireshark 中对比 offload 开/关的包差异
- [ ] 理解为什么 HFT 需要关闭 GRO

## 实验 7：HFT 延迟分析综合实验

### 目标
- 综合运用 Wireshark + tshark + eBPF
- 分析模拟交易流量的延迟
- 定位延迟根因

### 步骤

```bash
# 1. 准备环境
# 两个终端：一个跑模拟交易服务，一个跑客户端
# 服务端: python3 -m http.server 8000
# 客户端: while true; do curl http://localhost:8000; sleep 0.01; done

# 2. 抓包
sudo tcpdump -nni lo -w labs/lab07-hft-latency.pcapng -s 0

# 3. 同时用 eBPF 追踪内核事件（如果有 bpftrace）
sudo bpftrace -e '
  kprobe:tcp_retransmit_skb {
    printf("%lld retransmit\n", nsecs);
  }' > labs/lab07-retrans-events.txt &

# 4. 运行 30 秒后停止

# 5. 分析
# 5.1 基本统计
tshark -r labs/lab07-hft-latency.pcapng -q -z io,stat,0

# 5.2 RTT 分布
tshark -r labs/lab07-hft-latency.pcapng -Y "tcp.analysis.ack_rtt" \
  -T fields -e tcp.analysis.ack_rtt | sort -n | awk '
  {vals[NR]=$1; sum+=$1}
  END{printf "count=%d avg=%.0fus p50=%.0fus p95=%.0fus p99=%.0fus max=%.0fus\n",
    NR, sum/NR*1e6, vals[int(NR*0.5)]*1e6, vals[int(NR*0.95)]*1e6, vals[int(NR*0.99)]*1e6, vals[NR]*1e6}'

# 5.3 重传统计
tshark -r labs/lab07-hft-latency.pcapng -Y "tcp.analysis.retransmission" -c 10

# 5.4 零窗口检查
tshark -r labs/lab07-hft-latency.pcapng -Y "tcp.analysis.zero_window" -c 10

# 5.5 包间隔分布
tshark -r labs/lab07-hft-latency.pcapng -Y "tcp.len > 0" \
  -T fields -e frame.time_delta_displayed | \
  awk '{if($1>0.001) print $1*1000 "ms"}' | sort -n | uniq -c | sort -rn | head -10
```

### 验证清单

- [ ] 能计算 RTT 的 P50/P95/P99
- [ ] 能识别重传和零窗口事件
- [ ] 能将 pcap 时间戳与 eBPF 事件对齐
- [ ] 能给出延迟根因分析报告

## 实验 8：容器网络抓包（对应 HFT 场景 04）

### 目标
- 在 Docker 容器中抓包
- 理解 veth 和 docker0 的关系
- 分析容器流量

### 步骤

```bash
# 1. 启动一个容器
docker run -d --name webtest nginx

# 2. 在容器内抓包
docker exec webtest tcpdump -nni eth0 -w /tmp/container.pcap &
# 产生流量: curl http://<container_ip>/

# 3. 在宿主机找 veth 抓包
docker inspect -f '{{.State.Pid}}' webtest
# 假设 PID=12345
nsenter -t 12345 -n ip link
# 找到 eth0@ifN

# 在宿主机抓 docker0
sudo tcpdump -nni docker0 -w labs/lab08-docker-bridge.pcap

# 4. 对比容器内和宿主机看到的包
# - 源/目的 IP 是否不同？（NAT）
# - 包数量是否一致？
```

### 验证清单

- [ ] 能用 `nsenter` 进入容器网络命名空间
- [ ] 能在 docker0 上抓到容器流量
- [ ] 理解容器 eth0 和宿主机 veth 的对应关系
- [ ] 能识别 Docker NAT 前后的 IP 变化

## 实验文件管理

```bash
# .pcap 文件被 .gitignore 忽略
# 建议在 labs/ 目录下按实验编号存放
labs/
├── lab01-first-capture.pcapng
├── lab02-tcp-handshake.pcapng
├── lab03-dns-http.pcapng
├── lab04-retransmission.pcapng
├── lab06-offload-on.pcapng
├── lab06-offload-off.pcapng
├── lab07-hft-latency.pcapng
├── lab07-retrans-events.txt
└── lab08-docker-bridge.pcap
```

## 进阶资源

| 资源 | 说明 |
|------|------|
| [Wireshark Sample Captures](https://wiki.wireshark.org/SampleCaptures) | 官方示例 pcap |
| [CloudShark](https://www.cloudshark.org/) | 在线 pcap 分析 |
| [HFT 场景目录](../hft-scenarios/00-overview.md) | HFT 专属分析 |
| [速查笔记](../cheatsheet/notes.md) | 过滤器速查 |
