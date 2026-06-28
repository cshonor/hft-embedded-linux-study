# Ch 10 §2 IKE 与密码学 · IKE and Cryptography

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 2. IKE 与密码学 (IKE and Cryptography)

**数据面** 加解密在 **内核 XFRM**；**控制面** **SA（安全关联）** 协商由用户态 **IKE 守护进程** 完成 — **StrongSwan、Libreswan、racoon** 等。

---

## IKE（Internet Key Exchange）

| 阶段 | 作用 |
|------|------|
| **IKEv1/v2** | 认证对端、协商 **加密/哈希/DH 组**、建立 **Child SA** |
| 输出 | 写入内核 **SAD** — `xfrm_state`（§3） |

```
用户态 IKE daemon
  ↔ Netlink XFRM (NETLINK_XFRM)
  → 内核 SAD / SPD 更新
```

**与 tick 路径关系：** IKE **仅建链/重协商** — 不 per-packet；但 **rekey** 不当可 **短暂断流**。

---

## Linux 两套历史栈

| 栈 | 时代 | 特点 |
|----|------|------|
| **KLIPS** | 2.4 早期 | **内核外** 补丁栈；**OCF** 异步 **硬件 crypto** |
| **Netkey (native)** | **2.6+** | 主线 **`af_key` / XFRM** + **内核 Crypto API** |

**现代：** 仅 **Netkey** — `/proc/net/xfrm_*`、`ip xfrm` 管理。

---

## Crypto API

内核 **`crypto_*`** 框架 — ESP 调用 **AES-GCM、SHA256** 等 **算法驱动**：

| 实现 | 说明 |
|------|------|
| **软件** | aesni、arm CE — CPU 指令加速 |
| **硬件** | QAT、IPsec offload 网卡 — **async** 路径 |

**HFT 共置：** 无 IPsec 则 **无关**；若 **管理 VPN** — 优先 **AES-NI**，避免 **软加密占满 core**。

---

← [1. 基础](./section-1-IPsec基础与操作模式.md) · [Ch 10](../README.md) · 下一节 [3. XFRM](./section-3-XFRM框架.md)
