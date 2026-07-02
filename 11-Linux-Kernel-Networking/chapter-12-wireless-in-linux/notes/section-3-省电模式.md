# Ch 12 §3 省电模式 · Power Save Mode

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 3. 省电模式 (Power Save Mode)

无线终端常 **电池供电** — **802.11 Power Save** 让 STA **睡眠**，由 **AP 缓存** 待发数据。

---

## AP 侧缓存

| 缓冲 | 内容 |
|------|------|
| **`ps_tx_buf`** | **单播** — 目标 STA **睡眠** 时排队 |
| **`bc_buf`** | **广播/多播** — 等 **DTIM** 再发 |

---

## STA 唤醒与拉取

```
STA 睡眠
  → 周期性唤醒听 Beacon
  → 查 TIM (Traffic Indication Map) — 是否有待发单播
  → 若有：发 PS-Poll 控制帧
  → AP 释放缓存帧
```

| 机制 | 作用 |
|------|------|
| **TIM** | Beacon 中 **bitmap** — 哪些 AID 有缓冲 |
| **DTIM** | **组播指示** — 广播/多播 delivery 时机 |
| **PS-Poll** | STA **主动拉** 单播缓存 |

---

## 延迟影响

| | 活跃 | 省电 |
|---|------|------|
| **延迟** | 低 | **Beacon 间隔级** + PS-Poll 往返 |
| **吞吐** | 正常 | 突发后 **burst 释放** |

**HFT：** 笔记本 **关省电**（`iw dev wlan0 set power_save off`）；服务器 **无 Wi‑Fi** 更佳。

---

← [2. 拓扑](./section-2-网络拓扑结构.md) · [Ch 12](../README.md) · 下一节 [4. MLME](./section-4-管理层MLME.md)
