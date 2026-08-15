# 第9章 常见高层网络协议

> 全书：[../README.md](../README.md) · 上一章：[第8章 传输层](../chapter-08-transport-layer-tcp-udp/chapter-summary.md)

## 整体框架

```text
9.1 DHCP（DORA · 选项53 · DHCPv6 SARR）
        ↓
9.2 DNS（A/AAAA/MX · 递归 · AXFR/TCP）
        ↓
9.3 HTTP（GET/POST · gzip · 302）
        ↓
9.4 SMTP（EHLO/DATA · STARTTLS · base64≠加密）
        ↓
9.5 小结：应用层基线 = 排障与安全基石
```

| 小节 | 文件 |
|------|------|
| 9.1 DHCP | [01-dhcp-protocol.md](./01-dhcp-protocol.md) |
| 9.2 DNS | [02-dns-protocol.md](./02-dns-protocol.md) |
| 9.3 HTTP | [03-http-protocol.md](./03-http-protocol.md) |
| 9.4 SMTP | [04-smtp-protocol.md](./04-smtp-protocol.md) |

## 9.5 本章小结

熟练掌握 **DHCP、DNS、HTTP、SMTP** 的正常交互基线，是排查复杂故障与防御高级威胁的核心：

- 获址失败 → 先抓 **DORA** 是否完整、NAK、中继  
- 能 ping 不能上网 → **DNS** 查询/响应、递归链  
- Web 问题 → **TCP 握手 + HTTP 状态码 + Location**  
- 邮件 → **SMTP 命令序 + TLS 升级**；附件 **base64 可逆**  

建议结合 **RFC** 与 Wireshark Display Filter Reference 核对字段名。

## 重点难点

| 协议 | 抓包要点 |
|------|----------|
| DHCP | UDP 67/68；选项 53；续租仅 Request/ACK |
| DNS | ID 配对；RD/RA；AXFR 走 TCP |
| HTTP | 先 TCP；gzip 需 Follow Stream |
| SMTP | 文本命令；`.` 结束 DATA；Base64≠加密 |

## 实操要点

1. `ipconfig /renew` + `dhcp` 过滤器保存 baseline。  
2. `nslookup` + `dns.qry.name` 过滤。  
3. 明文 HTTP 站练 GET/200 与 Follow Stream。  
4. 过滤器见 [cheatsheet](../cheatsheet/notes.md)。

## 小节索引

- [9.1 DHCP](./01-dhcp-protocol.md)
- [9.2 DNS](./02-dns-protocol.md)
- [9.3 HTTP](./03-http-protocol.md)
- [9.4 SMTP](./04-smtp-protocol.md)
