# 17.9 路由表操作

> [Ch 18 路由套接字](../../4_ArchitectureDesign/Chapter18_RoutingSocket/study.md)

---

## 传统 ioctl 命令

部分 Unix 提供：

| 命令 | 作用 |
|------|------|
| **`SIOCADDRT`** | 添加路由 |
| **`SIOCDELRT`** | 删除路由 |

- 结构：**`<net/route.h>`** 的 **`struct rtentry`**

---

## 重点结论（架构指引）

**新代码不要用 ioctl 操作路由表。**

| 问题 | 说明 |
|------|------|
| 被动变化 | ICMP 重定向、路由守护进程更新时，ioctl **无法异步通知**应用 |
| 现代做法 | **`AF_ROUTE` 路由套接字**（Ch 18）、Linux **Netlink** |

---

## 个人学习总结

（待填）
