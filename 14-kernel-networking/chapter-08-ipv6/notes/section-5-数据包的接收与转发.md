# Ch 8 §5 接收与转发 · Rx Path & Forwarding

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 5. 数据包的接收与转发 (Rx Path & Forwarding)

IPv6 入站统一 **`ipv6_rcv()`** — 与 [Ch 4 §2 IPv4 `ip_rcv`](../../chapter-04-ipv4/notes/section-2-协议初始化与接收路径.md) **平行**。

---

## 接收主路径

```
ETH_P_IPV6 → ipv6_rcv(skb)
  ├─ 版本、payload_len、hop_limit 检查
  ├─ 解析 Hop-by-Hop 等扩展头
  ├─ NF_INET_PRE_ROUTING
  └─ ip6_rcv_finish()
        └─ fib6_lookup / 路由
              ├─ 本地 → ip6_input() → L4
              └─ 转发 → ip6_forward()
```

| 函数 | 角色 |
|------|------|
| **`ipv6_rcv()`** | L3 入口、统计、早期 drop |
| **`ip6_rcv_finish()`** | **路由决策** |
| **`ip6_input()`** | 本地交付 — TCP/UDP/ICMPv6 |
| **`ip6_forward()`** | **转发** — hop_limit--，无 IP checksum 更新 |

---

## 与 IPv4 对照

| | IPv4 | IPv6 |
|---|------|------|
| 入口 | `ip_rcv` | **`ipv6_rcv`** |
| 钩子 | PRE_ROUTING | **同** |
| TTL | `ttl` + **header checksum 重算** | **`hop_limit`-- 无 IP sum** |
| 本地 | `ip_local_deliver` | **`ip6_input`** |
| 转发 | `ip_forward` | **`ip6_forward`** |

**Netfilter：** **同一 hook 点** — [Ch 9](../../chapter-09-netfilter/) · nft/iptables **family inet6**。

---

## 转发注意

- **hop_limit=0** → ICMPv6 **Time Exceeded**  
- **大于路径 MTU** → **Packet Too Big**（**不**在中间分片）  
- **`net.ipv6.conf.all.forwarding`** — 同 IPv4 forward sysctl  

**HFT：** 纯 IPv4 栈 **可不启 IPv6**（`sysctl disable_ipv6`）— 减 **双栈意外路径**；若禁，确保 **无应用 bind v6**。

---

← [4. SLAAC](./section-4-地址自动配置-SLAAC.md) · [Ch 8](../README.md) · 下一节 [6. MLD](./section-6-多播侦听者发现-MLD.md)
