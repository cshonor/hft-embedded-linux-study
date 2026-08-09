# Ch 10 §1 IPsec 基础与操作模式 · General

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 1. IPsec 基础与操作模式 (General)

**IPsec** — 在 **IP 层** 对 **每个包** 提供 **认证、加密、抗重放** — 全球 **IP VPN** 的事实标准。Linux **IPv4/IPv6 均支持**；IPv6 规范 **要求实现**。

---

## 两大安全协议

| 协议 | 作用 | 现状 |
|------|------|------|
| **AH (Authentication Header)** | **整包认证**（含部分 IP 头）— **不加密** | 少用（**NAT 不友好**） |
| **ESP (Encapsulating Security Payload)** | **加密 + 可选认证** | **主流** |

**抗重放：** **序列号窗口** — 拒绝 **旧序号** 包（§4）。

---

## 两种操作模式

| 模式 | 封装 | 典型场景 |
|------|------|----------|
| **传输 (Transport)** | **仅加密 L4 载荷** — IP 头 **保留**（源/目的 **不变**） | **主机到主机**、共置 **端到端** |
| **隧道 (Tunnel)** | **整个 inner IP 包** 加密，外再包 **新 IP 头** | **site-to-site VPN**、网关间 |

```
传输模式：
[ Outer IP ][ ESP ][ TCP/UDP payload encrypted ]

隧道模式：
[ Outer IP ][ ESP ][ Inner IP ][ TCP/UDP … encrypted ]
```

---

## 与上层 VPN

| 组件 | 关系 |
|------|------|
| **IKE/IPsec** | 标准 **L3 VPN** |
| **WireGuard/OpenVPN** | **用户态/内核其他实现** — 非本章 XFRM 主线 |
| **TLS** | **L4+** — 与 IPsec **互补** |

**HFT：** 共置 **同一 L2 域** 行情/发单 **不用 IPsec** — **crypto + 额外头** 增 **延迟与 jitter**；跨公网 **管理通道** 可用。

---

← [Ch 10](../README.md) · 下一节 [2. IKE](./section-2-IKE与密码学.md)
