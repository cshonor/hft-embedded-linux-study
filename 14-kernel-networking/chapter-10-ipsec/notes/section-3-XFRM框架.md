# Ch 10 §3 XFRM 框架 · The XFRM Framework

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 3. XFRM 框架 (The XFRM Framework)

**XFRM** — Linux **IPsec 基础设施**：用户态（IKE/`ip xfrm`）与内核 **策略/状态** 的 **Netlink 接口**；数据面 **lookup + transform**。

---

## 网络命名空间

每个 **`netns`** 独立 **`netns_xfrm`**：

| 对象 | 作用 |
|------|------|
| **policy 哈希表** | **SPD** — 哪些流 **需要** IPsec |
| **state 哈希表** | **SAD** — **如何** 加解密 |

容器 **各自 IPsec 策略** — 与 [Ch 9](../chapter-09-netfilter/) netns 隔离 **同构**。

---

## SAD — `xfrm_state`

**安全关联 (SA)** — 单方向 **SPI + 算法 + 密钥**：

| 哈希表 | 键 | 用途 |
|--------|-----|------|
| **`state_bydst`** | 目的地址相关 | 入站/出站 lookup |
| **`state_bysrc`** | 源地址 | 辅助 |
| **`state_byspi`** | **SPI** | **入站 ESP** 快速匹配 |

```bash
ip xfrm state show
```

---

## SPD — `xfrm_policy`

**策略** — **selector**（源/目的前缀、proto、port）→ **动作为**：

| 动作 | 含义 |
|------|------|
| **require** | **必须** IPsec，否则 drop |
| **allow** | 可选 |
| **block** | 禁止 |

```bash
ip xfrm policy show
```

**`xfrm_policy` + `xfrm_state`：** 策略说 **「要不要保护」**；状态说 **「用哪套密钥/SPI」**。

---

## 数据面协作

```
Tx: xfrm_lookup() — 策略 → 选 state → bundle 缓存
Rx: xfrm_input()  — SPI/selector → state → esp_input/ah_input
```

→ 详 §5

---

← [2. IKE](./section-2-IKE与密码学.md) · [Ch 10](../README.md) · 下一节 [4. ESP](./section-4-IPv4-ESP实现.md)
