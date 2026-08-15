# 11.5 网络基线

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md) · 第3章基线：[§3.4](../chapter-03-wireshark-intro/04-get-started.md) · 统计：[第5章](../chapter-05-advanced-feature/chapter-summary.md)

**核心主旨**：故障前在**正常时**存 pcap + 统计快照，对比 Protocol Hierarchy、吞吐与 TCP 行为。

## 核心知识点

### 11.5.1 站点基线（Site Baseline）

| 项 | 说明 |
|----|------|
| **抓包位置** | 边缘：核心路由、防火墙、出口镜像 |
| **记录** | `Protocol Hierarchy` 各协议 **%**；总 **pps/bps**（IO Graph） |
| **用途** | 发现 ARP 风暴、未知协议、整体流量结构漂移 |

---

### 11.5.2 主机基线（Host Baseline）

| 项 | 说明 |
|----|------|
| **位置** | 关键服务器/设备本机或镜像口 |
| **内容** | 开机/关机广播；**依赖 IP 列表**（必须连上的 DNS、DB、域控） |
| **用途** | 故障时看「该连的没连」还是「协议异常」 |

---

### 11.5.3 应用程序基线（Application Baseline）

| 项 | 说明 |
|----|------|
| **位置** | 跑业务的服务器侧 |
| **内容** | 正常端口与协议；**IO Graph** 峰值/均值 bps |
| **用途** | 区分「链路限速」vs「应用真打满」 |

---

### 11.5.4 注意事项

| 策略 | 说明 |
|------|------|
| **三时段采样** | 早晨低负载、下午高峰、深夜空闲 |
| **勿高峰本机抓** | 关键服务器上 live 抓包耗 CPU/磁盘 → 用 **TAP/SPAN** 或短时 `-c` |
| 命名 | `site-baseline-YYYYMMDD-peak.pcapng` |

与 [§10.1](../chapter-10-basic-scenario/01-lost-web-content.md) 对比：无 baseline 难发现「第 8 会话」异常。

## 抓包/实操记录

| 交付物 | 内容 |
|--------|------|
| pcapng | 每类 baseline 至少一份 |
| 截图/表 | Hierarchy %、Top Conversations、正常 HTTP 三段时间 |
| CLI | `tshark -r base.pcapng -q -z io,phs` 输出存档 |

## 疑问与总结

- Baseline 要**定期更新**（应用升级、IPv6 双栈上线会改分布）。
- 合规：baseline 可能含敏感流量，存储需授权。
