# Ch 8 §4 地址自动配置 · Autoconfiguration (SLAAC)

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 4. 地址自动配置 (Autoconfiguration)

**SLAAC（无状态地址自动配置）** — 主机 **无需 DHCP** 即可得到 **全局 IPv6 地址**（也可与 **DHCPv6** 并存取 DNS 等）。

---

## SLAAC 流程

```
1. 生成 link-local fe80::/64（EUI-64 等）
2. DAD 验证唯一（[Ch 7 §5](../../chapter-07-neighbouring-subsystem/notes/section-5-重复地址检测-DAD.md)）
3. 向 ff02::2 发 Router Solicitation (RS)
4. 路由器 Router Advertisement (RA)：
      - 前缀（Prefix Information）
      - 默认路由
      - 首选/有效生命周期 (preferred/valid lifetime)
5. 主机合成 global 地址 = 前缀 + 接口 ID
6. 地址 preferred → deprecated → 回收
```

| 消息 | 方向 | 内容 |
|------|------|------|
| **RS (ICMPv6 133)** | 主机 → 所有路由器 | 「请通告」 |
| **RA (ICMPv6 134)** | 路由器 → 链路 | 前缀、M/O 标志、lifetime |

**RA 标志：** **M**=用 DHCPv6 取地址；**O**=用 DHCPv6 取其他配置 — 视网络设计。

---

## 生命周期

| 阶段 | 含义 |
|------|------|
| **preferred** | 可 **任意** 用作源地址 |
| **deprecated** | 仍 **有效** 但 **不优先** 选为新连接源 |
| **valid 过期** | 地址 **不可用** |

---

## 与 HFT 共置

| 现实 | 说明 |
|------|------|
| **静态规划** | 交易主机常 **手动 `ip -6 addr`** — 禁用 **意外 SLAAC 改址** |
| **稳定源地址** | 防火墙/ACL 绑定 **固定 /128 或 /64 host** |
| **DAD 延迟** | 新地址 **tentative** 期间 **不可发包** — 与 IPv4 **立即可用** 不同 |

```bash
ip -6 addr show
ip -6 route show
```

---

← [3. 扩展头](./section-3-扩展头部.md) · [Ch 8](../README.md) · 下一节 [5. Rx/转发](./section-5-数据包的接收与转发.md)
