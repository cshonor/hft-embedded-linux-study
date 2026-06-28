# Ch 12 §4 管理层 MLME

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 4. 管理层 (MLME)

**MLME (MAC Layer Management Entity)** — **扫描、认证、关联、漫游**；内核 **mac80211** 处理部分，**wpa_supplicant/hostapd** 经 **nl80211** 驱动。

---

## 扫描 (Scanning)

| 模式 | 行为 |
|------|------|
| **Active** | STA 发 **Probe Request** — 快 |
| **Passive** | 只听 **Beacon** — 合规/隐蔽 |

输出：**BSS 列表**（SSID、BSSID、信道、信号、加密）。

---

## 认证 (Authentication)

| 类型 | 说明 |
|------|------|
| **开放系统 (Open)** | 无密钥 — 仅 **关联前步骤** |
| **共享密钥 (Shared Key)** | 已废弃 WEP 时代 |

现代 **WPA2/WPA3** 在 **关联后 4-way handshake**（用户态）。

---

## 关联与漫游

| 阶段 | 说明 |
|------|------|
| **Association** | STA ↔ AP **首次加入** BSS |
| **Reassociation** | **换 AP**（漫游）— 带 **旧 BSSID** 信息 |

**漫游触发：** 信号弱、负载均衡 — **毫秒～秒级** 中断 — **不适合交易路径**。

---

## 与 netlink

**`NL80211_CMD_TRIGGER_SCAN`**、**`CONNECT`**、**`DISCONNECT`** — cfg80211/mac80211 入口。

---

← [3. 省电](./section-3-省电模式.md) · [Ch 12](../README.md) · 下一节 [5. 内核实现](./section-5-Mac80211内核实现.md)
