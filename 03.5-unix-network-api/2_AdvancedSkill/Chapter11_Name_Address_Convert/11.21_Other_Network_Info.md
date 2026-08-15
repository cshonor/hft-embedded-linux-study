# 11.21 其他网络相关信息

---

## 辅助数据库

除主机/服务外，Unix 还有：

| 数据库 | 文件 | 函数 |
|--------|------|------|
| **网络名** | `/etc/networks` | `getnetbyname` / `getnetbyaddr` |
| **协议名** | `/etc/protocols` | `getprotobyname` / `getprotobynumber` |

协议号示例：**tcp=6, udp=17, icmp=1** — 创建**原始套接字**时常用。

---

## 与 getaddrinfo 关系

日常 TCP/UDP 应用以 **`getaddrinfo` 的 `service` 参数** 为主；协议数据库多用于底层或特殊套接字。

---

## 个人学习总结

（待填）
