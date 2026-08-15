# 7.3 互联网控制消息协议（ICMP / ICMPv6）

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md) · IPv4：[§7.2.1](./02-ipv4-protocol.md)

**核心主旨**：ICMP 作为 IP 的「控制平面」——报错、Ping、Traceroute；ICMPv6 还承载 NDP。

## 核心知识点

### 7.3.1–7.3.2 定位与头部

| 项 | 说明 |
|----|------|
| 封装 | **无独立端口**；载荷装在 **IP protocol=1**（ICMP）或 IPv6 **Next Header=ICMPv6** |
| **Type** | 宏观类别（如 3=目的不可达，8=Echo Request） |
| **Code** | 子类型（如 3/3=端口不可达） |
| **Checksum** | 覆盖 ICMP 头+数据 |

**Wireshark**：展开 **Internet Control Message Protocol**；`icmp.type`、`icmp.code`。

---

### 7.3.3 Echo 请求与响应（Ping）

| 包 | Type | Code |
|----|------|------|
| Echo **Request** | **8** | **0** |
| Echo **Reply** | **0** | **0** |

| 字段 | 作用 |
|------|------|
| Identifier / Sequence | 配对请求与响应 |

**过滤器**：`icmp.type==8` · `icmp.type==0` · 或 `icmp`

| 易错 | 说明 |
|------|------|
| Ping 不通 ≠ 宕机 | 防火墙常**丢弃 ICMP** |
| 安全 | 随机载荷可用于指纹/隧道（ICMP tunneling） |

IPv6：`icmpv6.type == 128`（Echo Request）/ `129`（Echo Reply）

---

### 7.3.4 路由跟踪（Traceroute）

**原理**：利用 **TTL 逐跳耗尽**。

```text
TTL=1 → 第1跳路由器丢弃 → ICMP Type 11 (Time Exceeded) 回源
TTL=2 → 第2跳 …
直至到达目的或收到目的端口不可达等
```

| ICMP（常见） | 含义 |
|--------------|------|
| **Type 11, Code 0** | TTL 超时（传输期间超时） |

响应常为 **「双头」**：外层 ICMP + 内嵌**触发丢弃的原始 IP/ICMP 副本**，供源端核对。

**变体**：Windows `tracert` 可能用 ICMP；Linux `traceroute` 常用 UDP 高端口或 TCP——抓包时除 ICMP 11 外还可能见 **3/3 端口不可达**。

**Wireshark**：对 trace 文件按 `icmp.type==11` 过滤，看 `ip.src` 为各跳路由器。

---

### 7.3.5 ICMPv6（RFC 4443）

| 角色 | 说明 |
|------|------|
| 报错 / Ping6 | 类似 ICMPv4 |
| **NDP** | Type **135/136** 等（见 [§7.2.2](./03-ipv6-protocol.md)） |
| **PMTUD** | **Packet Too Big**（Type 2） |
| 地位 | IPv6 **基础设施级**，比 v4 时代 ICMP 更核心 |

> **拓展**：ICMP 隧道渗漏 → 异常大 ICMP 载荷、固定间隔 Echo、非标准 Type；深度检测看载荷熵与频率。

## 抓包/实操记录

| 实验 | 命令 / 过滤 |
|------|-------------|
| Ping | `ping 8.8.8.8` → `icmp` 成对 8/0 |
| Traceroute | `tracert` / `traceroute` + `icmp.type==11` |
| 端口不可达 | 访问关闭的 UDP 端口 → `icmp.type==3 && icmp.code==3` |
| 禁 ping 主机 | 仅有 Request 无 Reply → 怀疑防火墙 |

```bash
tshark -r cap.pcapng -Y "icmp.type==8 || icmp.type==0" -T fields -e frame.time -e ip.src -e ip.dst -e icmp.type -e icmp.seq
```

## 疑问与总结

- **Type/Code 表** 背常用即可；其余查 Wireshark 解码或 RFC。
- Traceroute 路径**不对称**时，往返可能走不同路由（见第 2 章多网段抓包）。
