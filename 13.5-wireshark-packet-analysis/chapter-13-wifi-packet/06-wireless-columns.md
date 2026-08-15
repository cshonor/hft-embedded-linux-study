# 13.6 在 Packet List 增加无线专用列

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md)

**核心主旨**：「信号满格但极卡」——用 **RSSI、速率、信道** 列验证物理层真相。

## 核心知识点

`Edit` → `Preferences` → **Columns** → Add（需 **radiotap**，§13.3）

| 列名 | 字段 | 解读 |
|------|------|------|
| **RSSI** | `wlan_radio.signal_dbm` | dBm，**越接近 0 越强**（通常 -30 强，-80 弱） |
| **Data rate** | `wlan_radio.data_rate` | 物理层 Mbps |
| **Channel** | `wlan_radio.channel` | 信道号 |

| 交叉验证 | 若 RSSI 好但 rate 低、重传多 → 干扰/冲突，非「距离」问题 |

与 [§11.1 重传](../chapter-11-network-slow-fix/01-tcp-retransmission.md) 联查：无线差导致 TCP 重传。

## 抓包/实操记录

| 问题 | 看列 |
|------|------|
| 慢 | RSSI vs `tcp.analysis.retransmission` 同时间段 |
|  roam | Channel 列是否跳变 |

## 疑问与总结

- 无 `wlan_radio.*` → Capture Type 未选 802.11+Radio 或驱动未注入 radiotap。
