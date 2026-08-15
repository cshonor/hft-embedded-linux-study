# 13.2 无线网卡模式

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md)

**核心主旨**：抓 802.11 管理/控制/数据帧须 **Monitor mode（RFMON）**；Managed 模式只见本机流量。

## 核心知识点

| 模式 | 用途 | 抓包 |
|------|------|------|
| **Managed** | 客户端连 AP（日常上网） | ❌ 仅本机参与帧 |
| **Ad-hoc** | 设备直连组网 | 有限 |
| **Master** | 网卡作 AP（特殊驱动） | 作 AP 侧 |
| **Monitor / RFMON** | 被动听空中该信道**所有**帧 | ✅ **分析首选** |

```text
排障无线 → 必须先 Monitor + 正确信道
```

Linux：`iw dev wlan0 set type monitor`（现代）或 `iwconfig mode monitor`（见 §13.4）。

Windows：多数内置驱动**不支持** Monitor → §13.3 AirPcap 等。

## 抓包/实操记录

| 检查 | 预期 |
|------|------|
| 模式 | `iwconfig` / Wireshark 接口说明为 Monitor |
| 可见 Beacon | 未关联也应看到周边 AP 广播 |

## 疑问与总结

- Monitor 不等于混杂模式；以太网混杂 ≠ 802.11 Monitor。
- 虚拟机 USB 网卡直通更易成功 Monitor。
