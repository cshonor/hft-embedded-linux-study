# Ch 10 §5 收发 IPsec 数据包 · Rx & Tx (Transport Mode)

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 5. 接收与发送 IPsec 数据包 (Rx & Tx Path in Transport Mode)

以 **IPv4 传输模式 ESP** 为例 — 隧道模式在 **decap 后多一层 inner IP 路由**。

---

## 接收路径 (Rx)

```
ip_rcv → … → ip_local_deliver
  → 若 protocol == ESP (50)
       → xfrm4_rcv() / esp4_rcv
            → xfrm_input()
                 → SAD lookup (SPI, dst, …)
                 → esp_input()：解密、验 ICV、验序号
                 → 还原 inner payload（TCP/UDP）
            → 继续 L4 交付
```

| 步骤 | 函数 | 要点 |
|------|------|------|
| L3 识别 | **`ip_local_deliver`** demux | protocol **50** |
| XFRM 入口 | **`xfrm4_rcv`** | 包 **metadata**（dst->xfrm） |
| 通用处理 | **`xfrm_input()`** | **replay check**、选 **state** |
| ESP | **`esp_input()`** | **crypto decrypt** |

**失败：** 认证/序号错 → **drop + 计数** — 不交付 L4。

---

## 发送路径 (Tx)

```
TCP/UDP → ip_queue_xmit / ip6_xmit
  → dst 上 xfrm_lookup()（或已 cached bundle）
       → 匹配 xfrm_policy
       → 绑定 xfrm_state 链
  → esp_output()：加密、加 ESP 头、序号++
  → ip_output() → 邻居 → 网卡
```

### `xfrm_lookup()` 与 bundle

| 概念 | 说明 |
|------|------|
| **`xfrm_policy` 匹配** | 五元组 + 方向 → **require** |
| **`xfrm_dst` / bundle** | **缓存** 「路由 dst + 一组 xfrm_state」 — 避免 **每包全策略扫描** |
| **失效** | 策略/state 变更 → **bundle flush** |

---

## 性能要点

| 因素 | 影响 |
|------|------|
| **每包 encrypt/decrypt** | **CPU 主因** |
| **bundle 命中** | 减 **策略 lookup** |
| **AEAD 一次 pass** | 优于 **enc+auth 分离** |

**HFT：** 数据面 **应无 xfrm_lookup miss** 在热路径 — 即 **根本不走 IPsec**。

---

← [4. ESP 格式](./section-4-IPv4-ESP实现.md) · [Ch 10](../README.md) · 下一节 [6. NAT-T](./section-6-IPsec中的NAT穿越.md)
