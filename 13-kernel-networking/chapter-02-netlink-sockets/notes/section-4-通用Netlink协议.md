# Ch 2 §4 通用 Netlink 协议 · Generic Netlink Protocol

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 4. 通用 Netlink 协议 (Generic Netlink Protocol)

传统 Netlink **协议族数量硬上限** **`MAX_LINKS` = 32** — 子系统增多后不够分配。**Generic Netlink**（**Linux 2.6.15+**）在单一 **`NETLINK_GENERIC`** 之上做 **多路复用**，**动态注册** 新「族」。

---

## 问题与解法

```
旧模型：每种子系统占一个 NETLINK_* 号  →  最多 32 个，耗尽
Genl：  全部走 NETLINK_GENERIC
         →  控制消息注册 family name → 动态分配 family ID
         →  数据消息带 genlmsghdr + 属性（仍 TLV）
```

| | 传统 Netlink | Generic Netlink |
|---|-------------|-----------------|
| 族 ID | 编译期 `NETLINK_*` 常量 | **运行时** `genl_family` 注册 |
| 数量 | ≤ 32 | ** practically  unlimited** |
| 工具 | rtnetlink 专用 | **`genl ctrl list`** 列 family |

---

## 典型使用者

| Family | 用途 |
|--------|------|
| **`nl80211`** | 无线配置 — 用户态 **`iw`** |
| **nl802154** 等 | 其它链路层 |
| **非网络** | ACPI、**taskstats**（进程 IO/调度统计）等也走 genl |

无线子系统 [Ch 12](../../chapter-12-wireless-in-linux/) 深度依赖 **nl80211 + genl**；共置 **以太网 HFT** 通常 **不碰**，但 **同一机制** 理解 **`libnl-genl`** 即可。

---

## 开发要点

1. 用户态先 **`GENL_ID_CTRL`** 解析 **`CTRL_CMD_GETFAMILY`** 得到 **动态 family id**。
2. 消息头多一层 **`struct genlmsghdr`**（cmd / version）。
3. 内核侧 **`genl_register_family()`** 注册 ops。

```text
用户：iw dev wlan0 scan
  → libnl-genl → NETLINK_GENERIC → nl80211 处理
```

→ 与 **rtnetlink** 并列：**rtnetlink 管「接口/路由」；genl 管「可扩展子系统插件」**。

---

← [3. 消息格式](./section-3-数据结构与消息格式.md) · [Ch 2](../README.md) · 下一节 [5. sock_diag](./section-5-套接字监控-sock_diag.md)
