# 12.2 流量操纵

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md) · ARP：[§7.1](../chapter-07-network-layer-proto/01-arp-protocol.md) · [§2.3.4](../chapter-02-traffic-monitor/03-sniff-on-switched-network.md)

**核心主旨**：MITM 与会话劫持在包里的痕迹——异常 ARP、MAC 突变、明文 Cookie。

## 核心知识点

### 12.2.1 ARP 缓存污染（MITM）

| 逻辑 | 伪造 ARP，使 A、B 流量经攻击者转发 |
|------|-------------------------------------|

**抓包异常线索**

| 线索 | 说明 |
|------|------|
| **非广播 ARP 请求** | 单播 ARP Req，且内容与「问网关 MAC」不符常规 |
| **未请求的 ARP Reply** | **Gratuitous / 单方面** Reply，把**网关 IP → 攻击者 MAC** |
| **MAC 突变** | 同一**目的 IP（外网）** 的帧，以太网 **DST MAC** 从真网关变为攻击者 MAC |

**过滤器**：`arp` · `arp.opcode==2` · 对比前后 `eth.dst`

| 防御 | 静态 ARP、DAI、802.1X、加密（TLS） |

> **拓展**：**SSL Stripping** + ARP 污染：降级 HTTPS → 明文 HTTP 再截 Cookie。

---

### 12.2.2 会话劫持

| 步骤 | 抓包可见 |
|------|----------|
| 1 | MITM 或镜像看到 **明文 HTTP** |
| 2 | `Set-Cookie` / 请求头 **`Cookie: PHPSESSID=...`** |
| 3 | 攻击者浏览器植入同 Cookie → 冒充用户 |

| 防御 | **HTTPS**、HttpOnly/Secure Cookie、短会话、绑定 IP/UA |

**Wireshark**：`http.cookie` · Follow TCP Stream 搜 `Cookie:`

## 抓包/实操记录

| 练习 | 操作 |
|------|------|
| 对比 Gratuitous | [§7.1.4](../chapter-07-network-layer-proto/01-arp-protocol.md) 合法 vs 攻击者高频伪造 |
| HTTP 明文 | 实验网登录 HTTP 站，MITM lab 中导出 Cookie 字段 |

**合规**：仅在授权 lab；生产嗅探需书面授权。

## 疑问与总结

- 全链路 **TLS 1.3** 时 Cookie 不可见，除非终端被控或密钥泄露。
- 与 [§10.2](../chapter-10-basic-scenario/02-weather-station-no-response.md) 区别：10.2 是配置错密码，12.2 是主动窃听/篡改。
