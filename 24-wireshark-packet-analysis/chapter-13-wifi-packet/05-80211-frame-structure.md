# 13.5 802.11 数据包结构

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md) · 深入：[TCP/IP 3.5](../../TCP-IP-Volume1-Protocols/chapter03-link-layer/3.5-wireless-80211.md)

**核心主旨**：802.11 三层帧类型——管理、控制、数据；Beacon 宣告 BSS。

## 核心知识点

### 三大分组类型

| 类型 | `wlan.fc.type` | 作用 |
|------|----------------|------|
| **Management** | 0 | 认证、关联、**Beacon**、Probe 等 |
| **Control** | 1 | **RTS/CTS**、ACK 等拥塞/预约 |
| **Data** | 2 | 承载 LLC/IP；**唯一能桥接到有线**的 802.11 帧 |

上层 TCP/IP 在 Data 帧的 LLC/SNAP 载荷内，与有线一致。

---

### 案例：Beacon

| 项 | 说明 |
|----|------|
| 谁发 | AP **周期广播** |
| 作用 | 宣告 BSS 存在 |
| 关键 IE | **SSID**、信道、速率、Capability、厂商 |

**过滤器**：`wlan.fc.type_subtype == 0x08`（Beacon）

| 子类型速查 | 值（常用） |
|------------|------------|
| Probe Request | 0x04 |
| Probe Response | 0x05 |
| Authentication | 0x0b |
| Association Request | 0x00 |

## 抓包/实操记录

| 练习 | 过滤 |
|------|------|
| 只看管理 | `wlan.fc.type == 0` |
| 只看数据 | `wlan.fc.type == 2` |
| 读 SSID | Beacon → Tagged parameters → SSID |

## 疑问与总结

- 抓 Monitor 才能见**别人家 AP** 的 Beacon；Managed 只见已关联 BSS 部分管理帧。
