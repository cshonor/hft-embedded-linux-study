# 11.22 小结

> [study.md](../study.md)

---

## 章节核心提炼

### 范式转移

从 **IPv4 + 硬编码端口** → **主机名 + 服务名 + 协议无关**。

### 黄金准则

| 场景 | API |
|------|-----|
| 客户端解析、连接 | **`getaddrinfo`** + 遍历 + `freeaddrinfo` |
| 服务端展示对端 | **`getnameinfo`** |
| **严禁** | `gethostbyname` / `gethostbyaddr` |

### 封装函数（书中）

`host_serv`、`tcp_connect`、`tcp_listen`、`udp_client`、`udp_connect`、`udp_server` — 统一隐藏 hints 与链表遍历。

### 可重入性

新 API **无静态缓冲区** → 多线程安全；旧 API 与 `_r` 变体仅作历史背景。

---

## 与阶段一衔接

| 先前章节 | 本章升级 |
|----------|----------|
| Ch 3 `sockaddr_in` | 通用 `sockaddr` + `addrinfo` |
| Ch 4 `connect` 固定地址 | 多地址 Failover |
| Ch 8 UDP | `udp_client` / `udp_connect` 解析封装 |

---

## 个人学习总结

（待填）
