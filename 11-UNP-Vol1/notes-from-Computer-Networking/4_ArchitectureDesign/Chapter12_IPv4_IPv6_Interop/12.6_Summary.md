# 12.6 小结

> [study.md](../study.md)

---

## 章节核心提炼

### 1. 互操作枢纽

**双栈内核** + **IPv4-mapped IPv6 地址（`::ffff:IPv4`）**。

### 2. 服务端优势

通配绑定的 **IPv6 监听套接字** = 同时收 IPv4 / IPv6 — 降低向 IPv6 演进的运维成本。

### 3. 单向包容

| 能力 | 范围 |
|------|------|
| 双栈本地 | IPv4↔IPv6 映射互通 |
| 跨异构网 | 需 NAT64/DNS64 等 |

### 4. 代码演进

严格使用 Ch 11 的 **`getaddrinfo` / `getnameinfo`** + **`sockaddr_storage`**；需区分真 IPv6 时用 **`IN6_IS_ADDR_V4MAPPED`**；仅 IPv6 监听时设 **`IPV6_V6ONLY`**。

---

## 学习路径

```text
Ch11 解析 API → Ch12 双栈互操作 → Ch15 UDS / Ch21 多播（继续协议无关）
```

---

## 个人学习总结

（待填）
