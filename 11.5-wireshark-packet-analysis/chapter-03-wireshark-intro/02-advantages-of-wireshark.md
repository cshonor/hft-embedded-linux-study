# 3.2 Wireshark 的优点

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md)

**核心主旨**：Wireshark 作为首选嗅探/分析工具的核心竞争优势。

## 核心知识点

### 一句话结论

对**初学者**与**专家**均适用：易用 GUI、**免费开源**、**跨平台**、**深度协议解码**与丰富统计，使其成为事实上的行业标准。

### 功能维度

| 维度 | Wireshark |
|------|-----------|
| 学习曲线 | 图形化三面板、着色、过滤器友好 |
| 成本 | 开源免费（GPL） |
| 平台 | Windows / Linux / macOS |
| 协议覆盖 | 数千种 dissector，可扩展 |
| 生态 | 配置文件、Lua 脚本、专家信息、IO 图、流图 |
| 协作 | 导出 pcap/pcapng，团队共用同一分析环境 |

### 与同类工具横向对比（速记）

| 工具 | 定位 | 相对 Wireshark |
|------|------|----------------|
| **tcpdump / tshark** | 命令行、脚本化 | 轻量抓包强；深度交互分析弱于 GUI |
| **OmniPeek 等商业** | 企业级 NPM | 部分场景集成报表/7×24 Probe；闭源、授权费用 |
| **浏览器 DevTools** | HTTP/WebSocket | 仅应用层、本机浏览器流量，无 L2–L4 全貌 |

**选型习惯**：现场/学习 → Wireshark；服务器无人值守抓包 → `tcpdump` 落盘再 Wireshark 打开。

## 抓包/实操记录

（本章以认识工具为主；首次抓包见 [04-get-started.md](./04-get-started.md)）

## 疑问与总结

- 「会 Wireshark」≈ 会 **看协议树 + 过滤器 + 统计**，而非只会点 Start。
- 与 [第2章 监听线路](../chapter-02-traffic-monitor/chapter-summary.md) 结合：工具再强，抓包点不对仍看不到目标流量。
