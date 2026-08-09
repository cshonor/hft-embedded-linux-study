# Ch 7 §5 重复地址检测 · DAD (Duplicate Address Detection)

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 5. 重复地址检测（DAD - Duplicate Address Detection）

IPv6 地址配置（**手动 / SLAAC / DHCPv6**）后，在 **正式使用前** 发送 **NS** 探测 **链上是否已有相同地址** — 避免 **静默冲突**。

---

## 流程（概念）

```
接口添加 IPv6 地址（初始 tentative）
  → 发送 Neighbor Solicitation
       Target Address = 待测地址
       Source = ::（未配置完成）
  → 若收到 Neighbor Advertisement 表明冲突
       → 标记地址 **duplicate**，不可用
  → 若无冲突
       → 地址 **preferred**，可通信
```

| 状态 | 含义 |
|------|------|
| **tentative** | DAD 进行中 |
| **preferred/deprecated** | DAD 通过 / 生命周期 |
| **duplicate** | **冲突** — 需换地址 |

---

## 与 IPv4 对比

IPv4 **传统无标准 DAD** — 同网段 **重复 IP** 可能导致 **间歇性断连**；靠 **人工规划** 或 **DHCP 冲突检测**。

**HFT：** 纯 IPv4 共置 **靠 IPAM**；双栈/IPv6 管理需 **`ip -6 addr` 看 tentative/DAD**。

---

## 运维

```bash
ip -6 addr show dev eth0
# 见 tentative、dadfailed 等内核标志（视工具输出）
```

**禁用 DAD（特殊场景）：** `accept_dad` sysctl — **仅当明确无冲突风险**（如 isolated L2）。

---

← [4. NDISC](./section-4-IPv6-NDISC邻居发现.md) · [Ch 7](../README.md) · 下一节 [6. NUD](./section-6-NUD网络不可达检测状态机.md)
