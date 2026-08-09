# Ch 12 §8 无线开发流程 · Wireless Development

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 8. 无线开发流程

Linux **无线子系统** 独立演进 — 补丁经 **邮件列表** 进 **wireless 树**，再 **merge 主线**。

---

## Git 树

| 树 | 用途 |
|----|------|
| **`wireless-testing`** | **稳定测试** — 驱动作者常用 |
| **`wireless-next`** | **下一版特性** — 实验性 |
| **Torvalds `linux.git`** | 发布版 **net/wireless/** |

**路径：** `net/mac80211/`、`net/wireless/`、`drivers/net/wireless/`。

---

## 邮件列表

| 列表 | 内容 |
|------|------|
| **linux-wireless@vger** | mac80211、cfg80211、驱动 |
| **netdev** | 与 **核心网络** 交汇补丁 |

**格式：** **`[PATCH net-next]`** — 遵循 **`Documentation/process/submitting-patches.rst`**。

---

## 本章小结

| 节 | 带走 |
|----|------|
| §1 | **mac80211、CSMA/CA、`ieee80211_hdr`** |
| §2 | **Infrastructure BSS vs IBSS** |
| §3 | **TIM/PS-Poll 省电 → 延迟** |
| §4 | **MLME 扫描/认证/关联/漫游** |
| §5 | **`ieee80211_hw`、`ieee80211_rx/tx`** |
| §6 | **AMPDU、Block Ack** |
| §7 | **802.11s Mesh、HWMP** |
| §8 | **wireless-testing、linux-wireless** |

---

## 相关章节

- 下一章：[Ch 13 InfiniBand](../../chapter-13-infiniband/) — 共置 **🟡 选读**
- Netlink：[Ch 2 §4 genl/nl80211](../../chapter-02-netlink-sockets/notes/section-4-通用Netlink协议.md)

---

← [7. Mesh](./section-7-Mesh网络80211s.md) · [Ch 12](../README.md)
