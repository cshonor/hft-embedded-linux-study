# 17.8 ARP 高速缓存操作

> [Ch 18 路由套接字](../../4_ArchitectureDesign/Chapter18_RoutingSocket/study.md)

---

## 核心机制

`ioctl` 直接操作内核 **ARP 表**（IP → MAC）。

- 结构：**`<net/if_arp.h>`** 的 **`struct arpreq`**

---

## 三大命令

| 命令 | 作用 | 权限 |
|------|------|------|
| **`SIOCGARP`** | 按 IP 查 MAC | 普通用户通常可 |
| **`SIOCSARP`** | 增/改表项（静态绑定、防欺骗） | **root** |
| **`SIOCDARP`** | 删除表项 | **root** |

---

## 个人学习总结

（待填）
