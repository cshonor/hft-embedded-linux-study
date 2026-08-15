# 第13章 无线网络数据包分析

> 全书：[../README.md](../README.md) · 上一章：[第12章 安全分析](../chapter-12-security-analysis/chapter-summary.md)

## 整体框架

```text
13.1 物理：单信道 · 干扰 · 频谱仪
13.2 Monitor 模式
13.3 Windows / AirPcap + radiotap
13.4 Linux iw 切信道
13.5 802.11 管理/控制/数据 · Beacon
13.6 RSSI / 速率 / 信道列
13.7 BSSID · 帧类型过滤器
13.8 Wireless Profile
13.9 WEP / WPA EAPOL
```

| 文件 | 小节 |
|------|------|
| [01-physical-factors.md](./01-physical-factors.md) | 13.1 |
| [02-adapter-modes.md](./02-adapter-modes.md) | 13.2 |
| [03-sniff-windows-airpcap.md](./03-sniff-windows-airpcap.md) | 13.3 |
| [04-sniff-linux-monitor.md](./04-sniff-linux-monitor.md) | 13.4 |
| [05-80211-frame-structure.md](./05-80211-frame-structure.md) | 13.5 |
| [06-wireless-columns.md](./06-wireless-columns.md) | 13.6 |
| [07-wireless-filters.md](./07-wireless-filters.md) | 13.7 |
| [08-save-wireless-profile.md](./08-save-wireless-profile.md) | 13.8 |
| [09-wireless-security-wep-wpa.md](./09-wireless-security-wep-wpa.md) | 13.9 |

## 13.10 小结

上层 **TCP/IP 相同**，难点在 **802.11 链路层**：Monitor 抓包、帧类型、RSSI/速率、WPA 握手。

掌握无线过滤器与 radiotap 列，才能快速区分**认证失败、干扰、漫游、应用慢**。

## 重点难点

| 点 | 要点 |
|----|------|
| 单信道 | 抓包信道 = AP 信道 |
| Monitor | 抓空口必备 |
| 802.11 + Radio | FCS、radiotap |
| 仅 Data 桥有线 | 管理/控制不转发 |
| Beacon 0x08 | SSID/信道 |
| WPA 成功 | EAPOL replay 1,1,2,2 |
| WPA 失败 | 重试 + Deauth |

## 实操要点

1. Linux：`monitor` + `channel` + 验证 Beacon。
2. 建 **Wireless** Profile（§13.8）。
3. 联读 [TCP/IP 3.5](../../TCP-IP-Volume1-Protocols/chapter03-link-layer/3.5-wireless-80211.md) CSMA/CA、RTS/CTS。

## 小节索引

- [13.1 物理因素](./01-physical-factors.md)
- [13.2 无线网卡模式](./02-adapter-modes.md)
- [13.3 Windows 嗅探](./03-sniff-windows-airpcap.md)
- [13.4 Linux 嗅探](./04-sniff-linux-monitor.md)
- [13.5 802.11 结构](./05-80211-frame-structure.md)
- [13.6 无线专用列](./06-wireless-columns.md)
- [13.7 无线过滤器](./07-wireless-filters.md)
- [13.8 保存配置](./08-save-wireless-profile.md)
- [13.9 无线安全](./09-wireless-security-wep-wpa.md)
