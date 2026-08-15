# 第3章 Wireshark 入门

> 全书：[../README.md](../README.md) · 上一章：[第2章 监听网络线路](../chapter-02-traffic-monitor/chapter-summary.md)

## 整体框架

```text
3.1 简史（Ethereal → Wireshark · GPL 社区）
        ↓
3.2 优点（开源 / 跨平台 / 深度解码）
        ↓
3.3 安装（Npcap/WinPcap · Linux 包/源码 · macOS）
        ↓
3.4 入门（基线抓包 · 三面板 · 首选项 · 着色 · Profile）
```

| 小节 | 主题 | 文件 |
|------|------|------|
| 3.1 | Ethereal 更名与社区 | [01-wireshark-history.md](./01-wireshark-history.md) |
| 3.2 | 工具优势与对比 | [02-advantages-of-wireshark.md](./02-advantages-of-wireshark.md) |
| 3.3 | 各平台安装与驱动 | [03-install-wireshark.md](./03-install-wireshark.md) |
| 3.4 | 界面与配置 | [04-get-started.md](./04-get-started.md) |

## 重点难点

| 点 | 说明 |
|----|------|
| **Npcap / WinPcap** | 无抓包驱动 = 无法实时抓本机网卡 |
| **基线** | 正常时抓包，故障时才有对照 |
| **三面板联动** | List 选题 → Details 看协议 → Bytes 对十六进制 |
| **名称解析** | 方便读但可能引入额外 DNS 包 |
| **Global vs Personal** | 个人配置覆盖全局；团队共享用 Profile 目录 |
| **Configuration Profiles** | 右下角快速切换分析场景 |

## 实操要点

1. 安装后验证 `Capture` → `Interfaces` 非空并完成试抓。
2. 建立一份 **baseline** pcapng（家庭/办公/机房各一份更佳）。
3. 熟悉 `Edit` → `Preferences` 与 `View` → `Coloring Rules`。
4. 创建至少一个自定义 **Profile**（如 `daily-debug`）。
5. 过滤器语法见 [cheatsheet/notes.md](../cheatsheet/notes.md)。

## 小节索引

- [3.1 Wireshark 简史](./01-wireshark-history.md)
- [3.2 Wireshark 的优点](./02-advantages-of-wireshark.md)
- [3.3 安装 Wireshark](./03-install-wireshark.md)
- [3.4 Wireshark 初步入门](./04-get-started.md)
