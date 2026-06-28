# Ch 10 §4 IPv4 ESP 实现 · ESP Implementation

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 4. IPv4 ESP 实现 (ESP Implementation)

**ESP** — 最常用 IPsec 协议；Linux 通过 **`xfrm_type`** + **`net_protocol`** 挂接 **IPv4 栈**。

---

## ESP 包格式（概念）

```
[ IP header ][ ESP header ][ Payload… ][ Padding ][ Pad Len ][ Next Header ][ ICV ]
                  │              │                              │
                 SPI          加密区                      认证(tag)
              Seq Number
```

| 字段 | 作用 |
|------|------|
| **SPI** | **SA 标识** — 与 dst IP 等 **查 SAD** |
| **Sequence Number** | **单调递增** — **抗重放** |
| **Payload** | 原 **L4 及以下**（传输模式）或 **整个 inner IP**（隧道） |
| **Padding** | 块对齐 |
| **ICV** | **完整性校验**（若启用 auth） |

**协议号：** IPv4 **50 (IPPROTO_ESP)** — 可在 IP 头 **protocol** 或 **UDP 4500 封装**（NAT-T §6）。

---

## 内核注册

| 对象 | 作用 |
|------|------|
| **`esp_type`** (`struct xfrm_type`) | XFRM **transform 回调**：`input`、`output`、`hdr_len`… |
| **`esp4_protocol`** (`struct net_protocol`) | IPv4 **`handler`** — 收包 **ESP 协议号** 入口 |

**初始化：** `esp4_init()` — 向 XFRM 与 **`inet_add_protocol`** 注册。

**算法：** 通过 **`xfrm_state->aead` / `ealg` / `calg`** 绑定 **Crypto API** 模板。

---

## 与 IPv6

**`esp6_*`** 平行 — ESP 载荷 **相同逻辑**；外层 [Ch 8 §3 扩展头](../../chapter-08-ipv6/notes/section-3-扩展头部.md) **或 protocol 50**。

---

← [3. XFRM](./section-3-XFRM框架.md) · [Ch 10](../README.md) · 下一节 [5. Rx/Tx](./section-5-收发IPsec数据包-传输模式.md)
