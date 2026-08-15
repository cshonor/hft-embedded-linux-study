# 6.1 安装 TShark

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md) · GUI 安装：[§3.3](../chapter-03-wireshark-intro/03-install-wireshark.md)

**核心主旨**：基于终端的 Wireshark——与 GUI 同源解析器，跨平台命令行分析。

## 核心知识点

| 项 | 说明 |
|----|------|
| **TShark** | Wireshark 的 **CLI** 版本；协议 dissector 与 GUI **相同** |
| **安装** | 安装 Wireshark 时 **默认包含** TShark / `dumpcap` |
| **跨平台** | 不依赖图形库；Windows / Linux / macOS **命令语法一致** |

### 验证安装

```bash
tshark -h
```

有完整帮助输出即安装成功。

### 环境变量（拓展）

- Windows：将 Wireshark 安装目录（含 `tshark.exe`）加入 **PATH**，任意目录可调用。
- Linux：`which tshark` 确认在 PATH；部分发行版包名为 `tshark` 与 `wireshark` 分包。

## 抓包/实操记录

| 命令 | 用途 |
|------|------|
| `tshark -v` | 查看版本 |
| `tshark -D` | 列出网卡编号（Windows 抓包前必做） |

## 疑问与总结

- 抓包权限：Linux 常需 root 或 `dumpcap` 能力；与 [§6.2](./02-install-tcpdump.md) tcpdump 权限类似。
- 分析离线文件一般不需 root：`tshark -r file.pcapng`。
