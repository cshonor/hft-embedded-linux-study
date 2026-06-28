# Ch 14 §5 IEEE 802.15.4 与 6LoWPAN

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 5. IEEE 802.15.4 与 6LoWPAN

**802.15.4** — **低功耗、短距** WSN（传感器）；**MTU 127 字节**、**无原生 IPv6 多播** — 需 **6LoWPAN** 适配层跑 **IPv6**。

---

## 802.15.4 特点

| | |
|---|---|
| **频段** | 2.4 GHz / sub-GHz |
| **拓扑** | star / mesh（via 上层） |
| **MTU** | **127 B** — 远小于以太 1500 |

---

## 6LoWPAN 适配

**6LoWPAN (IPv6 over Low-Power Wireless Personal Area Network)**：

| 功能 | 说明 |
|------|------|
| **分片/重组** | 大 IPv6 包 → 多个 802.15.4 帧 |
| **头部压缩 (HC)** | 省 airtime — **IPHC、NHC** |
| **ND 优化** | **IPv6 邻居发现** 适配 **低功耗** — 减广播 |

**Linux：** `net/ieee802154/`、`net/6lowpan/`。

---

## HFT

**无交易用途** — **延迟 ms 级、吞吐 kbps** — 与 **共置 μs 栈** 无关。

---

← [4. 蓝牙](./section-4-Linux蓝牙子系统.md) · [Ch 14](../README.md) · 下一节 [6. NFC](./section-6-近场通信NFC.md)
