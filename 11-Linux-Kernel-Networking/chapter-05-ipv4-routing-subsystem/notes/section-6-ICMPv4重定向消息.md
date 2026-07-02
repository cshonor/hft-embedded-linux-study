# Ch 5 §6 ICMPv4 重定向 · ICMPv4 Redirect Message

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 6. ICMPv4 重定向消息 (ICMPv4 Redirect Message)

当路由器发现 **次优路径 (suboptimal route)** — 典型：**入接口与出接口相同**（主机 **本应直连** 却经路由器转发）— 向 **原发送主机** 发 **ICMP Redirect**，建议 **更优下一跳**。

---

## 生成路径

```
ip_forward() 判定：同一 dev 入/出 或 更优 GW 存在
  → ip_rt_send_redirect()
       → icmp_send(Redirect, code=HOST|NET)
       → 载荷含 **建议网关 IP** + 原 IP 头片段
```

| Code | 含义 |
|------|------|
| **0** | Redirect for network |
| **1** | Redirect for host |
| **2** | Redirect for TOS and network |
| **3** | Redirect for TOS and host |

**限流：** redirect **rate limit** — 防 **ICMP 放大** 与 **刷表**。

---

## 接收处理

主机栈收到 **ICMP Redirect**：

```
icmp_redirect()
  → 校验来源 **必须是当前第一跳 GW**
  → 可能 **更新 host route** 或 **nexthop exception**
  → 后续到该 host  **直连或走建议 GW**
```

**安全：** 仅接受 **来自当前认定网关** 的 redirect；**伪造 redirect** 可导致 **中间人路径** — 生产常 **关闭 accept_redirects**。

---

## 典型拓扑（教科书触发）

```
主机 H ──► 路由器 R（同一以太网）──► 目标 D
         实际 D 与 H 同 L2，H 应 **直接 ARP D**
R 转发时发现 iif == oif → 向 H 发 Redirect：「D 直连，别经我」
```

**HFT 共置 L2：** 同一 VLAN 内 **直连交易对手** — redirect **极少**；若见 redirect 日志，查 **错误默认 GW** 或 **混杂路由**.

---

## 与 §4 异常的关系

Redirect 触发的 **GW 学习** 可落入 **FIB nexthop exception** 或 **临时 /32 host route** — **不必** 用户 `ip route add`。

→ 转发上下文：[Ch 4 §7](../../chapter-04-ipv4/notes/section-7-数据包转发.md)

---

← [5. 策略路由](./section-5-策略路由.md) · [Ch 5](../README.md) · 下一节 [7. 路由缓存移除](./section-7-IPv4路由缓存的移除与FIB-TRIE.md)
