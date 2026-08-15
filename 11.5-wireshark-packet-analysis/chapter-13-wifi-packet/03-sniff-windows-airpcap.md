# 13.3 在 Windows 上嗅探无线网络

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md)

**核心主旨**：Windows 自带驱动通常无 Monitor → **AirPcap** 等专用硬件与 Wireshark 深度集成。

## 核心知识点

### AirPcap

| 项 | 说明 |
|----|------|
| 形态 | USB 无线抓包网卡 |
| 作用 | 突破 Windows 监听模式限制 |
| 集成 | Wireshark Capture Options 中专用项 |

### Capture Options 要点

| 选项 | 建议 |
|------|------|
| **Include 802.11 FCS in Frames** | ✅ 含尾部 **4 字节 FCS**，识别**物理损坏**帧 |
| **Capture Type** | 选 **802.11 + Radio** |
| 效果 | 标准 802.11 头前附加 **radiotap**：**速率、频率、信号强度 dBm、噪声** 等 |

无 radiotap 时难以做 §13.6 的 RSSI/速率列分析。

## 抓包/实操记录

| 步骤 | 操作 |
|------|------|
| 安装 | AirPcap 驱动 + Wireshark 识别接口 |
| 验证 | 抓包可见 `wlan_radio` 字段与 Beacon 广播 |

## 疑问与总结

- 新方案：部分 **Npcap + 支持 Monitor 的 USB 网卡**（芯片依赖列表查 Wireshark Wiki）。
- 仍无法在 Windows 上对**内置 Intel** 稳定 Monitor 时，用 Linux 笔记本或 AirPcap。
