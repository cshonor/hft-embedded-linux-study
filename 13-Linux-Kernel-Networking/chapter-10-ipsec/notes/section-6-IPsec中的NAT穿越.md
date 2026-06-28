# Ch 10 §6 IPsec 中的 NAT 穿越 · NAT Traversal (NAT-T)

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 6. IPsec 中的 NAT 穿越 (NAT Traversal in IPsec)

**ESP + NAT 冲突：** NAT 改 **IP/端口** 且 **不算 ESP 内 ICV** — **AH 几乎不可用**；**ESP** 若 **仅 IP 头 NAT** 尚可，但 **端点 behind NAT** 需 **NAT-T**。

---

## 问题

| 问题 | 原因 |
|------|------|
| **IKE 端口** | UDP **500/4500** 需 **pinhole** |
| **ESP 非 TCP/UDP** | 某些 NAT **不跟踪 protocol 50** |
| **checksum 覆盖** | AH **验 IP 头** — NAT 改头 **破坏 AH** |

---

## NAT-T 模式

**RFC 3947/3948 思路：**

1. **IKE 协商 NAT-T** — 检测 **NAT 存在**
2. **ESP over UDP** — 把 ESP 包 **封装在 UDP 4500** 内 — NAT 当 **普通 UDP 会话**
3. **Keepalive** — 定期 **空 UDP** 保 **mapping**

```
[ Outer IP ][ UDP 4500 ][ ESP… ]   ← NAT 友好
```

内核：**`xfrm4_rcv_encap()`** / **`udp_encap_rcv()`** — 解 UDP 再 **xfrm_input**。

---

## 与 [Ch 9 NAT](../chapter-09-netfilter/notes/section-5-网络地址转换-NAT.md)

| 组合 | 注意 |
|------|------|
| **IPsec passthrough** | 规则 **不 SNAT ESP** 或 **专门 bypass** |
| **L2TP/IPsec** | 用户态 **ppp** + **xfrm** |
| **conntrack** | **ESP/NAT-T** 需 **helper** 或 **NOTRACK** |

---

## HFT

共置 **私网固定 IP** → **通常无 NAT-T**。  
**远程 VPN 运维** 才常遇 **4500/UDP** — 与 **低延迟交易网** **物理隔离**。

---

## 本章小结

| 节 | 带走 |
|----|------|
| §1 | **AH/ESP、传输/隧道** |
| §2 | **IKE 用户态、Netkey、Crypto API** |
| §3 | **XFRM policy/state、SAD 三哈希** |
| §4 | **ESP 头、esp_type 注册** |
| §5 | **xfrm_input/output、xfrm_lookup bundle** |
| §6 | **NAT-T = ESP in UDP 4500** |

---

## 相关章节

- 下一章：[Ch 11 L4](../../chapter-11-layer-4-protocols/) — **HFT 精读**
- Netfilter：[Ch 9](../../chapter-09-netfilter/)

---

← [5. Rx/Tx](./section-5-收发IPsec数据包-传输模式.md) · [Ch 10](../README.md)
