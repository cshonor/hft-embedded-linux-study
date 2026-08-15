# Ch 14 §8 其他杂项 · Misc Topics

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 8. 其他杂项与补充协议

本章末尾 **独立专题** — 与前面 L3/L4 主线 **补充**。

---

## PCI 子系统与网卡

多数 **NIC 为 PCI/PCIe 设备**：

| 概念 | 说明 |
|------|------|
| **配置空间** | BAR、MSI-X — **多队列中断** |
| **WoL (Wake-On-LAN)** | **Magic packet** 唤醒 — **BMC/运维** 用，非 tick |

**HFT：** **MSI-X 每 queue 一向量** — 与 [§3 RSS/RPS](./section-3-忙轮询套接字与收包路径.md) **配合绑核**。

---

## Teaming 链路聚合

**team driver** — **新一代 bonding 替代**：

| | **bonding (旧)** | **team (新)** |
|---|------------------|---------------|
| 模式 | active-backup、802.3ad… | **userspace libteam** 灵活策略 |
| 逻辑设备 | `bond0` | `team0` |

**HFT：** 共置 **常单链路低延迟** — **聚合增复杂**；**双网卡** 多用于 **管理/行情分离** 而非 team。

---

## PPPoE

**PPPoE (Point-to-Point Protocol over Ethernet)** — **DSL/运营商接入**：

| 阶段 | 包 |
|------|-----|
| **Discovery** | PADI → PADO → PADR → PADS |
| **Session** | PPP 帧封装在 **以太类型 0x8864** |

**HFT：** 共置 **不用 PPPoE** — 直连 **光纤/以太**。

---

## Android 网络

**移动栈** — **netd**、**ConnectivityService**、**eBPF/cgroups** 策略；与 **主线 Linux net** **分叉演进**。

**用途：** 嵌入式/移动端 **移植参考** — **交易所共置 ⚪**。

---

## 全书小结（Ch 14）

| 节 | HFT |
|----|-----|
| §1 netns | 🟡 容器/测试 |
| §2 cgroup tc | 🟡 可选隔离 |
| §3 **Busy Poll + RSS/RPS/XPS** | **🔴 核心** |
| §4–§6 蓝牙/6LoWPAN/NFC | ⚪ |
| §7 notifier | 🟡 链路事件 |
| §8 PCI/team/PPPoE | 🟡 选读 |

---

## 相关章节

- 附录：[appendix-A-Linux-API.md](../../appendix-A-Linux-API.md)
- 全书 HFT 路线：[OUTLINE.md](../../OUTLINE.md)

---

← [7. 通知链](./section-7-通知链.md) · [Ch 14](../README.md)
