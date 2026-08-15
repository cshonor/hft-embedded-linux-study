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

详见 [第6章](../chapter-06-tshark-tcpdump/chapter-summary.md)。

## 排障口诀

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

## 个人常用组合

（待填）
