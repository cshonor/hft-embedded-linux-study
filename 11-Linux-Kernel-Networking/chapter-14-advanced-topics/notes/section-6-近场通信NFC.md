# Ch 14 §6 近场通信 NFC · Near Field Communication

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 6. 近场通信子系统 (NFC)

**NFC** — **13.56 MHz**、**< 2 inch** 极短距 — 支付/配对/标签；**HFT 无关**。

---

## 基础

| | |
|---|---|
| **标签类型** | Type 1–4 — 容量/安全各异 |
| **模式** | Reader/Writer、P2P、Card Emulation |

---

## Linux 架构

```
用户态 (neard / Android NFC stack)
  ↕ netlink
NFC Core (net/nfc/)
  ├── HCI 层
  └── NCI (NFC Controller Interface) — 现代控制器
```

**套接字：**

| 类型 | 用途 |
|------|------|
| **RAW** | 读写器模式 **原始帧** |
| **LLCP** | **点对点** 逻辑链路 |

**Android：** **libnfc-nci** + HAL — 与 Linux 主线 **NCI 趋同**。

---

← [5. 6LoWPAN](./section-5-IEEE802154与6LoWPAN.md) · [Ch 14](../README.md) · 下一节 [7. 通知链](./section-7-通知链.md)
