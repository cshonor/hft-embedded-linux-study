# Ch 12 §2 网络拓扑结构 · Network Topologies

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 2. 网络拓扑结构 (Network Topologies)

802.11 定义 **BSS（基本服务集）** — Linux 常见两种 **组网模式**。

---

## 基础设施 BSS (Infrastructure)

```
        [ AP / 接入点 ]
       /    |    \
   STA1   STA2   STA3
```

| 步骤 | 说明 |
|------|------|
| **扫描** | 发现 AP（Beacon / Probe） |
| **认证 Authentication** | 开放或共享密钥 |
| **关联 Association** | STA 加入 BSS，获 **AID** |
| **数据** | 经 **AP 转发**（同一 BSS 内 STA↔STA 常 **经 AP**） |

**Linux 模式：** **`NL80211_IFTYPE_STATION`**（客户端）、**`AP`**（hostapd）。

---

## 独立 BSS / Ad Hoc (IBSS)

```
   STA1 ←——→ STA2
     \       /
      \     /
       STA3
```

| | IBSS |
|---|------|
| **中心** | **无 AP** — 站点 **对等** |
| **用途** | 临时组网、Mesh 前身 |
| **限制** | 规模小、协调难 |

**Linux：** **`NL80211_IFTYPE_ADHOC`** — 生产 HFT **不用**。

---

## 与以太网桥

AP 侧常 **802.11 ↔ 以太网桥接** — 无线帧 **解 MAC 头** 后以 **以太帧** 进 **内核 netif**（类似 [Ch 7 邻居](../../chapter-07-neighbouring-subsystem/) L2）。

---

← [1. MAC 头](./section-1-Mac80211与80211-MAC头部.md) · [Ch 12](../README.md) · 下一节 [3. 省电](./section-3-省电模式.md)
