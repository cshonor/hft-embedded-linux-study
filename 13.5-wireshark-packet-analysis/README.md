# Wireshark Packet Analysis Notes

书籍：《Wireshark 数据包分析实战》

> **打开本文件夹**：每个 `chapter-XX-*/` 或 `appendix-*/` 是一章；**`chapter-summary.md` = 本章总览**，**`序号-英文名称.md` = 小节笔记**。

## 目录规范

| 类型 | 说明 |
|------|------|
| `chapter-xx-…/` | 单章文件夹（附录用 `appendix-a` 等） |
| `chapter-summary.md` | 本章整体总结 |
| `01-xxx.md` | 对应独立小节，**见名知意**（不用 section 通用名） |
| `cheatsheet/` | [核心一页纸](./cheatsheet/notes.md) · [安装与首次抓包](./cheatsheet/install-and-verify.md) |

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

## 前置知识

- [计算机网络 自顶向下](../top_down/)
- [TCP/IP 详解 卷一](../TCP-IP-Volume1-Protocols/)
- [HTTP 权威指南](../http-authoritative-guide/)

## 使用工具

Wireshark · Docker（实验）· NotebookLM（按章上传 `chapter-summary.md` 或单节 `*.md`）

## 其他

- 实验 `.pcap` 可放在对应章文件夹或本根目录（[.gitignore](./.gitignore) 已忽略）
- 自顶向下实验：[99_practice_wireshark_lab](../top_down/99_practice_wireshark_lab/)

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
