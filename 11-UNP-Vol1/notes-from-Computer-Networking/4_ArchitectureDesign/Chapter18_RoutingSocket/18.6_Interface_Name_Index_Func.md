# 18.6 接口名字和索引函数

> [Ch 7 套接字选项](../../1_BasicFoundation/Chapter07_SocketOption/study.md) · [Ch 21 IPv6 多播](../../3_DeepMaster/Chapter21_Multicast/study.md)

---

## 为何需要

IPv6 多播、`IPV6_MULTICAST_IF`、部分选项用 **接口索引** 而非名称 — 必须在 **名称 ↔ 索引** 间转换。

---

## POSIX 四个标准 API

| 函数 | 作用 |
|------|------|
| **`if_nametoindex(const char *ifname)`** | 名 → 索引；不存在返回 **0** |
| **`if_indextoname(unsigned ifindex, char *ifname)`** | 索引 → 名，写入调用者缓冲区 |
| **`if_nameindex(void)`** | 动态数组：系统全部 (名, 索引) 对 |
| **`if_freenameindex(struct if_nameindex *ptr)`** | 释放 `if_nameindex` 分配的内存 |

---

## 底层实现（常见 Unix）

许多系统上，这 4 个函数内部正是：

```text
打开 AF_ROUTE 路由套接字
  → sysctl(NET_RT_IFLIST) 或 RTM_GET 类消息
  → 解析 if_msghdr / sockaddr_dl
```

应用层应优先用 **POSIX API**，而非自己重复解析路由消息。

---

## 个人学习总结

（待填）
