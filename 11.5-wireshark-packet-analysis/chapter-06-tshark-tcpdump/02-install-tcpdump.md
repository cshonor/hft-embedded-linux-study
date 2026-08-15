# 6.2 安装 Tcpdump

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md) · 逐步安装：[../cheatsheet/install-and-verify.md](../cheatsheet/install-and-verify.md)

**核心主旨**：Linux/macOS 原生 CLI 抓包；Windows 用 **tshark** 或 **WSL**。

## 核心知识点

### Linux（最常用）

**Ubuntu / Debian**

```bash
sudo apt update
sudo apt install tcpdump -y
tcpdump --version
```

**CentOS / RHEL**

```bash
sudo yum install tcpdump -y    # CentOS 7
sudo dnf install tcpdump -y    # CentOS 8+ / Fedora
```

### macOS

- 系统**常自带** `tcpdump`
- 更新：`brew install tcpdump`

### Windows

| 方案 | 说明 |
|------|------|
| **tshark**（推荐） | 安装 Wireshark 后 PATH 加入 `C:\Program Files\Wireshark` |
| **WSL** | Ubuntu 内按 Linux 安装 `tcpdump` |

### 权限（易错）

```bash
sudo tcpdump -i any -c 5    # 验证
```

普通用户抓包失败 → 先 **sudo** 或查能力/组权限。

## 抓包/实操记录

| 检查 | 命令 |
|------|------|
| 版本 | `tcpdump --version` |
| 网卡 | `ip link` · `tcpdump -D` |
| 落盘 | `sudo tcpdump -nni eth0 -c 100 -w /tmp/cap.pcap` |

首次练习见 [install-and-verify.md](../cheatsheet/install-and-verify.md)。

## 疑问与总结

- 深度解析用 Wireshark 打开同一 pcap（[§6.9](./09-tshark-vs-tcpdump.md)）。
- 生产：**tcpdump 抓** → 下载 → **Wireshark 分析**。
