# 6.9 TShark VS Tcpdump

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md)

**核心主旨**：两工具差异与选型——探针抓包 vs 无 GUI 深度分析。

## 核心知识点

### 横向对比

| 维度 | **tcpdump** | **TShark** |
|------|-------------|------------|
| **平台** | UNIX/Linux 为主 | **Windows / Linux / macOS** |
| **解析深度** | L3/L4 为主 | **L7**（Wireshark dissector） |
| **统计 / Follow** | 无；靠 `awk`/`grep` | **`-z`** 会话、Hierarchy、follow |
| **资源** | **极低**，适合 7×24 探针 | 较高，适合离线清洗 |
| **过滤器** | BPF only | BPF（`-f`）+ 显示滤（`-Y`） |
| **时间默认** | 绝对时间倾向 | 相对时间 → 用 **`-t ad`** |

### 选型结论

```text
Linux 服务器只要抓下来 → tcpdump（-nn -w）
同一台机要剖 HTTP/DNS、出统计、跟流 → TShark
Windows 服务器无 GUI → TShark（tcpdump 非首选）
超大 pcap 瘦身 → 两者皆可；复杂滤用 tshark -Y 写新文件
```

### 典型工作流

```text
[srv] sudo tcpdump -nni eth0 -w /tmp/cap.pcap -c 10000
      scp → 分析机
[pc]  wireshark cap.pcap   或   tshark -r cap.pcap -Y "..." -z io,phs
```

> **拓展**：`ssh user@host "tcpdump -w - -i eth0 -c 100"` 管道到本地 `wireshark -k -i -` 实时看（需 root 与网络带宽）。

## 抓包/实操记录

| 角色 | 工具 |
|------|------|
| 生产探针 | tcpdump + rotatelogs |
| 应急 SSH | `tshark -i 1 -f "host x" -w -` 或 tcpdump |
| 报告自动化 | `tshark -r ... -q -z conv,ip` 进 CI |

## 疑问与总结

- 不是二选一；**抓用 tcpdump、析用 TShark/GUI** 最常见。
- 与 [§6.0 适用场景](../chapter-06-tshark-tcpdump/chapter-summary.md) 对照：管道、大文件、无 GUI 四条。
