# 第12章 安全领域的数据包分析

> 全书：[../README.md](../README.md) · 上一章：[第11章 让网络不再卡](../chapter-11-network-slow-fix/chapter-summary.md) · 对照：[分层攻击速查](../../top_down/08_network_security/layer-attacks-cheatsheet.md)

## 整体框架

```text
12.1 侦察：SYN 扫描 · OS 指纹
        ↓
12.2 操纵：ARP 污染 · HTTP Cookie 劫持
        ↓
12.3 利用：Aurora · RAT/IDS · Carving
        ↓
12.4 EK/勒索：重定向链 · C2 POST
```

| 小节 | 文件 |
|------|------|
| 12.1 | [01-network-reconnaissance.md](./01-network-reconnaissance.md) |
| 12.2 | [02-traffic-manipulation.md](./02-traffic-manipulation.md) |
| 12.3 | [03-vulnerability-exploitation.md](./03-vulnerability-exploitation.md) |
| 12.4 | [04-exploit-kit-ransomware.md](./04-exploit-kit-ransomware.md) |

## 12.5 小结

安全向分析必须建立在**正常基线**之上。扫描、MITM、漏洞利用、C2 最终都会在**头部参数、时序、十六进制载荷**上留痕。

| 能力 | 用途 |
|------|------|
| 溯源攻击路径 | 重定向链、扫描源 IP |
| 提取载荷 | Follow Stream、JFIF 对齐、Export Objects |
| 编写 IDS 规则 | 特征串、行为（SYN 扫描、异常 ARP） |

## 重点难点

| 主题 | 抓包要点 |
|------|----------|
| SYN 扫描 | 开放 5 包 / 关闭 2 包；无完成握手 |
| 被动指纹 | TTL 128 vs 64、MSS、Win |
| ARP MITM | Gratuitous、MAC 突变 |
| 会话劫持 | 明文 Cookie |
| Aurora | 302、iframe、反向 shell 明文 |
| RAT | IDS hex → `tcp contains` → 修 JFIF |
| 勒索 C2 | 随机 POST URL 模式 |

## 实操要点

1. 维护 [§11.5](../chapter-11-network-slow-fix/05-network-baseline.md) 安全基线 pcap。
2. 恶意分析：**隔离环境**；关闭 [§5.3](../chapter-05-advanced-feature/03-name-resolution.md) 外连 DNS。
3. 合法授权后再对生产网镜像抓包。

## 小节索引

- [12.1 网络侦察](./01-network-reconnaissance.md)
- [12.2 流量操纵](./02-traffic-manipulation.md)
- [12.3 漏洞利用](./03-vulnerability-exploitation.md)
- [12.4 漏洞利用工具包和勒索软件](./04-exploit-kit-ransomware.md)
