# Wireshark 数据包分析实战

> **打开本文件夹**：每个 `chapter-XX-*/` 或 `appendix-*/` 是一章；**`chapter-summary.md` = 本章总览**，**`序号-英文名称.md` = 小节笔记**。

## 目录规范

| 类型 | 说明 |
|------|------|
| `chapter-xx-…/` | 单章文件夹（附录用 `appendix-a` 等） |
| `chapter-summary.md` | 本章整体总结 |
| `01-xxx.md` | 对应独立小节，**见名知意**（不用 section 通用名） |
| `cheatsheet/` | [核心一页纸](./cheatsheet/notes.md) · [安装与首次抓包](./cheatsheet/install-and-verify.md) |
| `hft-scenarios/` | [HFT 专属抓包场景](./hft-scenarios/) — 延迟分析、NIC offload、内核旁路等 |
| `labs/` | [实验指引](./labs/lab-guide.md) — 动手练习 pcap |

## 章节目录

| 章 | 文件夹 | 总览 |
|----|--------|------|
| 1 | [chapter-01-network-basics](./chapter-01-network-basics/) | [summary](./chapter-01-network-basics/chapter-summary.md) |
| 2 | [chapter-02-traffic-monitor](./chapter-02-traffic-monitor/) | [summary](./chapter-02-traffic-monitor/chapter-summary.md) |
| 3 | [chapter-03-wireshark-intro](./chapter-03-wireshark-intro/) | [summary](./chapter-03-wireshark-intro/chapter-summary.md) |
| 4 | [chapter-04-capture-packet](./chapter-04-capture-packet/) | [summary](./chapter-04-capture-packet/chapter-summary.md) |
| 5 | [chapter-05-advanced-feature](./chapter-05-advanced-feature/) | [summary](./chapter-05-advanced-feature/chapter-summary.md) |
| 6 | [chapter-06-tshark-tcpdump](./chapter-06-tshark-tcpdump/) | [summary](./chapter-06-tshark-tcpdump/chapter-summary.md) |
| 7 | [chapter-07-network-layer-proto](./chapter-07-network-layer-proto/) | [summary](./chapter-07-network-layer-proto/chapter-summary.md) |
| 8 | [chapter-08-transport-layer-tcp-udp](./chapter-08-transport-layer-tcp-udp/) | [summary](./chapter-08-transport-layer-tcp-udp/chapter-summary.md) **重点** |
| 9 | [chapter-09-application-layer-proto](./chapter-09-application-layer-proto/) | [summary](./chapter-09-application-layer-proto/chapter-summary.md) |
| 10 | [chapter-10-basic-scenario](./chapter-10-basic-scenario/) | [summary](./chapter-10-basic-scenario/chapter-summary.md) |
| 11 | [chapter-11-network-slow-fix](./chapter-11-network-slow-fix/) | [summary](./chapter-11-network-slow-fix/chapter-summary.md) |
| 12 | [chapter-12-security-analysis](./chapter-12-security-analysis/) | [summary](./chapter-12-security-analysis/chapter-summary.md) |
| 13 | [chapter-13-wifi-packet](./chapter-13-wifi-packet/) | [summary](./chapter-13-wifi-packet/chapter-summary.md) |
| 附录 A | [appendix-a](./appendix-a/) | [summary](./appendix-a/chapter-summary.md) |
| 附录 B | [appendix-b](./appendix-b/) | [summary](./appendix-b/chapter-summary.md) |
| 速查 | [cheatsheet](./cheatsheet/) | [notes.md](./cheatsheet/notes.md) |
| HFT | [hft-scenarios](./hft-scenarios/) | [总览](./hft-scenarios/00-overview.md) |
| 实验 | [labs](./labs/) | [lab-guide.md](./labs/lab-guide.md) |

## 前置知识

- [计算机网络 自顶向下](../top_down/)（如果仓库中有）
- [TCP/IP 详解 卷一](../TCP-IP-Volume1-Protocols/)（如果仓库中有）
- [HTTP 权威指南](../http-authoritative-guide/)（如果仓库中有）
- HFT 仓库内：[12-tcpip-protocols](../12-tcpip-protocols/) · [04.5-network-sockets](../04.5-network-sockets/) · [15-systems-performance](../15-systems-performance/)

## 使用工具

Wireshark · tshark · tcpdump · Docker（实验）· NotebookLM（按章上传 `chapter-summary.md` 或单节 `*.md`）

## HFT 关联

Wireshark 在高频交易（HFT）场景中是**网络延迟分析**的核心工具。交易系统对微秒级延迟敏感，TCP 行为直接影响交易延迟。

| HFT 场景 | Wireshark 价值 | 参考章节 |
|----------|---------------|---------|
| **TCP 重传/快速重传** | 每次重传 = 交易延迟尖峰，需定位丢包点 | [ch11](./chapter-11-network-slow-fix/) · [hft-01](./hft-scenarios/01-tcp-latency-analysis.md) |
| **RTT 测量** | RTT 决定 TCP 窗口大小，影响吞吐与延迟 | [ch11](./chapter-11-network-slow-fix/) · [hft-01](./hft-scenarios/01-tcp-latency-analysis.md) |
| **NIC offload 影响** | TSO/GRO 会让 Wireshark 看到聚合包，非真实线缆包 | [hft-02](./hft-scenarios/02-nic-offload-impact.md) |
| **内核旁路（DPDK/AF_XDP）** | 绕过内核栈，Wireshark 无法抓包，需特殊方案 | [hft-03](./hft-scenarios/03-kernel-bypass-limitations.md) |
| **容器/云环境抓包** | K8s overlay 网络抓包需要特殊技巧 | [hft-04](./hft-scenarios/04-container-cloud-capture.md) |
| **eBPF 对比** | eBPF 可在内核态过滤，比 Wireshark 更低开销 | [hft-05](./hft-scenarios/05-ebpf-vs-wireshark.md) |

### HFT 模块交叉引用

| 主题 | 本模块章节 | 关联模块 |
|------|----------|---------|
| TCP/IP 协议细节 | ch07-ch09 | [12-tcpip-protocols](../12-tcpip-protocols/) |
| Socket 编程 | ch08 | [04.5-network-sockets](../04.5-network-sockets/) |
| 内核网络栈 | ch06 | [13-kernel-networking](../13-kernel-networking/) · [13.5-modern-networking](../13.5-modern-networking/) |
| 系统性能 | ch11 | [15-systems-performance](../15-systems-performance/) |
| eBPF 可观测性 | hft-05 | [16-bpf-observability](../16-bpf-observability/) |
| DPDK 内核旁路 | hft-03 | [14-dpdk](../14-dpdk/) |

## 其他

- 实验 `.pcap` 可放在对应章文件夹或 `labs/` 目录（[.gitignore](./.gitignore) 已忽略）
- 自顶向下实验：[99_practice_wireshark_lab](../top_down/99_practice_wireshark_lab/)（如果仓库中有）

## 小节文件模板

```markdown
# 小节标题
## 核心知识点
## 抓包/实操记录
## 疑问与总结
```

```markdown
# 本章总览（chapter-summary.md）
## 整体框架
## 重点难点
## 实操要点
```
