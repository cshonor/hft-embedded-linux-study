# Ch 12 §5 Mac80211 内核实现 · Kernel Implementation

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 5. Mac80211 的内核实现细节

**分层：** **cfg80211**（配置/nl80211）→ **mac80211**（MAC 逻辑）→ **驱动**（`ieee80211_ops`）→ **固件/硬件**。

---

## 核心数据结构

| 结构 | 作用 |
|------|------|
| **`ieee80211_hw`** | 每 **radio/芯片** 抽象 — 挂 **`ieee80211_ops`** |
| **`ieee80211_ops`** | 驱动回调：**`tx`、`start`、`config`、`sta_add`**… |
| **`sta_info`** | 每 **对端站点** 状态 — 速率、聚合、**PS 标志**、密钥 |
| **`ieee80211_vif`** | 虚拟接口 — AP/STA/Monitor/Mesh |

---

## 接收路径 Rx

```
驱动 ISR / NAPI
  → 提交 skb + 802.11 元数据
  → ieee80211_rx()
       → 解密 (mac80211/crypto)
       → 重放/序号检查
       → 管理帧 → MLME
       → 数据帧 → 去 802.11 头 → 以太类型 demux → netif_rx
```

与 [Ch 14 NAPI](../../chapter-14-advanced-topics/) **同构** — 但无线 **额外 MAC 处理**。

---

## 发送路径 Tx

```
网络栈 / 桥接
  → ieee80211_subif_start_xmit
  → ieee80211_tx()
       → 选速率、排队 (fq/codel)
       → 加密、建 802.11 头
       → drv->ops->tx → 硬件
  → 等 ACK / Block Ack（§6）
```

---

## 接口模式

| 模式 | 用途 |
|------|------|
| **Station** | 客户端 |
| **AP** | 热点 |
| **Monitor** | **抓包** — tcpdump 原始 802.11 |
| **Mesh** | 802.11s（§7） |
| **WDS** | 无线桥 |

---

## debugfs

**`/sys/kernel/debug/ieee80211/<phy>/`** — **stations、keys、queues** — 内核无线 **排障** 标准入口。

---

← [4. MLME](./section-4-管理层MLME.md) · [Ch 12](../README.md) · 下一节 [6. 802.11n](./section-6-高吞吐量80211n.md)
