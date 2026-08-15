# 13.7 无线专用过滤器

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md) · 速查：[cheatsheet/notes.md](../cheatsheet/notes.md)

**核心主旨**：共享介质上多 AP/多客户端混杂 → 用 **BSSID、帧类型、信道** 隔离。

## 核心知识点

### 1. 按 BSSID（AP MAC）

```text
wlan.bssid == 00:11:22:33:44:55
```

只显示该 AP 相关流量，屏蔽其他路由器。

### 2. 按帧类型 / 子类型

| 过滤 | 含义 |
|------|------|
| `wlan.fc.type == 0` | 管理帧 |
| `wlan.fc.type == 1` | 控制帧 |
| `wlan.fc.type == 2` | 数据帧 |
| `wlan.fc.type_subtype == 0x08` | Beacon |
| `wlan.fc.type_subtype == 0x04` | Probe Request |

### 3. 按信道

```text
wlan_radio.channel == 11
```

### 组合示例

```text
wlan.bssid == aa:bb:cc:dd:ee:ff && wlan.fc.type == 2
```

## 抓包/实操记录

| 场景 | 过滤器 |
|------|--------|
| 入网失败 | `wlan.addr == <client_mac>` 看 Auth/Assoc |
| 周边 AP 扫描 | `wlan.fc.type_subtype == 0x04` |

## 疑问与总结

- **wlan.addr** 匹配任一地址字段（SA/DA/BSSID/RA）。
- 解密后才有 IP 过滤器；WPA 需密钥（§13.9）。
