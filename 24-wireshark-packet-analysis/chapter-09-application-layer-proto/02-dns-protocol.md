# 9.2 域名系统（DNS）

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md)

**核心主旨**：域名 → IP 的层级查询；读 QR/标志位、记录类型与递归/区域传送。

## 核心知识点

### 9.2.1 DNS 数据包结构

| 字段/区 | 说明 |
|---------|------|
| **Transaction ID** | 匹配查询与响应 |
| **QR** | 0=查询，1=响应 |
| **OpCode** | 标准查询等 |
| **AA** | Authoritative Answer，权威应答 |
| **RD** | Recursion Desired，期望递归 |
| **RA** | Recursion Available，支持递归 |
| **Question** | 查询名、类型、类（如 IN） |
| **Answer** | 应答 RR |
| **Authority** | 权威 NS 等 |
| **Additional** | 附加 RR（如 glue A 记录） |

**默认**：UDP **53**；大响应可能 **TC** 截断 → 客户端改 **TCP 53** 重查。

**Wireshark**：`dns` · `dns.flags.response == 0`（查询）

---

### 9.2.2 简单查询

| 步 | 说明 |
|----|------|
| 客户端 | A 记录查询 `www.example.com` |
| 服务器 | QR=1，Answer 含 IPv4 |
| 过滤器 | `dns.qry.name` 包含域名 |

---

### 9.2.3 常见记录类型

| Type | 名称 | 用途 |
|------|------|------|
| **1** | A | IPv4 |
| **28** | AAAA | IPv6 |
| **5** | CNAME | 别名 |
| **15** | MX | 邮件交换 |
| **252** | AXFR | 完整区域传送（查询类型） |

显示：`dns.a` · `dns.aaaa` · `dns.cname`

---

### 9.2.4 DNS 递归

| 角色 | 行为 |
|------|------|
| 客户端 → 本地解析器 | 常设 **RD=1** |
| 本地服务器 | 若无缓存，以**新 ID** 向上游（如 8.8.8.8）迭代查询 |
| 抓包 | 同一工作站可见**多条**查询：内网 DNS IP 与公网 DNS IP |

**过滤器**：`dns.flags.recdesired == 1`

---

### 9.2.5 区域传送（AXFR / IXFR）

| 项 | 说明 |
|----|------|
| 目的 | 主从 DNS **同步区域** 全部记录 |
| 协议 | 数据量大 → 通常 **TCP 53** |
| 风险 | 泄露整域主机图；须 **ACL** 限制从服务器 IP |

**Wireshark**：`dns.flags.opcode == 0` 且 TCP 流上大量 Answer；或 `dns.axfr`（视版本）

> **拓展**：DNS 欺骗/放大 → [第12章 安全分析](../chapter-12-security-analysis/chapter-summary.md)；异常大 UDP 响应对 → 放大攻击特征。

## 抓包/实操记录

| 实验 | 命令/过滤 |
|------|-----------|
| 清缓存查询 | `nslookup example.com` → `dns` |
| 只看查询 | `dns.flags.response == 0` |
| 递归链 | 抓内网 DNS 出口，看多级 QR=0/1 |
| Follow | Follow UDP Stream 看单查询往返 |

```bash
tshark -r cap.pcapng -Y "dns" -T fields -e dns.qry.name -e dns.qry.type -e dns.a
```

## 疑问与总结

- **NXDOMAIN**：响应码 3，无 Answer A 记录。
- mDNS（5353）、DoH（HTTPS）不在传统 UDP 53 模型内，需另辨。
