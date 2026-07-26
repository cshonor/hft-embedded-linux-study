# 28.8 小结

> [study.md](../study.md) · [Ch 29](../Chapter29_DataLinkAccess/study.md)

---

## 章节核心提炼

### 1. 打破传输层壁垒

Raw socket → 用户态构造/处理 **IP 层** — `ping`、`traceroute`、路由守护进程等基础设施。

### 2. 发送/接收细节

| 方向 | 要点 |
|------|------|
| **发** | `IP_HDRINCL`、字节序移植、校验和/源 IP 内核规则 |
| **收** | 内核下放规则；**IPv4 含 IP 头** vs **IPv6 不含** |

### 3. 非万能

**不能**截获本机 TCP/UDP → **Ch 29** 链路层嗅探。

---

## 工具与 API 对照

| 工具 | 主要 API |
|------|----------|
| ping | ICMP raw |
| traceroute | UDP + TTL + ICMP raw |
| tcpdump | BPF / datalink（Ch 29） |
| icmpd | ICMP raw + Unix 域 IPC |

---

## 个人学习总结

（待填）
