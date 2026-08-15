# 第6章 用命令行分析数据包

> 全书：[../README.md](../README.md) · 上一章：[第5章 高级特性](../chapter-05-advanced-feature/chapter-summary.md)

## 6.0 核心主旨与适用场景

**核心主旨**：脱离 GUI，用 **TShark** 与 **tcpdump** 完成捕获、过滤、统计与流重组。

### 为什么用命令行

| # | 原因 |
|---|------|
| 1 | **规避信息过载**：只输出需要的字段或统计 |
| 2 | **管道**：`tshark … \| grep`、与脚本/ELK 集成 |
| 3 | **超大 pcap**：流式处理、`-Y` 瘦身；GUI 易占满内存 |
| 4 | **无 GUI 服务器**：SSH 环境只能 CLI |

## 整体框架

```text
6.1 TShark 安装验证
6.2 tcpdump 安装与 sudo
6.3 -D/-i · -w · -c · -r
6.4 -V/-v · -x/-X 控制输出
6.5 -n/-nn 名称解析
6.6 -f BPF · -Y 显示滤
6.7 -t ad 绝对时间
6.8 -z 统计与 follow
6.9 选型与工作流
```

| 小节 | 文件 |
|------|------|
| 6.1 | [01-install-tshark.md](./01-install-tshark.md) |
| 6.2 | [02-install-tcpdump.md](./02-install-tcpdump.md) |
| 6.3 | [03-capture-save-read.md](./03-capture-save-read.md) |
| 6.4 | [04-control-output.md](./04-control-output.md) |
| 6.5 | [05-name-resolution.md](./05-name-resolution.md) |
| 6.6 | [06-filters.md](./06-filters.md) |
| 6.7 | [07-tshark-time-format.md](./07-tshark-time-format.md) |
| 6.8 | [08-tshark-stats-z.md](./08-tshark-stats-z.md) |
| 6.9 | [09-tshark-vs-tcpdump.md](./09-tshark-vs-tcpdump.md) |

## 重点难点

| 点 | 说明 |
|----|------|
| **tshark -D** | Windows 必须用编号 `-i` |
| **-f vs -Y** | BPF 抓 vs Wireshark 显示滤 |
| **tcpdump -nn** | 生产抓包默认 |
| **-t ad** | TShark 对日志对齐 |
| **-z follow,tcp,ascii,0** | CLI 版 Follow Stream |
| **权限** | 抓包要 root；分析 `-r` 往往不要 |

## 实操要点

1. 熟记：`tshark -ni N -w out.pcapng -f "..."` / `tcpdump -nni eth0 -w out.pcap '...'`。
2. 大文件：`tshark -r big -Y "..." -w small -q`。
3. 报告：`tshark -r f -q -z conv,ip` 与 `-z io,phs`。
4. 过滤器见 [cheatsheet/notes.md](../cheatsheet/notes.md)。

## 小节索引

- [6.1 安装 TShark](./01-install-tshark.md)
- [6.2 安装 Tcpdump](./02-install-tcpdump.md)
- [6.3 捕获和保存流量](./03-capture-save-read.md)
- [6.4 控制输出](./04-control-output.md)
- [6.5 名称解析](./05-name-resolution.md)
- [6.6 应用过滤器](./06-filters.md)
- [6.7 TShark 时间显示格式](./07-tshark-time-format.md)
- [6.8 TShark 总结统计](./08-tshark-stats-z.md)
- [6.9 TShark VS Tcpdump](./09-tshark-vs-tcpdump.md)
