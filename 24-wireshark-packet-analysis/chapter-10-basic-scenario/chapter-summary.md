# 第10章 基础的现实世界场景

> 全书：[../README.md](../README.md) · 上一章：[第9章 应用层协议](../chapter-09-application-layer-proto/chapter-summary.md)

## 整体框架

| 案例 | 关键线索 | 根因层级 |
|------|----------|----------|
| [10.1 丢失网页](./01-lost-web-content.md) | 7 DNS / 8 会话；SYN 黑洞 | 本地 DNS 缓存旧 IP |
| [10.2 气象站](./02-weather-station-no-response.md) | HTTP 200 + INVALIDPASSWORD | 应用配置 |
| [10.3 无法上网](./03-no-internet-access.md) | 无 DNS 响应 / 无 DNS+RST / SYN 重传 | 网关 / hosts / 上游 |
| [10.4 打印机](./04-printer-fault.md) | 零窗口 + 重传 | 打印机内存 |
| [10.5 分公司](./05-branch-office-dns.md) | TCP 53 SYN 无应答 | 防火墙仅 UDP 53 |
| [10.6 FTP MD5](./06-ftp-md5-proof.md) | STOR + Follow Stream | 应用非网络 |

## 10.7 结语

表层问题再像「应用报错」，底层仍是**标准包交互**。结合协议逻辑 + 抓包，可剥离干扰，定位系统、硬件或应用的真实症结。

**通用排障顺序**

```text
物理/镜像点 → L2 ARP → L3 路由/DNS → L4 TCP 标志 → L7 正文
```

## 重点难点

| 技巧 | 场景 |
|------|------|
| Conversations 计数 | DNS 次数 vs TCP 会话数 |
| 200 + 错误体 | IoT/HTTP API |
| 无 DNS 连 IP | hosts / 缓存 |
| SYN 无应答 | 上游 vs 旧 IP |
| Zero Window | 接收端（打印机） |
| TCP 53 | DNS 区域传送 / 大响应 |
| Follow Stream + hash | FTP/HTTP 举证 |

## 实操要点

1. 每类案例保存一份 **baseline + 故障** pcap 对照。
2. 熟练 `Statistics` → Conversations / HTTP / Expert Info。
3. 跨章工具：第 5 章统计、第 8 章 TCP、第 9 章 DNS/HTTP。

## 小节索引

- [10.1 丢失的网页内容](./01-lost-web-content.md)
- [10.2 无响应的气象服务](./02-weather-station-no-response.md)
- [10.3 无法访问 Internet](./03-no-internet-access.md)
- [10.4 打印机故障](./04-printer-fault.md)
- [10.5 分公司之困](./05-branch-office-dns.md)
- [10.6 生气的开发者](./06-ftp-md5-proof.md)
