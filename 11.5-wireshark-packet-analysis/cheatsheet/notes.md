# Wireshark 速查笔记

> 全书：[../README.md](../README.md) · 官方：[Display Filter Reference](https://www.wireshark.org/docs/dfref/) · 详解：[第1章](../chapter-01-network-basics/chapter-summary.md)  
> **安装与首次抓包**：[install-and-verify.md](./install-and-verify.md)（Win / macOS / Linux 逐步 + 验证命令）

---

## 核心一页纸（精简版）

### 一、核心定义

| 术语 | 说明 |
|------|------|
| **数据包分析**（协议分析 / 数据包嗅探） | 捕获并解析网络传输数据，用于排障、性能、安全 |
| **数据包嗅探器** | 抓包 + 解析工具；抓取介质上的**原始二进制** |

### 二、嗅探器标准流程

```text
原始比特流 → 收集 → 转换 → 分析 → 可读结果
```

| 步 | 要点 |
|----|------|
| **收集** | 从介质抓二进制；**混杂模式** → 收全网段帧（不限本机 MAC） |
| **转换** | 二进制 → 可读格式 |
| **分析** | 逐层解析协议，提取 IP/端口/标志位等 → 排障结论 |

### 三、主流工具对比

| 工具 | 优势 | 场景 |
|------|------|------|
| **Wireshark** | GUI、协议树、过滤器、统计 | 日常排障、学协议、深度分析 |
| **tcpdump / tshark** | 轻量、无 GUI、可脚本化 | 服务器生产、后台抓包、自动化 |

**通用技巧**：大流量先 `tcpdump` 落盘 `.pcap` / `.pcapng`，再 Wireshark **离线分析**。

### 四、基础实操

| 项 | 操作 |
|----|------|
| 混杂模式 | Capture Options → 勾选 **Promiscuous**（无线/部分网卡受限） |
| 过滤器 | `http` · `ip.addr == x.x.x.x` |
| 验证 | 抓网页 → `Frame` → 协议树 |

### 五、概念区分 & 合规

| 区分 | 说明 |
|------|------|
| **嗅探** | 侧重**抓到**数据 |
| **分析** | 侧重**读懂**协议与行为 |
| tcpdump | 偏收集 + 简单转换 |
| Wireshark | 转换 + **深度分析** |

**合法边界**：仅在**自有/授权**网络或实验环境抓包；公网/商用环境须**书面授权**。

---

## 显示过滤器（Display Filters）

| 场景 | 过滤器 |
|------|--------|
| 某 IP | `ip.addr == 192.168.1.1` |
| 某端口 | `tcp.port == 8080` |
| HTTP | `http` |
| HTTP 302 | `http.response.code == 302` |
| DNS | `dns` |
| DNS 查询 | `dns.flags.response == 0` |
| DHCP | `bootp` 或 `dhcp` |
| DHCP Discover | `dhcp.option.dhcp == 1` |
| SMTP | `smtp` |
| TCP 握手 | `tcp.flags.syn==1 && tcp.flags.ack==0` |
| TCP 挥手 | `tcp.flags.fin==1` |
| TCP RST | `tcp.flags.reset==1` |
| 某 TCP 流 | `tcp.stream eq 0` |
| UDP / DNS | `udp.port == 53` 或 `dns` |
| 重传 | `tcp.analysis.retransmission` |
| 快速重传 | `tcp.analysis.fast_retransmission` |
| 重复 ACK | `tcp.analysis.duplicate_ack` |
| 零窗口 | `tcp.analysis.zero_window` |
| 窗口更新 | `tcp.analysis.window_update` |
| ARP | `arp` |
| 隐藏 ARP | `!arp` |
| ICMP | `icmp` |
| Ping 请求 | `icmp.type == 8` |
| TTL 超时 | `icmp.type == 11` |
| IPv6 | `ipv6` |
| NDP | `icmpv6.type == 135 or icmpv6.type == 136` |
| HTTP Cookie | `http.cookie` |
| HTTP POST | `http.request.method == "POST"` |
| 802.11 管理帧 | `wlan.fc.type == 0` |
| Beacon | `wlan.fc.type_subtype == 0x08` |
| 指定 AP | `wlan.bssid == aa:bb:cc:dd:ee:ff` |
| 信道 11 | `wlan_radio.channel == 11` |
| EAPOL / WPA | `eapol` |

### TCP 异常事件（Expert 同款，第11章）

| 场景 | 过滤器 |
|------|--------|
| **全部 TCP 异常** | `tcp.analysis.flags` |
| 丢段（抓包起点/真丢包） | `tcp.analysis.lost_segment` |
| 乱序 | `tcp.analysis.out_of_order` |
| 前一段未捕获 | `tcp.analysis.previous_segment_not_captured` |
| ACK 未捕获段 | `tcp.analysis.ack_lost_segment` |
| keep-alive | `tcp.analysis.keep_alive` |
| Window Full（发满对端窗口） | `tcp.analysis.window_full` |

### 现代协议补充（书外实战）

| 场景 | 过滤器 |
|------|--------|
| TLS | `tls` |
| ClientHello | `tls.handshake.type == 1` |
| TLS 版本 | `tls.record.version == 0x0303` |
| QUIC / HTTP3 | `quic` |
| HTTP/2 | `http2` |
| WebSocket | `websocket` |
| HTTP 错误码 | `http.response.code >= 400` |
| 纯控制包（无载荷） | `tcp.len == 0` |
| 大包（满载传输） | `tcp.len >= 1400` |
| 时间间隔 > 1s 的帧 | `frame.time_delta > 1` |

## 捕获过滤器（Capture / BPF）

| 场景 | 过滤器 |
|------|--------|
| 某主机 | `host 192.168.1.1` |
| 某端口 | `port 443` |
| 组合 | `host 10.0.0.1 and port 8080` |
| 仅 TCP RST（BPF） | `tcp[13] & 4 != 0` 或 `tcp&4==4` |

## 命令行（TShark / tcpdump）

| 场景 | 命令 |
|------|------|
| 列网卡 | `tshark -D` |
| 抓包写盘 | `tshark -ni 1 -w out.pcapng -f "host x"` |
| 离线显示滤 | `tshark -r out.pcapng -Y "http" -n` |
| 绝对时间 | `tshark -r out.pcapng -t ad` |
| 协议分层 | `tshark -r out.pcapng -q -z io,phs` |
| Follow TCP 0 | `tshark -r out.pcapng -q -z follow,tcp,ascii,0` |
| tcpdump 抓 | `sudo tcpdump -nni eth0 -w out.pcap 'tcp port 443'` |

### tshark 字段提取模板（-T fields）

```bash
# 通用骨架：-e 指定字段，多字段用 tab 分隔
tshark -r out.pcapng -Y "过滤表达式" \
  -T fields -e frame.number -e ip.src -e ip.dst \
  -e tcp.srcport -e tcp.dstport -e frame.time_relative

# 常用字段速查
# frame.time_relative   相对时间（延迟分析必备）
# tcp.analysis.ack_rtt  每包 RTT
# tcp.len               TCP 载荷长度
# http.request.full_uri 完整 URL
# dns.qry.name          查询的域名
# tls.handshake.extensions_server_name  TLS SNI
```

### 伴随工具（Wireshark 自带命令行全家桶）

| 工具 | 用途 | 示例 |
|------|------|------|
| `capinfos` | pcap 元信息（时长/包数/速率） | `capinfos out.pcapng` |
| `editcap` | 切分/截取/去重 | `editcap -c 100000 big.pcap part.pcapng` |
| `mergecap` | 合并多个 pcap | `mergecap -w merged.pcapng a.pcap b.pcap` |
| `text2pcap` | 十六进制文本转 pcap | 调试协议解析器用 |

详见 [第6章](../chapter-06-tshark-tcpdump/chapter-summary.md)。

## 排障口诀

> **排除网络问题，首先看 TCP。** TCP 是分水岭——握手成功说明"连得通"，RTT 和重传率说明"通得好不好"，一眼区分问题在物理链路层还是应用层。
>
> 这条原则的操作化展开见下方 **[TCP 排障速查](#tcp-排障速查首先看-tcp-的操作化)**。

```text
抓包三步：收数据、转格式、析协议
工具分工：线上 tcpdump 抓包，线下 Wireshark 分析
红线牢记：抓包必合规，勿越权操作
```

| 场景 | 口诀 |
|------|------|
| 慢/卡 | 先 Expert（重传/零窗口）→ 再延迟三段时间（第11章） |
| 上不了网 | DNS 有无响应 → 有无查 DNS → SYN 后 RST 还是重传 |
| 无线 | Monitor 模式 + 信道对齐 + RSSI/速率列 |

## TCP 排障速查（"首先看 TCP" 的操作化）

### 1. 标志位组合速查

| 组合 | 含义 | 过滤器 |
|------|------|--------|
| SYN, ACK=0 | 连接发起 | `tcp.flags.syn==1 && tcp.flags.ack==0` |
| SYN+ACK | 服务端应答握手 | `tcp.flags.syn==1 && tcp.flags.ack==1` |
| ACK | 纯确认/数据携带 | `tcp.flags.ack==1 && tcp.len==0` |
| FIN | 半关闭 | `tcp.flags.fin==1` |
| FIN+ACK | 挥手 | `tcp.flags.fin==1 && tcp.flags.ack==1` |
| RST | 连接拒绝/异常中断 | `tcp.flags.reset==1` |
| RST+ACK | 端口未监听（拒绝） | `tcp.flags.reset==1 && tcp.flags.ack==1` |

### 2. 排障决策树

```text
抓到流量？
├─ 否 → 物理层/交换机/VLAN/防火墙（Wireshark 无能为力，查链路）
└─ 是 → 先看 TCP
    ├─ 无 SYN → 没发起连接 → 查客户端应用/DNS（先查 DNS 有无应答）
    ├─ SYN 后无 SYN+ACK → 对端/中间设备吞包 → 防火墙或端口未开
    ├─ SYN 后立即 RST → 端口未监听 → 服务没起/挂了
    ├─ 握手成功但慢 → 看 RTT 与重传：
    │   ├─ tcp.analysis.retransmission 多 → 丢包，定位路径
    │   ├─ tcp.analysis.zero_window 多 → 接收方处理不过来（应用慢）
    │   ├─ RTT 高但无重传 → 链路本身远/绕路，非丢包
    │   └─ 40ms 聚集 → Nagle + Delayed ACK 互锁
    └─ TCP 正常但应用异常 → 上应用层（HTTP 状态码/DNS/TLS 失败）
```

### 3. TCP 健康四指标

| 指标 | 看哪里 | 健康标准（经验值） |
|------|--------|--------------------|
| 握手完整性 | SYN → SYN+ACK → ACK 全在 | 无缺步 |
| RTT | `tcp.analysis.ack_rtt` 或 TCP Stream Graph → Round Trip Time | 稳定、无锯齿尖峰 |
| 重传率 | 重传包数 / 总数据包 | < 1% |
| 窗口 | `tcp.window_size_value` + zero_window 事件 | 不触零、持续更新 |

## 统计功能速查（Statistics 菜单，第5章）

| 功能 | 路径 | 用途 |
|------|------|------|
| Endpoints | Statistics → Endpoints | Top Talkers，按 Bytes 排序找大户 |
| Conversations | Statistics → Conversations | 按会话维度聚合流量 |
| Protocol Hierarchy | Statistics → Protocol Hierarchy | 协议占比 vs 基线，突变=异常 |
| Packet Lengths | Statistics → Packet Lengths | 大包传数据小包控传输的分布 |
| IO Graph | Statistics → IO Graph | 吞吐随时间曲线 |
| TCP Stream Graphs | Statistics → TCP Stream Graphs → * | RTT / 窗口 / 时序图，TCP 排障核心 |
| Flow Graph | Statistics → Flow Graph | 全局时序图，讲给别人听用 |
| Expert Info | Analyze → Expert Information | 全部异常事件分级汇总，排障第一步 |

## 快捷键

| 键 | 功能 |
|----|------|
| `Ctrl + /` | 追加当前过滤器表达式到过滤器栏 |
| `Ctrl + F` | 在包列表中查找（配合过滤表达式） |
| `右键 → Follow → TCP Stream` | 查看整条流（快捷键默认 Alt+Shift+T 视版本） |
| `M` | 标记/取消标记当前包 |
| `Shift + ←/→` | 跳到上/下一个会话 |
| `Ctrl + .` / `Ctrl + ,` | 跳到下/上一个有颜色标记的包 |
| 双击包详情字段 | 选中后可直接"作为过滤器应用" |
| 右键字段 → Apply as Filter | 最快的过滤器生成方式 |

## HFT 常用组合

### 延迟快速诊断

```bash
# 一行命令：RTT P50/P95/P99 统计
tshark -r trade.pcapng -Y "tcp.analysis.ack_rtt" -T fields -e tcp.analysis.ack_rtt | \
  awk '{v[NR]=$1;s+=$1}END{printf "n=%d avg=%.0fus p50=%.0fus p95=%.0fus p99=%.0fus max=%.0fus\n",NR,s/NR*1e6,v[int(NR*.5)]*1e6,v[int(NR*.95)]*1e6,v[int(NR*.99)]*1e6,v[NR]*1e6}'

# 重传统计
tshark -r trade.pcapng -Y "tcp.analysis.retransmission" -c 20

# 零窗口检查
tshark -r trade.pcapng -Y "tcp.analysis.zero_window" -c 20

# Nagle/Delayed ACK 检测（40ms 附近聚集 = 有问题）
tshark -r trade.pcapng -Y "tcp.len>0" -T fields -e frame.time_delta_displayed | \
  awk '{if($1>0.001)print $1*1000"ms"}' | sort -n | uniq -c | sort -rn | head
```

### 抓包前必做

```bash
# 关闭 GRO（影响 RX 分析）
sudo ethtool -K eth0 gro off

# 验证
ethtool -k eth0 | grep generic-receive
```

### 生产环境高流量抓包

```bash
# 10Gbps+：先 BPF 过滤再落盘
sudo tcpdump -nni eth0 -w trade.pcapng -s 0 \
  'host 10.0.0.5 and tcp port 443' -C 100 -W 10
# -C 100: 每 100MB 轮转，-W 10: 保留 10 个文件
```
